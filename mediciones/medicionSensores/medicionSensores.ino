#include "..\..\modules\sensores\sensores.h"
#include "..\..\modules\sensores\sensores.cpp"

#define TIEMPO_ESTABILIZACION_MS    3000

const float* valorSensores_;
SemaphoreHandle_t xSemaphore, isrSemaphore, countSemaphore;

// TAREAS PRINCIPALES

/*  
    Defino si quiero tener sincronico, que se ejecute exacto cada 50ms o asincronico
    El asincronico sirve para testear como responde frente a errores de temporizacion
*/
#define SINCRONICO (0)
#define ASINCRONICO (1)
#define TEMPORIZACION (SINCRONICO) // ACA ELIJO CON CUAL COMPILAR

void cicloSensores (void* parameters){
    while(1){
        if( xSemaphoreTake( isrSemaphore, portMAX_DELAY) == pdTRUE ){
            if( xSemaphoreTake( countSemaphore, portMAX_DELAY) == pdTRUE ){
                actualizarSensores();
            }
        }
    }
}

// FUNCIONES DISPONIBLES PARA TESTEO
/*
    Printeo los valores medidos cada 1 segundo para ver el funcionamiento
*/
void printeoValoresSensores(void * parameters){
  while(1){
    if (uxSemaphoreGetCount(countSemaphore) == 0){
        
      for(int i=0; i<CICLOS_PT100_1+CICLOS_PT100_2+CICLOS_LOOP_I;i++){
        xSemaphoreGive(countSemaphore);
      }

        // PRINTEO LOS VALORES
        Serial.print("\n");
        // Serial.print("S1: ");
        // Serial.println(valorSensores_[PT100_1],3);
        // Serial.print("S2: ");
        // Serial.println(valorSensores_[PT100_2]);
        Serial.print("S3: ");
        Serial.println(valorSensores_[LOOP_CORRIENTE],4);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/*
    Tomo muestras de las oscilaciones entre dos instantes de control 
    Calculo la media y varianza, y detecta outliers
*/
#define CANT_MUESTRAS 600
#define CANT_MUESTRAS_INICIALES 3
#define MARGEN_TEMPERATURA      0.5   
#define MARGEN_PRESION          0.5
void medirOscilacionLectura(void * parameters){
    
    float valorSensoresAnteriores_[CANT_SENSORES];

    float media[CANT_SENSORES];
    float varianza[CANT_SENSORES];
    float delta, delta2;

    static float margenes[CANT_SENSORES] = {MARGEN_TEMPERATURA / 2, MARGEN_TEMPERATURA / 2, MARGEN_PRESION};
    static float valorMedio[CANT_SENSORES];
    static int valoresFalladosSuperiores[CANT_SENSORES];
    static int valoresFalladosInferiores[CANT_SENSORES];

    // Inicializo valores
    for (int j=0; j < CANT_SENSORES; j++){
        media[j] = 0;
        varianza[j] = 0;
        valorMedio[j] = 0;
        valoresFalladosSuperiores[j] = 0;
        valoresFalladosInferiores[j] = 0;
    }

    Serial.println("INICIANDO MEDICION...");

    // Espero a que se estabilize, y tomo muestras para ver el valor medio
    bool medicionesIniciales = true;
    int cantMedicionesIniciales = 0;
    while (medicionesIniciales){
        if (uxSemaphoreGetCount(countSemaphore) == 0){
            if (cantMedicionesIniciales < CANT_MUESTRAS_INICIALES){
                
                // Tomo mediciones
                for (int j=0; j < CANT_SENSORES; j++){
                    valorMedio[j] += valorSensores_[j];
                }          
                cantMedicionesIniciales++;

            } else {

                medicionesIniciales = false;
                
                for (int j=0; j < CANT_SENSORES; j++){
                    valorMedio[j] /= CANT_MUESTRAS_INICIALES;
                }

                for (int j=0; j < CANT_SENSORES; j++){
                    valorSensoresAnteriores_[j] = valorSensores_[j];
                }
                Serial.println("MIDIENDO...");
            } 
            
            // Devuelvo el semaforo para volver a medir
            for (int i=0; i<CICLOS_PT100_1+CICLOS_PT100_2+CICLOS_LOOP_I;i++){
                xSemaphoreGive(countSemaphore);
            }
        }
    }

    int i=0;
    while (i < CANT_MUESTRAS){

      if (uxSemaphoreGetCount(countSemaphore) == 0){
        
        Serial.println(valorSensores_[0]);
        for (int j=0; j < CANT_SENSORES; j++){
            
            // RECALCULO MEDIA Y VARIANZA
            delta = valorSensores_[j] - valorSensoresAnteriores_[j] - media[j];
            
            media[j] += delta/(i+1);

            delta2 = valorSensores_[j] - valorSensoresAnteriores_[j] - media[j];
            varianza[j] += delta * delta2;

            // DETECTO OUTLIERS Y GUARDO
            if (valorSensores_[j] < valorMedio[j] - margenes[j]){
                valoresFalladosInferiores[j]++;
                if (valorSensores_[j] <  valorMedio[j] - (2 * margenes[j])){
                    Serial.print("Outlier detectado en sensor ");
                    Serial.print(j+1);
                    Serial.print(": valor=");
                    Serial.print(valorSensores_[j]);
                    Serial.print(" < ");
                    Serial.print(valorMedio[j] - margenes[j]);
                    Serial.print(", a los ");
                    Serial.print(i+CANT_MUESTRAS_INICIALES);
                    Serial.println(" segundos");
                }
                
            }
            if (valorSensores_[j] > valorMedio[j] + margenes[j]){
                valoresFalladosSuperiores[j]++;
                if (valorSensores_[j] >  valorMedio[j] + (2 * margenes[j])){
                    Serial.print("Outlier detectado en sensor ");
                    Serial.print(j+1);
                    Serial.print(": valor = ");
                    Serial.print(valorSensores_[j]);
                    Serial.print(" > ");
                    Serial.print(valorMedio[j] + margenes[j]);
                    Serial.print(", a los ");
                    Serial.print(i+CANT_MUESTRAS_INICIALES);
                    Serial.println(" segundos");
                }
            }
        }

        i++;

        for(int i=0; i<CICLOS_PT100_1+CICLOS_PT100_2+CICLOS_LOOP_I;i++){
            xSemaphoreGive(countSemaphore);
        }

      }
      
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // Calculo y muestro metricas    
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
        Serial.println("\nMETRICAS:");

        float porcentajeErradosTotales = 0;
        
        for (int j=0; j < CANT_SENSORES; j++){
            
            // MEDIA Y VARIANZA
            Serial.print("Sensor ");
            Serial.print(j+1);
            delay(10);
            Serial.print(" -> Media: ");
            Serial.print(valorMedio[j],4);
            delay(10);
            Serial.print(", Varianza: ");
            Serial.print(varianza[j]/CANT_MUESTRAS,4);
            delay(10);

            // OUTLIERS
            porcentajeErradosTotales = ((float)(valoresFalladosInferiores[j]+valoresFalladosSuperiores[j]))/CANT_MUESTRAS;
            Serial.print(": \nValores errados inferiormente: ");
            delay(10);
            Serial.print(valoresFalladosInferiores[j]);
            Serial.print("/");
            Serial.print(CANT_MUESTRAS);
            Serial.print("\nValores errados superiormente: ");
            delay(10);
            Serial.print(valoresFalladosSuperiores[j]);
            Serial.print("/");
            Serial.print(CANT_MUESTRAS);
            Serial.print("\nPorcentaje de error: ");
            delay(10);
            Serial.print(porcentajeErradosTotales);
            Serial.println("\%\n");
            delay(10);

        xSemaphoreGive(xSemaphore);
        }
    }
    
    // termino la prueba
    vTaskDelete(NULL);
}

/*
    Obtengo el valor medio de la temperatura
    A partir de que cambia 10 grados, cuento cuanto tarda en subir otros 20 grados
*/
#define TEMP_INICIO     2
#define AUMENTO_TEMP    10
#define SENSOR_MEDIDO   1
void medirTiempoCambioTemperatura(void * parameters){
    static int valorMedio = 0;

    Serial.println("ESPERAR PARA INCIAR MEDICION");

    // Espero a que se estabilize, y tomo muestras para ver el valor medio
    bool medicionesIniciales = true;
    int cantMedicionesIniciales = 0;
    while (medicionesIniciales){
        if (uxSemaphoreGetCount(countSemaphore) == 0){
            if (cantMedicionesIniciales < CANT_MUESTRAS_INICIALES){
                valorMedio += valorSensores_[SENSOR_MEDIDO - 1];
                cantMedicionesIniciales++;
            } else {
                medicionesIniciales = false;
                valorMedio /= CANT_MUESTRAS_INICIALES;
            } 
            
            // Devuelvo el semaforo para volver a medir
            for (int i=0; i<CICLOS_PT100_1+CICLOS_PT100_2+CICLOS_LOOP_I;i++){
                xSemaphoreGive(countSemaphore);
            }
        }
    }

    Serial.println("LISTO PARA INCIAR MEDICION");
    
    hw_timer_t *timer = NULL;
    bool enMedicion = true;
    bool inicioMedicion = false;
    float tiempoMedido = 0;

    while (enMedicion){
        
        if (uxSemaphoreGetCount(countSemaphore) == 0){
            // En cuanto supera la primer temperatura inicia a contar
            if (inicioMedicion == false && valorSensores_[SENSOR_MEDIDO-1] > valorMedio + TEMP_INICIO){
                inicioMedicion = true;
                timer = timerBegin(100000);
                Serial.println("MEDICION EN PROCESO...");
                Serial.print("Cambio esperado: ");
                Serial.print(valorMedio + TEMP_INICIO);
                Serial.print(" -> ");
                Serial.println(valorMedio + TEMP_INICIO + AUMENTO_TEMP);
            }
            // Si esta contando aumento el contador
            if (inicioMedicion == true ){
                Serial.print("Temperatura: ");
                Serial.print(valorSensores_[SENSOR_MEDIDO-1]);
                Serial.print(", tiempo= ");
                Serial.println(timerReadMillis(timer)* 0.001);

                // Si llega supera el limite dejo de contar
                if (valorSensores_[SENSOR_MEDIDO-1] >= valorMedio + TEMP_INICIO + AUMENTO_TEMP){
                    enMedicion = false;
                    tiempoMedido = timerReadMillis(timer)* 0.001;
                    timerEnd(timer);
                    Serial.println("FIN DE LA MEDICION");
                }
            } 
            // Devuelvo el semaforo para volver a medir
            for (int i=0; i<CICLOS_PT100_1+CICLOS_PT100_2+CICLOS_LOOP_I;i++){
                xSemaphoreGive(countSemaphore);
            }
        }
    vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    Serial.print("\nTiempo en aumentar ");
    Serial.print(AUMENTO_TEMP);
    Serial.print("°C: ");
    Serial.print(tiempoMedido);
    Serial.println(" segundos");

    // termino la prueba
    vTaskDelete(NULL);
}

// Interrupcion de mediciones listas
void IRAM_ATTR isr_mediciones() {
    BaseType_t xHigherPriorityTaskWoken;
    xSemaphoreGiveFromISR(isrSemaphore, &xHigherPriorityTaskWoken);
}

void setup(){
    Serial.begin(250000);

    // pinMode(15,OUTPUT);
    // digitalWrite(15,HIGH);
    // pinMode(25,OUTPUT);
    // digitalWrite(25,HIGH);
    inicializarSensores();

    valorSensores_ = obtenerValorSensores();
    
    xSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore); 

    countSemaphore = xSemaphoreCreateCounting(CICLOS_PT100_1+CICLOS_PT100_2+CICLOS_LOOP_I+1,CICLOS_PT100_1+CICLOS_PT100_2+CICLOS_LOOP_I+1);

    isrSemaphore = xSemaphoreCreateBinary();
    attachInterrupt(digitalPinToInterrupt(2), isr_mediciones, FALLING);


    xTaskCreate(cicloSensores, "Task 1", 4000, NULL, 1, NULL);
    
    // De los siguientes conjuntos descomentar uno

    // 1. Vista de funcionamiento
    // xTaskCreate(printeoValoresSensores, "Task 2", 4000, NULL, 1, NULL);
        
    // 2. Metricas con los sensores en condiciones estables
    xTaskCreate(medirOscilacionLectura, "Task 3", 4000, NULL, 1, NULL);

    // 3. Metrica para cambio de temperatura
    // xTaskCreate(medirTiempoCambioTemperatura, "Task 5", 2000, NULL, 1, NULL);
}

void loop(){
    
}
