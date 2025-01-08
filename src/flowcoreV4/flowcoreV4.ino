#include "../../lib/Unity/unity.h"

//-------------- MODULOS ---------------------
#include "../../modules/contador/contador.h"
#include "../../modules/contador/contador.cpp"

#include "../../modules/reles/reles.h"
#include "../../modules/reles/reles.cpp"

#include "../../modules/entradasMecanicas/entradasMecanicas.h"
#include "../../modules/entradasMecanicas/entradasMecanicas.cpp"

#include "../../modules/sensores/sensores.h"
#include "../../modules/sensores/sensores.cpp"

#include "../../modules/buzzer/buzzer.h"
#include "../../modules/buzzer/buzzer.cpp"

#include "../../modules/NVS/NVS.h"
#include "../../modules/NVS/NVS.cpp"

#include "../../modules/controles/controles.h"
#include "../../modules/controles/controles.cpp"

#include "../../modules/display/display.h"
#include "../../modules/display/display.cpp"

//--------------------------------------------


#define TIMEPO_REFRESCO_DISPLAY_MS  10

SemaphoreHandle_t xSemaphore, isrSemaphore, initSemaphore;

// uint64_t medir_WCET, presentar_WCET, controlar_WCET;
// uint64_t tickRate = configTICK_RATE_HZ;

void medir(void * parameters){

  static uint64_t cycleCounter = 0;
  
  while(1){
    // Primero trato de obtener el semaforo de la interrupcion
    if (xSemaphoreTake(isrSemaphore, portMAX_DELAY) == pdTRUE) {
      // Tomo el semaforo contador para indicar que entre una vez mas a las mediciones
      if( xSemaphoreTake( xSemaphore, portMAX_DELAY) == pdTRUE ){

        // time_meter_us_reset();

        actualizarSensores();
        
        // cycleCounter = time_meter_us_get();

        // if (medir_WCET < cycleCounter){
        //   medir_WCET = cycleCounter;
        //   Serial.print("WCET mediciones: ");
        //   Serial.println(medir_WCET);
        // }

      }
    }
    vTaskDelay( 10 / portTICK_PERIOD_MS); // SIN ESTO SALTA EL WATCHDOG
  }
}

void presentar(void * parameters){

  TickType_t  xLastWakeTime = xTaskGetTickCount();
  // static uint64_t cycleCounter = 0;
  
  static bool init = false;
  while (init == false){
    if (xSemaphoreTake( initSemaphore, portMAX_DELAY) == pdTRUE){
      init = true;
    }
    vTaskDelay( 10 / portTICK_PERIOD_MS); // SIN ESTO SALTA EL WATCHDOG
  }

  while(1){
    
    // time_meter_us_reset();
    actualizarDisplay(TIMEPO_REFRESCO_DISPLAY_MS);
    // cycleCounter = time_meter_us_get();

    // if (presentar_WCET < cycleCounter){
    //   presentar_WCET = cycleCounter;
    //   Serial.print("WCET presentacion: ");
    //   Serial.println(presentar_WCET);
    // }
    vTaskDelayUntil(&xLastWakeTime, TIMEPO_REFRESCO_DISPLAY_MS / portTICK_PERIOD_MS);
  }
}

void controlar(void * parameters){
  
  // Espero 5 ciclos de medicion a que se estabilize el sistema

  int i=0;
  while(i<5){
    if (uxSemaphoreGetCount(xSemaphore) == 0){
      i++;
      for(int i=0; i<CICLOS_PT100_1+CICLOS_PT100_2+CICLOS_LOOP_I;i++){
        xSemaphoreGive(xSemaphore);
      }
    }
  }

  xSemaphoreGive(initSemaphore);

  // static uint64_t cycleCounter = 0;
  while(1){
    if (uxSemaphoreGetCount(xSemaphore) == 0){
      
      for(int i=0; i<CICLOS_PT100_1+CICLOS_PT100_2+CICLOS_LOOP_I;i++){
        xSemaphoreGive(xSemaphore);
      }

      
      // time_meter_us_reset();

        actualizarEntradasMecanicas();
        actualizarControles();
        actualizarReles();
        // actualizarBuzzer();

      // cycleCounter = time_meter_us_get();

      // if (controlar_WCET < cycleCounter){
      //   controlar_WCET = cycleCounter;
      //   Serial.print("WCET control: ");
      //   Serial.println(controlar_WCET);
      // }
    }
    vTaskDelay( 10 / portTICK_PERIOD_MS); // SIN ESTO SALTA EL WATCHDOG
  }
}

// // Interrupcion de mediciones listas
void IRAM_ATTR isr_mediciones() {
    BaseType_t xHigherPriorityTaskWoken;
    xSemaphoreGiveFromISR(isrSemaphore, &xHigherPriorityTaskWoken);
}


void setup(){
    Serial.begin(115200);

    // // Serial.println(tickRate);
    // // time_meter_us_init();
    
    xSemaphore = xSemaphoreCreateCounting(CICLOS_PT100_1+CICLOS_PT100_2+CICLOS_LOOP_I+1,CICLOS_PT100_1+CICLOS_PT100_2+CICLOS_LOOP_I+1);
    isrSemaphore = xSemaphoreCreateBinary();
    initSemaphore = xSemaphoreCreateBinary();

    // if (xSemaphore == NULL || isrSemaphore == NULL) {
    //     Serial.println("Error al crear semáforo.");
    //     while(1);
    // }

    // pinMode(42, OUTPUT);
    // digitalWrite(42,HIGH);
    // pinMode(21,OUTPUT);
    // digitalWrite(21,HIGH);
    // pinMode(11,OUTPUT);
    // digitalWrite(11,HIGH);
    
    inicializarNVS();
    inicializarSensores(); 
    inicializarDisplay();
    
    inicializarEntradasMecanicas();
    inicializarReles();

    inicializarBuzzer();
    inicializarControles();

    inicializarContador();

    attachInterrupt(digitalPinToInterrupt(2), isr_mediciones, FALLING);
    // attachInterrupt(digitalPinToInterrupt(36), isr_mediciones, FALLING); // Pin de entrada de sensado de termostato de seguridad

    size_t totalHeap = ESP.getHeapSize();
    size_t freeHeap = xPortGetFreeHeapSize();

    // Serial.printf("Total Heap Size: %d bytes\n", totalHeap);
    // Serial.printf("Free Heap Size: %d bytes\n", freeHeap);
    
    xTaskCreate(medir, "Task 1", 4000, NULL, 3, NULL);
    xTaskCreate(controlar, "Task 2", 5000, NULL, 2, NULL);
    xTaskCreate(presentar, "Task 3", 50000, NULL, 1, NULL);
}

void loop(){}