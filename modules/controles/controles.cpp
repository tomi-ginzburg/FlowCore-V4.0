#include "controles.h"

#include "..\contador\contador.h"
#include "..\reles\reles.h"
#include "..\entradasMecanicas\entradasMecanicas.h"
#include "..\sensores\sensores.h"
#include "..\buzzer\buzzer.h"
#include "..\NVS\NVS.h"

#include <Arduino.h>

// =====[Declaracion de defines privados]============

#define LIMITE_INF_PRESION      1
#define LIMITE_SUP_PRESION      20
#define LIMITE_INF_TEMPERATURA  5
#define LIMITE_SUP_TEMPERATURA  80
#define TEMPERATURA_EMERGENCIA  95
#define CANT_RESISTENCIAS       2
#define CANT_RESISTENCIAS_TOTALES 4
#define CANT_MUESTRAS_FALLA     3

// Tiempos para verificar falla
#define TIEMPO_BOMBA_CAL_MS         10000
#define TIEMPO_BOMBA_ACS_MS         5000
#define TIEMPO_APAGADO_BOMBAS_MS    5000

#define TIEMPO_PURGA_ON             15000
#define TIEMPO_PURGA_OFF            30000

// Tiempos de accion de alarma
#define TIEMPO_SEG_EMERGENCIA       5000    // Tiempo antes de accionar la falla del disyuntor
#define TIEMPO_BOMBAS_EMERGENCIA    10000  // Tiempo durante el cual accionar las bombas en caso de alta temperatura    

// =====[Declaracion de tipos de datos privados]=====

//---------------------------------------------------
// ESTO VA A SER PARTE DE LAS CONFIGURACIONES

uint64_t tiempoDuracionMantenimiento = 5000;
uint64_t tiempoEsperaMantenimiento = 2592000000;

//---------------------------------------------------

estadoControl_t estadoControl;
// estadoControl_t estadoControlAnterior;
causaAlarma_t causaAlarma;

#ifdef TESTING
int *estadoEntradasControl;
float *valorSensoresControl;
uint64_t *tiempoEstadoEntradasControl;
uint64_t *tiempoEncendidoEtapasControl;
uint64_t *tiempoApagadoEtapasControl;
int32_t *alarmasControl;
#else
const int *estadoEntradasControl;
const float *valorSensoresControl;
const uint64_t *tiempoEstadoEntradasControl;
const uint64_t *tiempoEncendidoEtapasControl;
const uint64_t *tiempoApagadoEtapasControl;
const int32_t *alarmasControl;
#endif

const bool *etapasControl;
const Configs *configuracionesControl;

uint64_t contadorMantenimiento = 0;
int relesDeResistencias[CANT_RESISTENCIAS_TOTALES] = {ETAPA_1, ETAPA_2, ETAPA_3, ETAPA_4};

bool acsOn = true;
bool supervisionOn = false;
bool flagsPurga[CANT_BOMBAS] = {false, false};

// =====[Declaracion de funciones privadas]==========
void verificarInicioControl();
void actualizarEstadoControl();

void actualizarEstadoAlarma();
void verificarFalla();
int obtenerPrioridadAlarma(causaAlarma_t causa);

void actualizarEtapasSalida();
void controlarResistencias();
void actualizarControlesAlarma();
void actualizarMantenimiento();
void purgarBomba(int bomba);

#ifdef TESTING
void mockingSensores();
void mockingEntradasMecanicas();
void test_resistenciasApagadas();
void test_fallaTermostatoSeguridad_forzarFalla();
void test_fallaTermostatoSeguridad_verificarTest();
void test_fallaTemperaturaAlta_forzarFalla();
void test_fallaTemperaturaAlta_verificarTest();
void test_fallaTemperaturaBaja_forzarFalla();
void test_fallaTemperaturaBaja_verificarTest();
void test_fallaApagadoBombas_forzarFalla();
void test_fallaApagadoBombas_verificarTest();
void test_fallaSensorCaldera_forzarFalla();
void test_fallaSensorCaldera_verificarTest();
void test_fallaSensorACS_forzarFalla();
void test_fallaSensorACS_verificarTest();
void test_fallaEncendidoBombaCalefaccion_forzarFalla();
void test_fallaEncendidoBombaCalefaccion_verificarTest();
void test_fallaEncendidoBombaACS_forzarFalla();
void test_fallaEncendidoBombaACS_verificarTest();
void test_fallaPresionAlta_forzarFalla();
void test_fallaPresionAlta_verificarTest();
void test_fallaPresionBaja_forzarFalla();
void test_fallaPresionBaja_verificarTest();
void test_gestionFallas(void (*forzarFalla)(), void (*testearRespuesta)(), const bool* tests = nullptr);
void test_superposicionFallas(void (*fallaPrimera)(), const bool* tests);
#endif

// =====[Implementacion de funciones publicas]=======

void inicializarControles(){
    estadoControl = APAGADO;

    // Al iniciarse se guarda en estadoControlAnterior el estado que tenia la caldera antes de apagarse, idem con causaAlarma
    // estadoControlAnterior = ENCENDIDO_IDLE;
    causaAlarma = NO_FALLA;

    // Obtengo los punteros a variables de otros modulos
    estadoEntradasControl = obtenerEstadoEntradasMecanicas();
    tiempoEstadoEntradasControl = obtenerTiempoCambioEntradasMecanicas();
    etapasControl = obtenerEstadoEtapas();
    tiempoEncendidoEtapasControl = obtenerTiempoEncendidoEtapas();
    tiempoApagadoEtapasControl = obtenerTiempoApagadoEtapas();
    valorSensoresControl = obtenerValorSensores();
    configuracionesControl = obtenerConfiguracionesNVS();
    alarmasControl = obtenerAlarmasNVS();
    
    // verificarInicioControl();
    
}

void actualizarControles(){

    actualizarEstadoControl();
    actualizarEtapasSalida();
    if (estadoControl != ALARMA){
        actualizarMantenimiento();
    }
}

void solicitarPurgarBomba(int bomba){
    if (bomba == BOMBA_CAL ){
        flagsPurga[0] = !flagsPurga[0];
    }
    else if (bomba == BOMBA_ACS){
        flagsPurga[1] = !flagsPurga[1];
    }    
}

void solicitarDiagnosticoEtapa(int etapa, bool encendido){
    if (encendido){
        solicitarActivarRele(relesDeResistencias[etapa]);
    }
    else {
        solicitarDesactivarRele(relesDeResistencias[etapa]);
    }
}

const estadoControl_t* obtenerEstadoControl(){
    return &estadoControl;
}

const causaAlarma_t* obtenerCausaAlarma(){
    return &causaAlarma;
}

#ifdef TESTING

void reiniciarEstadoControles(){
    estadoControl = APAGADO;
    causaAlarma = NO_FALLA;
    mockingSensores();
    mockingEntradasMecanicas();
    for (int i=0; i < CANT_ETAPAS - 1; i++){
        solicitarDesactivarRele(i, false, 0);
        flagsEtapasDesactivadas[i] = false;
    }
    actualizarReles();
    for (int i = 0; i< CANT_ALARMAS; i++){
        alarmasControl[i] = 0;
    }
    if (configuracionesControl->calefaccionOn == false){
        bool calOn = true;
        guardarConfigsNVS(CALEFACCION_ON, &calOn, sizeof(calOn));
    }
    if (acsOn == false){
        acsOn = true;
    }

    
}

void test_verificarFallaTermostatoSeguridad(){

    test_gestionFallas(test_fallaTermostatoSeguridad_forzarFalla, test_fallaTermostatoSeguridad_verificarTest);
    bool pruebasSeleccionadas[] = {false, true, false, true, true, true, true, true, true, true};
    test_superposicionFallas(test_fallaTermostatoSeguridad_forzarFalla, pruebasSeleccionadas); 
    
}

void test_verificarFallaTemperaturaAlta(){

    test_gestionFallas(test_fallaTemperaturaAlta_forzarFalla, test_fallaTemperaturaAlta_verificarTest);
    bool pruebasSeleccionadas[] = {true, false, false, true, true, true, true, true, true, false};
    test_superposicionFallas(test_fallaTemperaturaAlta_forzarFalla, pruebasSeleccionadas); 
}

void test_verificarFallaTemperaturaBaja(){

    test_gestionFallas(test_fallaTemperaturaBaja_forzarFalla, test_fallaTemperaturaBaja_verificarTest);
    bool pruebasSeleccionadas[] = {false, false, false, true, true, true, true, true, false, true};
    test_superposicionFallas(test_fallaTemperaturaBaja_forzarFalla, pruebasSeleccionadas); 
}

void test_verificarFallaApagadoBombas(){

    bool pruebasSeleccionadas[] = {false, true, false, false, true};
    test_gestionFallas(test_fallaApagadoBombas_forzarFalla, test_fallaApagadoBombas_verificarTest, pruebasSeleccionadas);
    bool pruebasSeleccionadas2[] = {true, true, true, false, true, true, false, false, true, true};
    test_superposicionFallas(test_fallaApagadoBombas_forzarFalla, pruebasSeleccionadas2); 
}

void test_verificarFallaSensorCaldera(){

    test_gestionFallas(test_fallaSensorCaldera_forzarFalla, test_fallaSensorCaldera_verificarTest);
    bool pruebasSeleccionadas[] = {true, false, false, true, false, true, true, true, true, true};
    test_superposicionFallas(test_fallaSensorCaldera_forzarFalla, pruebasSeleccionadas); 
}

void test_verificarFallaSensorACS(){

    test_gestionFallas(test_fallaSensorACS_forzarFalla, test_fallaSensorACS_verificarTest);
    bool pruebasSeleccionadas[] = {true, true, true, true, true, false, true, false, true, true};
    test_superposicionFallas(test_fallaSensorACS_forzarFalla, pruebasSeleccionadas); 
}

void test_verificarFallaEncendidoBombaCalefaccion(){
    
    bool pruebasSeleccionadas[] = {false, false, true, false, true};
    test_gestionFallas(test_fallaEncendidoBombaCalefaccion_forzarFalla, test_fallaEncendidoBombaCalefaccion_verificarTest, pruebasSeleccionadas);
    bool pruebasSeleccionadas2[] = {true, true, true, true, true, true, false, true, true, true};
    test_superposicionFallas(test_fallaEncendidoBombaCalefaccion_forzarFalla, pruebasSeleccionadas2); 
}

void test_verificarFallaEncendidoBombaACS(){
    
    bool pruebasSeleccionadas[] = {false, false, false, true, true};
    test_gestionFallas(test_fallaEncendidoBombaACS_forzarFalla, test_fallaEncendidoBombaACS_verificarTest, pruebasSeleccionadas);
    bool pruebasSeleccionadas3[] = {true, true, true, true, true, true, true, false, true, true};
    test_superposicionFallas(test_fallaEncendidoBombaACS_forzarFalla, pruebasSeleccionadas3); 
}

void test_verificarFallaPresionAlta(){

    test_gestionFallas(test_fallaPresionAlta_forzarFalla, test_fallaPresionAlta_verificarTest);
    bool pruebasSeleccionadas4[] = {true, true, false, true, true, true, true, true, false, false};
    test_superposicionFallas(test_fallaPresionAlta_forzarFalla, pruebasSeleccionadas4); 
}

void test_verificarFallaPresionBaja(){

    test_gestionFallas(test_fallaPresionBaja_forzarFalla, test_fallaPresionBaja_verificarTest);
    bool pruebasSeleccionadas[] = {true, false, true, true, true, true, true, true, false, false};
    test_superposicionFallas(test_fallaPresionBaja_forzarFalla, pruebasSeleccionadas); 
}

#endif


// =====[Implementacion de funciones privadas]=======

void verificarInicioControl(){
    if (estadoEntradasControl[TERMOSTATO_SEG] == LOW){
        estadoControl = ENCENDIDO_SUPERVISION;
        solicitarActivarRele(ETAPA_6, true, TIEMPO_BOMBA_ACS_MS);
        solicitarActivarAlarma();
    } else {
        estadoControl = ENCENDIDO_IDLE;
    }

}

void actualizarEstadoControl(){

    // Veo si hay alguna causa de alarma, o si habia causa si ya termino
    actualizarEstadoAlarma();
    
    // Actualizo el estado inmediatamente anterior
    // estadoControlAnterior = estadoControl;


    // Actualizo las cosas comunes a distitnos estados

    // Si se apaga la caldera con el boton de apagado, apago todas las salidas
    if (estadoControl != APAGADO && estadoControl != ALARMA){
        if (configuracionesControl->encendidoOn == false){
            estadoControl = APAGADO;
            for (int i=0; i < CANT_RESISTENCIAS; i++){
                solicitarDesactivarRele(i);
            }
            solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
            solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
        }
    }

    // Si se pasa a modo diagnostico apago todas las salidas
    if (estadoControl != DIAGNOSTICO){
        if (configuracionesControl->diagnosticoOn == true){
            estadoControl = DIAGNOSTICO;
            for (int i=0; i < CANT_RESISTENCIAS; i++){
                solicitarDesactivarRele(i);
            }
            solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
            solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
        }
    }


    // Actualizo el estado del control actual
    switch (estadoControl){
        case APAGADO: 
            // Si se vuelve a encender paso a estado ENCENDIDO_IDLE
            if (configuracionesControl->encendidoOn == true){
                estadoControl = ENCENDIDO_IDLE;
            }
            break;
        case ENCENDIDO_IDLE:
            // Si se activa el flujostato del agua sanitaria y esta activo el modo ACS, cambia de estado a ENCENDIDO_ACS 
            if (estadoEntradasControl[FLOW_SW_ACS] == LOW && acsOn == true){
                // Apaga la bomba de calefaccion
                if (etapasControl[ETAPA_5] == HIGH && contadorMantenimiento > tiempoDuracionMantenimiento){
                    solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
                } 
                estadoControl = ENCENDIDO_ACS;
            }
            
            break;

        case ENCENDIDO_ACS:
            
            // Si se desactiva el flujostato del agua sanitaria cambia de estado a ENCENDIDO_IDLE 
            if (estadoEntradasControl[FLOW_SW_ACS] == HIGH){
                // Apaga la bomba de agua caliente
                solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
                estadoControl = ENCENDIDO_IDLE;  
            }
            
            break;

        case ENCENDIDO_SUPERVISION:
            // static int contadorFallaTempAlta = 0;

            // if (valorSensoresControl[PT100_1] > LIMITE_SUP_TEMPERATURA){
            //     contadorFallaTempAlta++;
            //     if (contadorFallaTempAlta > 2){
            //         causaAlarma = TERMOSTATO_SEGURIDAD;
            //         estadoControl = ALARMA;

            //         contarMs(&contadorFalla, 1000, LOW);
            //         for (int i=0; i < CANT_RESISTENCIAS; i++){
            //             solicitarDesactivarRele(i);
            //         }
            //         solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
            //         solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
            //     }
            // }

            // // Si se pide ACS se controlan las resisitencias
            // if (estadoEntradasControl[FLOW_SW_ACS] == LOW && supervisionOn == false){
            //     supervisionOn = true;
            // } 

            // // Si se deja de pedir ACS se apagan las resistencias
            // else if (estadoEntradasControl[FLOW_SW_ACS] == HIGH && supervisionOn == true){
            //     supervisionOn = false;
            //     for (int i=0; i < CANT_RESISTENCIAS; i++){
            //         solicitarDesactivarRele(i);
            //     }

            // }

            // break;
            
        case DIAGNOSTICO: 
            if (configuracionesControl->diagnosticoOn == false){
                estadoControl = APAGADO;
                for (int i=0; i < CANT_RESISTENCIAS; i++){
                    solicitarDesactivarRele(i);
                }
                solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
                solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
                flagsPurga[0] = false;
                flagsPurga[1] = false;
            }
            break;
        case ALARMA: break;
        default: estadoControl = APAGADO;
    }
}

void actualizarEstadoAlarma(){

    if (estadoControl != ENCENDIDO_SUPERVISION){
        verificarFalla();
    }
    
    switch (causaAlarma){
        case PRESION_ALTA:

            if (valorSensoresControl[LOOP_CORRIENTE] < LIMITE_SUP_PRESION && valorSensoresControl[LOOP_CORRIENTE] != VALOR_ERROR_SENSOR){
                causaAlarma = NO_FALLA;
            }
            break;

        case PRESION_BAJA:

            if (valorSensoresControl[LOOP_CORRIENTE] > LIMITE_INF_PRESION && valorSensoresControl[LOOP_CORRIENTE] != VALOR_ERROR_SENSOR){
                causaAlarma = NO_FALLA;
            }
            break;

        case TEMPERATURA_ALTA:

            if (valorSensoresControl[PT100_1] < LIMITE_SUP_TEMPERATURA && valorSensoresControl[PT100_1] != VALOR_ERROR_SENSOR){
                causaAlarma = NO_FALLA;
                estadoControl = ENCENDIDO_IDLE;

                solicitarDesactivarAlarma();
                solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
                solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);

            }
            break; 

        case TEMPERATURA_BAJA:
            if (valorSensoresControl[PT100_1] > LIMITE_INF_TEMPERATURA && valorSensoresControl[PT100_1] != VALOR_ERROR_SENSOR){
                causaAlarma = NO_FALLA;
                estadoControl = ENCENDIDO_IDLE;
            
                solicitarDesactivarAlarma();
                // Como estan todas las resistencias prendidas, si no hay ningun modo prendido las apago
                if (configuracionesControl->calefaccionOn == false && acsOn == false){
                    for (int i=0; i < CANT_RESISTENCIAS; i++){
                        solicitarDesactivarRele(i);
                    }
                }
                solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
                solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
                
            }
            break;
    }
    
}

void verificarFalla(){
    static int contadorFallaTempAlta = 0, contadorFallaTempBaja = 0, contadorFallaPresionAlta = 0, contadorFallaPresionBaja = 0;
    
    // Termostato de seguridad
    if (estadoEntradasControl[TERMOSTATO_SEG] == HIGH
        && obtenerPrioridadAlarma(TERMOSTATO_SEGURIDAD) < obtenerPrioridadAlarma(causaAlarma)){

        causaAlarma = TERMOSTATO_SEGURIDAD;
        guardarAlarmaNVS((int32_t) causaAlarma);

        // ENCIENDO LA ALARMA
        estadoControl = ALARMA;
        solicitarActivarAlarma();
        
        // Apago las resistencias
        for (int i=0; i < CANT_RESISTENCIAS_TOTALES; i++){
            solicitarDesactivarRele(i);
        }

        // Prendo la bomba de calefaccion intentar bajar la temperatura
        solicitarActivarRele(ETAPA_5, true, TIEMPO_BOMBA_CAL_MS);
        solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
    }

    // Sobretemperatura se da sensor N°1
    if (valorSensoresControl[PT100_1] > LIMITE_SUP_TEMPERATURA 
        && valorSensoresControl[PT100_1] != VALOR_ERROR_SENSOR){

        contadorFallaTempAlta++;

        if (contadorFallaTempAlta > CANT_MUESTRAS_FALLA && obtenerPrioridadAlarma(TEMPERATURA_ALTA) < obtenerPrioridadAlarma(causaAlarma)){
            causaAlarma = TEMPERATURA_ALTA; 
            guardarAlarmaNVS((int32_t) causaAlarma);

            // Encienddo la alarma
            estadoControl = ALARMA;
            solicitarActivarAlarma();

            // APAGO RESISTENCIAS
            for (int i=0; i < CANT_RESISTENCIAS_TOTALES; i++){
                solicitarDesactivarRele(i);
            }

            // Prendo la bomba de calefaccion intentar bajar la temperatura
            solicitarActivarRele(ETAPA_5, true, TIEMPO_BOMBA_CAL_MS);
        solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
            
        }
    } else {
        contadorFallaTempAlta = 0;
    }

    // Baja temperatura
    if (valorSensoresControl[PT100_1] < LIMITE_INF_TEMPERATURA 
        && valorSensoresControl[PT100_1] != VALOR_ERROR_SENSOR){

        contadorFallaTempBaja++;

        if (contadorFallaTempBaja > CANT_MUESTRAS_FALLA && obtenerPrioridadAlarma(TEMPERATURA_BAJA) < obtenerPrioridadAlarma(causaAlarma)){
            causaAlarma = TEMPERATURA_BAJA; 
            guardarAlarmaNVS((int32_t) causaAlarma);

            // Enciendo la alarma
            estadoControl = ALARMA;
            solicitarActivarAlarma();
            
            // PRENDO RESISTENCIAS Y BOMBA RESISTENCIAS
            for (int i=0; i < CANT_RESISTENCIAS; i++){
                solicitarActivarRele(i);
            }

            //Prendo las bombas para que no se congele el agua 
            solicitarActivarRele(ETAPA_5, true, TIEMPO_BOMBA_ACS_MS);
            solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
            
        }
    } else {
        contadorFallaTempBaja = 0;
    }

    // Apagado de bombas, si hay flujo pero estan ambas apagadas
    if (estadoEntradasControl[FLOW_SW_BOMBAS] == LOW && tiempoEstadoEntradasControl[FLOW_SW_BOMBAS] >= TIEMPO_APAGADO_BOMBAS_MS
        &&  etapasControl[ETAPA_5] == LOW && tiempoApagadoEtapasControl[ETAPA_5] >= TIEMPO_APAGADO_BOMBAS_MS
        &&  etapasControl[ETAPA_6] == LOW && tiempoApagadoEtapasControl[ETAPA_6] >= TIEMPO_APAGADO_BOMBAS_MS){
        
        if (obtenerPrioridadAlarma(BOMBAS_OFF) < obtenerPrioridadAlarma(causaAlarma)){

            causaAlarma = BOMBAS_OFF;
            // Enciendo la alarma
            solicitarActivarAlarma();
            estadoControl = ALARMA;
            // APAGO RESISTENCIAS Y BOMBAS
            for (int i=0; i < CANT_RESISTENCIAS_TOTALES; i++){
                solicitarDesactivarRele(i);
            }
        }

        if (alarmasControl[0] != (int32_t) BOMBAS_OFF && alarmasControl[1] != (int32_t) BOMBAS_OFF){
            guardarAlarmaNVS((int32_t) BOMBAS_OFF);
        }

        desactivarEtapa(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
        desactivarEtapa(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
        
    }

    // Falla de sensores RTD
    if (valorSensoresControl[PT100_1] == VALOR_ERROR_SENSOR){

        // if (causaAlarma == TEMPERATURA_ALTA || causaAlarma == TEMPERATURA_BAJA){
        //     guardarAlarmaNVS((int32_t) causaAlarma);
        //     for (int i=0; i < CANT_RESISTENCIAS_TOTALES; i++){
        //         solicitarDesactivarRele(i);
        //     }
        //     for (int i=0; i < CANT_BOMBAS; i++){
        //         solicitarDesactivarRele(i + CANT_RESISTENCIAS_TOTALES, true, TIEMPO_APAGADO_BOMBAS_MS);
        //     }
        // }

        if (obtenerPrioridadAlarma(FALLA_SENSOR_1) < obtenerPrioridadAlarma(causaAlarma) || causaAlarma == FALLA_SENSOR_2
        || causaAlarma == TEMPERATURA_ALTA || causaAlarma == TEMPERATURA_BAJA){

            causaAlarma = FALLA_SENSOR_1;

            // Apago el sistema de calefaccion
            bool val = false;
            guardarConfigsNVS(CALEFACCION_ON, &val, sizeof(val));
            solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
        }
        
        if (alarmasControl[0] != (int32_t) FALLA_SENSOR_1 && alarmasControl[1] != (int32_t) FALLA_SENSOR_1){
            guardarAlarmaNVS((int32_t) FALLA_SENSOR_1);
        }
    }
    
    if (valorSensoresControl[PT100_2] == VALOR_ERROR_SENSOR){


        if (obtenerPrioridadAlarma(FALLA_SENSOR_2) < obtenerPrioridadAlarma(causaAlarma) || (causaAlarma == FALLA_SENSOR_1 && alarmasControl[0]!= (int32_t) FALLA_SENSOR_1)){
        
            // Si tengo como causa de alarma el sensor 1, guardo la falla pero no cambio la causa
            if (causaAlarma != FALLA_SENSOR_1){
                causaAlarma = FALLA_SENSOR_2;
            }            
            // Desactivo el circuito de acs y vuevlo a idle
            acsOn = false;
            if (estadoControl == ENCENDIDO_ACS){
                estadoControl = ENCENDIDO_IDLE;
            }
            solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
        }
        if (alarmasControl[0] != (int32_t) FALLA_SENSOR_2 && alarmasControl[1] != (int32_t) FALLA_SENSOR_2){
            guardarAlarmaNVS((int32_t) FALLA_SENSOR_2);
        }
    }

    // Si no hay flujo en ninguna bomba, pero la bomba de calefaccion esta encendida, hay fallo en la bomba de calefaccion 
    // Para ello hay que ver que tanto la bomba este prendida hace mas de TIEMPO_BOMBA_CAL_MS
    // Y el ultimo cambio de la entrada del flow switch sea hace mas de TIEMPO_BOMBA_CAL_MS hacia estado bajo
    if (estadoEntradasControl[FLOW_SW_BOMBAS] == HIGH && tiempoEstadoEntradasControl[FLOW_SW_BOMBAS] >= TIEMPO_BOMBA_CAL_MS
        &&  etapasControl[ETAPA_5] == HIGH && tiempoEncendidoEtapasControl[ETAPA_5] >= TIEMPO_BOMBA_CAL_MS
        && causaAlarma != BOMBA_CALEFACCION_ON){

        // Esto se hace aparte para que en caso de que ya estuviese con alarma, 
        // apague la bomba igual pero no cambie la causa de la alarma si la otra es mas prioritaria
        if (obtenerPrioridadAlarma(BOMBA_CALEFACCION_ON) <= obtenerPrioridadAlarma(causaAlarma)){
            causaAlarma = BOMBA_CALEFACCION_ON;
        }
        
        guardarAlarmaNVS((int32_t) BOMBA_CALEFACCION_ON);

        // Apago el circuito de calefaccion
        bool val = false;
        guardarConfigsNVS(CALEFACCION_ON, &val, sizeof(val));
        desactivarEtapa(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
        
        // Si ya habia fallado la otra bomba, paso al fallo de las dos
        if (causaAlarma == BOMBA_AGUA_CALIENTE_ON){
            causaAlarma = BOMBAS_ON;
            guardarAlarmaNVS((int32_t) causaAlarma);

            // Enciendo la alarma
            solicitarActivarAlarma();
            estadoControl = ALARMA;
        }
    } 

    // Si no hay flujo en ninguna bomba, pero la bomba de agua caliente esta encendida, hay fallo en la bomba de agua caliente 
    // Mismas condiciones que la bomba de calefaccion pero con tiempo TIEMPO_BOMBA_ACS_MS
    if (estadoEntradasControl[FLOW_SW_BOMBAS] == HIGH && tiempoEstadoEntradasControl[FLOW_SW_BOMBAS] >= TIEMPO_BOMBA_ACS_MS
        &&  etapasControl[ETAPA_6] == HIGH && tiempoEncendidoEtapasControl[ETAPA_6] >= TIEMPO_BOMBA_ACS_MS
        && causaAlarma != BOMBA_AGUA_CALIENTE_ON){

        if (obtenerPrioridadAlarma(BOMBA_AGUA_CALIENTE_ON) <= obtenerPrioridadAlarma(causaAlarma)){
            causaAlarma = BOMBA_AGUA_CALIENTE_ON;
        }

        guardarAlarmaNVS((int32_t) BOMBA_AGUA_CALIENTE_ON);
        
        // Apago el sistema de acs
        acsOn = false;
        desactivarEtapa(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);

        // Esto se hace porque si falla cuando estaba en ACS, se vuelve al idle, 
        // pero si fallo porque alguna alarma la encendio, se debe quedar en estado de alarma
        if (estadoControl != ALARMA){
            estadoControl = ENCENDIDO_IDLE;
        }
        
        
        // Si ya habia fallado la otra bomba, paso al fallo de las dos
        if (causaAlarma == BOMBA_CALEFACCION_ON){
            causaAlarma = BOMBAS_ON;
            guardarAlarmaNVS((int32_t) causaAlarma);

            // Enciendo la alarma
            solicitarActivarAlarma();
            estadoControl = ALARMA;
        }

    }

    // Limites de presion con la medicion del manometro
    if (valorSensoresControl[LOOP_CORRIENTE] > LIMITE_SUP_PRESION
        && valorSensoresControl[LOOP_CORRIENTE] != VALOR_ERROR_SENSOR){
        contadorFallaPresionAlta++;
        if (contadorFallaPresionAlta > 2 && obtenerPrioridadAlarma(PRESION_ALTA) < obtenerPrioridadAlarma(causaAlarma)){
            causaAlarma = PRESION_ALTA;
        }
        if (alarmasControl[0] != (int32_t) PRESION_ALTA && alarmasControl[1] != (int32_t) PRESION_ALTA){
            guardarAlarmaNVS((int32_t) PRESION_ALTA);
        }
    } else {
    contadorFallaPresionAlta = 0;
    }

    if (valorSensoresControl[LOOP_CORRIENTE] < LIMITE_INF_PRESION
        && valorSensoresControl[LOOP_CORRIENTE] != VALOR_ERROR_SENSOR){

        contadorFallaPresionBaja++;

        if (contadorFallaPresionBaja > 2 && obtenerPrioridadAlarma(PRESION_BAJA) < obtenerPrioridadAlarma(causaAlarma)){
            causaAlarma = PRESION_BAJA;
        }

        if (alarmasControl[0] != (int32_t) PRESION_BAJA && alarmasControl[1] != (int32_t) PRESION_BAJA){
            guardarAlarmaNVS((int32_t) PRESION_BAJA);
        }

    } else {
    contadorFallaPresionBaja = 0;
    }

    if (valorSensoresControl[LOOP_CORRIENTE] == VALOR_ERROR_SENSOR 
        && obtenerPrioridadAlarma(FALLA_SENSOR_I) < obtenerPrioridadAlarma(causaAlarma)){
        causaAlarma = FALLA_SENSOR_I;
        guardarAlarmaNVS((int32_t) FALLA_SENSOR_I);
    }
}

int obtenerPrioridadAlarma(causaAlarma_t causa){
    switch (causa){
    case NO_FALLA:
        return 6;
    case PRESION_ALTA:
    case PRESION_BAJA:
    case FALLA_SENSOR_I:
        return 5;
    case BOMBA_CALEFACCION_ON:
    case BOMBA_AGUA_CALIENTE_ON:
        return 4;
    case FALLA_SENSOR_1:
    case FALLA_SENSOR_2:
        return 3;
    case BOMBAS_OFF:
    case BOMBAS_ON:
        return 2;
    case TEMPERATURA_ALTA:
    case TEMPERATURA_BAJA:
        return 1;
    case TERMOSTATO_SEGURIDAD:
        return 0;
    default: return 10;
    }
}

void actualizarEtapasSalida(){
    switch (estadoControl){
        // La uso para apagar por unica ve las resistencias si no estan activados ni la calefaccion ni acs
        static bool apagarControl = true;
        
        case APAGADO: break;
        case ENCENDIDO_IDLE:     

            if (configuracionesControl->calefaccionOn){
                // Si el estado acaba de cambiar, no abro la bomba de calefaccion para poder detectar si se quedo pegada la bomba de agua caleinte
                // Si se cierra el termostato ambiente (pide calefaccion) y la bomba de calefaccion esta cerrada, se activa
                // if (estadoControl == estadoControlAnterior){
                if (estadoEntradasControl[TERMOSTATO_AMB] == LOW && etapasControl[ETAPA_5] == LOW){
                    solicitarActivarRele(ETAPA_5, true, TIEMPO_BOMBA_CAL_MS);
                } 
                // }
                // Si se abre el termostato ambiente y la bomba de calefaccion esta circulando, se apaga
                if (estadoEntradasControl[TERMOSTATO_AMB] == HIGH && etapasControl[ETAPA_5] == HIGH && contadorMantenimiento > tiempoDuracionMantenimiento){
                    
                    solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
                }
                
            } else {
                // Si apago la calefaccion manualente hay que apagarla
                if (etapasControl[ETAPA_5] == HIGH){
                    solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
                }
            }
            
            // En el idle si esta prendida la calefaccion o el agua caliente hay que controlar las resistencias
            // Si no esta el circuito de calefaccion ni el de acs apago las resistencias
            
            if ((configuracionesControl->calefaccionOn || acsOn) && causaAlarma != FALLA_SENSOR_1){
                controlarResistencias();
                apagarControl = true;
            } else if(apagarControl){
                apagarControl = false;
                // Apago las resistencias
                for (int i=0; i < CANT_RESISTENCIAS; i++){
                    solicitarDesactivarRele(i);
                }
            }
            
            break;

        case ENCENDIDO_ACS:
            // Espero un ciclo antes de prender la bomba de agua caliente para detectar si se quedo pegado el contactor de la bomba de calefaccion
            // if (estadoControl == estadoControlAnterior && etapasControl[ETAPA_6] == LOW){
            if (etapasControl[ETAPA_6] == LOW){    
                solicitarActivarRele(ETAPA_6, true, TIEMPO_BOMBA_ACS_MS);
            } 
            controlarResistencias();
            apagarControl = true;
            break;

        case ENCENDIDO_SUPERVISION:
            if (supervisionOn == true){
                controlarResistencias();
            }
            
            break;
        case DIAGNOSTICO: 
            if (flagsPurga[0] == true){
                purgarBomba(BOMBA_CAL);
            }
            if (flagsPurga[1] == true){
                purgarBomba(BOMBA_ACS);
            }
            break;
        case ALARMA: 
            switch(causaAlarma){
                case TEMPERATURA_ALTA:
                    static bool fallaTempAlta = false;
                    static int contadorFallaTempAlta = 0;
                    static uint64_t contadorFalla = 0;

                    // Si la temperatura sigue subeindo y supera el limite de emergencia,
                    // Deberia haber saltado el termostato de seguridad
                    // Si no lo hizo, se activa la falla desde el controlador
                    if (valorSensoresControl[PT100_1] > TEMPERATURA_EMERGENCIA && fallaTempAlta == false){
                        contadorFallaTempAlta++;
                        if (contadorFallaTempAlta == 2){
                            contarMs(&contadorFalla, TIEMPO_SEG_EMERGENCIA, LOW);
                            forzarFalla();
                            fallaTempAlta = true;
                        }
                    } else {
                        contadorFallaTempAlta = 0;
                    }
                    if (contadorFalla == 0 && fallaTempAlta){
                        fallaTempAlta = false;
                        cortarFalla();
                    }
                    break;
            }
            break;
    }

    actualizarControlesAlarma();
}

void controlarResistencias(){
    
    static float temperaturaMedida, temperaturaControl, histeresisControl;
    static uint64_t esperaEncendidoResistencia, esperaApagadoResistencia;
    // Segun el estado de la caldera controlo con el sensor 1 o 2 y con los parametros configurados para ese sensor
    switch (estadoControl){
        case ENCENDIDO_IDLE:
            temperaturaMedida = valorSensoresControl[PT100_1];
            histeresisControl = configuracionesControl->histeresisCalefaccion;
            if (configuracionesControl->calefaccionOn){
                temperaturaControl = configuracionesControl->temperaturaCalefaccion; 
            } else {
                temperaturaControl = configuracionesControl->temperaturaACS;
            }
            break;

        case ENCENDIDO_ACS:
        case ENCENDIDO_SUPERVISION:
            temperaturaMedida = valorSensoresControl[PT100_2];
            temperaturaControl = configuracionesControl->temperaturaACS;
            histeresisControl = configuracionesControl->histeresisACS;
            break;
        
    }
    
    // APAGADO DE RESISTENCIAS
    // Si la etapa 1 esta prendida es que seguro hay alguna prendida
    if (etapasControl[relesDeResistencias[0]] == HIGH){
        // Me fijo si debo apagar alguna
        for (int i=0;i<CANT_RESISTENCIAS;i++){
            // Divido la histeresis segun la cantidad de etapas 
            if (temperaturaMedida >= temperaturaControl - histeresisControl / (CANT_RESISTENCIAS-1) * (CANT_RESISTENCIAS-1-i)){
                // Si la etapa esta prendida la apago
                if (etapasControl[CANT_RESISTENCIAS-1-i] == HIGH && esperaApagadoResistencia == 0){
                    solicitarDesactivarRele(relesDeResistencias[CANT_RESISTENCIAS-1-i]);
                    contarMs(&esperaApagadoResistencia, configuracionesControl->retardoApagadoMs, LOW);
                }
            } 
            // Como se va comparando contra temperaturas cada vez mas altas, si no entro en una, seguro no entrara en la siguiente
            else {
                break;
            }
        }
    }

    // ENCENDIDO DE RESISTENCIAS
    // Sigue la logica inversa a la de apagado pero tiene el agregado de los retardos (no se prenden dos seguidas)
    // Si la ultima etapa esta apagada es que podria prenderse alguna
    if (etapasControl[relesDeResistencias[CANT_RESISTENCIAS]] == LOW ){
        // Me fijo si debo prender alguna
        for (int i=0;i<CANT_RESISTENCIAS;i++){
            // Divido la histeresis segun la cantidad de etapas 
            if (temperaturaMedida < temperaturaControl - histeresisControl / (CANT_RESISTENCIAS-1) * (i+1)){
                // Si la etapa esta apagada y ademas el contador me permite, la enciendo y reinicio el contador
                if (etapasControl[i] == LOW && esperaEncendidoResistencia == 0){
                    solicitarActivarRele(relesDeResistencias[i]);
                    contarMs(&esperaEncendidoResistencia, configuracionesControl->retardoEncendidoMs, LOW);
                }
            } 
            // Como se va comparando contra temperaturas cada vez mas altas, si no entro en una, seguro no entrara en la siguiente
            else {
                break;
            }
        }
    }        
}

void actualizarControlesAlarma(){
}

void actualizarMantenimiento(){
  if (contadorMantenimiento  == 0){
    contarMs(&contadorMantenimiento, tiempoEsperaMantenimiento, LOW);
    if (estadoControl != ENCENDIDO_IDLE || configuracionesControl->calefaccionOn == false){
        solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
    }
    if (estadoControl != ENCENDIDO_ACS){
        solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
    }    
  } else if (contadorMantenimiento  < tiempoDuracionMantenimiento){
    solicitarActivarRele(ETAPA_5, true, TIEMPO_BOMBA_CAL_MS);
    solicitarActivarRele(ETAPA_6, true, TIEMPO_BOMBA_ACS_MS);
  }
}

void purgarBomba(int bomba){
    static uint64_t contadorEspera[2] = {0,0};
    if (bomba != BOMBA_CAL && bomba != BOMBA_ACS){
        return;
    }

    if (bomba == BOMBA_CAL){
        contador = 0;
    } else {
        contador = 1;
    }

    // Si la bomba esta apagada y el contador esta en 0, activo la bomba durante TIEMPO_PURGA_ON con apagado automatico,
    // e inicio un contador en el tiempo de encendido mas apagado
    if (etapasControl[bomba] == LOW && contadorEspera[contador] == 0){
        solicitarActivarRele(bomba,true,TIEMPO_PURGA_ON,true);
        contarMs(&contadorEspera[contador],TIEMPO_PURGA_OFF + TIEMPO_PURGA_ON, LOW);
    }
}

// FUNCIONES AUXILIARES PARA TESTS

#ifdef TESTING

void mockingSensores(){
    valorSensoresControl[PT100_1] = 25;
    valorSensoresControl[PT100_2] = 25;
    valorSensoresControl[LOOP_CORRIENTE] = 15;
}

void mockingEntradasMecanicas(){
    estadoEntradasControl[TERMOSTATO_SEG] = LOW;
    estadoEntradasControl[TERMOSTATO_AMB] = HIGH;
    estadoEntradasControl[FLOW_SW_BOMBAS] = HIGH;
    estadoEntradasControl[FLOW_SW_ACS] = HIGH;
}

void test_resistenciasApagadas(){
    for (int i = 0; i < CANT_RESISTENCIAS_TOTALES; i++) {
        char mensaje[50];
        snprintf(mensaje, sizeof(mensaje), "La resistencia %d no se apago", i);
        TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[relesDeResistencias[i]]), mensaje);
    }
}

void test_fallaTermostatoSeguridad_forzarFalla(){
    // FUERZO LA FALLA DEL TERMOSTATO DE SEGURIDAD
    estadoEntradasControl[TERMOSTATO_SEG] = HIGH;
    // HAGO RESPONDER A LA FUNCIÓN
    verificarFalla();
    actualizarReles();
    actualizarBuzzer();
}

void test_fallaTermostatoSeguridad_verificarTest(){
    // VERIFICO QUE LA GESTION HAYA SIDO CORRECTA
    TEST_ASSERT_EQUAL_MESSAGE(ALARMA, estadoControl, "TS: El estado no es correcto");
    TEST_ASSERT_EQUAL_MESSAGE(TERMOSTATO_SEGURIDAD, causaAlarma, "TS: La causa de alarma no es la correcta");
    TEST_ASSERT_EQUAL_MESSAGE((int32_t) TERMOSTATO_SEGURIDAD, alarmasControl[0], "TS: La alarma no fue registrada");
    test_resistenciasApagadas();
    if (alarmasControl[1] == (int32_t) BOMBAS_OFF || alarmasControl[1] == (int32_t) BOMBAS_ON || alarmasControl[1] == (int32_t) BOMBA_CALEFACCION_ON){
        TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_5]), "TS: La bomba de calefacción no debio prenderse");
    } else {
        TEST_ASSERT_EQUAL_MESSAGE(LOW, digitalRead(pinesReles[ETAPA_5]), "TS: La bomba de calefacción no se prendió");
    }
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_6]), "TS: La bomba de agua caliente no se apagó");
}

void test_fallaTemperaturaAlta_forzarFalla(){
    // FUERZO LA FALLA DEL TERMOSTATO DE SEGURIDAD
    valorSensoresControl[PT100_1] = LIMITE_SUP_TEMPERATURA + 1;
    // HAGO RESPONDER A LA FUNCIÓN (3 veces para que detecte la falla)
    for (int i = 0; i <= CANT_MUESTRAS_FALLA; i++){
        verificarFalla();
        actualizarReles();
        actualizarBuzzer();
    }
}

void test_fallaTemperaturaAlta_verificarTest(){
    // VERIFICO QUE LA GESTIÓN HAYA SIDO CORRECTA
    TEST_ASSERT_EQUAL_MESSAGE(ALARMA, estadoControl, "TA: El estado no es correcto");
    if (alarmasControl[0] != (int32_t) TERMOSTATO_SEGURIDAD){
        TEST_ASSERT_EQUAL_MESSAGE(TEMPERATURA_ALTA, causaAlarma, "TA: La causa de alarma no es la correcta");
        TEST_ASSERT_EQUAL_MESSAGE((int32_t) TEMPERATURA_ALTA, alarmasControl[0], "TA: La alarma no fue registrada");
    }
    test_resistenciasApagadas();
    if (alarmasControl[1] == (int32_t) BOMBAS_OFF || alarmasControl[1] == (int32_t) BOMBAS_ON || alarmasControl[1] == (int32_t) BOMBA_CALEFACCION_ON){
        TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_5]), "TA: La bomba de calefacción no debio prenderse");
    } else {
        TEST_ASSERT_EQUAL_MESSAGE(LOW, digitalRead(pinesReles[ETAPA_5]), "TA: La bomba de calefacción no se prendió");
    }
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_6]), "TA: La bomba de agua caliente no se apagó");
}

void test_fallaTemperaturaBaja_forzarFalla(){
    // FUERZO LA FALLA DEL TERMOSTATO DE SEGURIDAD
    valorSensoresControl[PT100_1] = LIMITE_INF_TEMPERATURA - 1;
    // HAGO RESPONDER A LA FUNCIÓN (3 veces para que detecte la falla)
    for (int i = 0; i <= CANT_MUESTRAS_FALLA; i++){
        verificarFalla();
        actualizarReles();
        actualizarBuzzer();
    }
}

void test_fallaTemperaturaBaja_verificarTest(){
    // VERIFICO QUE LA GESTIÓN HAYA SIDO CORRECTA
    TEST_ASSERT_EQUAL_MESSAGE(ALARMA, estadoControl, "TB: El estado no es correcto");
    TEST_ASSERT_EQUAL_MESSAGE(TEMPERATURA_BAJA, causaAlarma, "TB: La causa de alarma no es la correcta");
    TEST_ASSERT_EQUAL_MESSAGE((int32_t) TEMPERATURA_BAJA, alarmasControl[0], "TB: La alarma no fue registrada");
    for (int i = 0; i < CANT_RESISTENCIAS; i++){
        char mensaje[50];
        snprintf(mensaje, sizeof(mensaje), "TB: La resistencia %d no se encendió", i);
        TEST_ASSERT_EQUAL_MESSAGE(LOW, digitalRead(pinesReles[relesDeResistencias[i]]), mensaje);
    }
    if (alarmasControl[1] == (int32_t) BOMBAS_OFF || alarmasControl[1] == (int32_t) BOMBAS_ON || alarmasControl[1] == (int32_t) BOMBA_CALEFACCION_ON){
        TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_5]), "TB: La bomba de calefacción no debio prenderse");
    } else {
        TEST_ASSERT_EQUAL_MESSAGE(LOW, digitalRead(pinesReles[ETAPA_5]), "TB: La bomba de calefacción no se prendió");
    }
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_6]), "TB: La bomba de agua caliente no se apagó");
}

void test_fallaApagadoBombas_forzarFalla(){
    // FUERZO LA FALLA
    solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
    solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
    actualizarReles();
    tiempoApagadoEtapasControl[ETAPA_5] = TIEMPO_APAGADO_BOMBAS_MS;
    tiempoApagadoEtapasControl[ETAPA_6] = TIEMPO_APAGADO_BOMBAS_MS;
    estadoEntradasControl[FLOW_SW_BOMBAS] = LOW;
    tiempoEstadoEntradasControl[FLOW_SW_BOMBAS] = TIEMPO_APAGADO_BOMBAS_MS;
    verificarFalla();
    actualizarReles();
    actualizarBuzzer();
}

void test_fallaApagadoBombas_verificarTest(){
    // VERIFICO QUE LA GESTIÓN HAYA SIDO CORRECTA
    TEST_ASSERT_EQUAL_MESSAGE(ALARMA, estadoControl, "AB: El estado no es correcto");
    if (obtenerPrioridadAlarma(BOMBAS_OFF) < obtenerPrioridadAlarma(causaAlarma)){
        TEST_ASSERT_EQUAL_MESSAGE(BOMBAS_OFF, causaAlarma, "AB: La causa de alarma no es la correcta");
    }
    TEST_ASSERT_EQUAL_MESSAGE((int32_t) BOMBAS_OFF, alarmasControl[0], "AB: La alarma no fue registrada");
    if (alarmasControl[1] != TEMPERATURA_BAJA){
        test_resistenciasApagadas();
    }
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_5]), "AB: La bomba de calefacción no se apagó");
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_6]), "AB: La bomba de agua caliente no se apagó");
}

void test_fallaSensorCaldera_forzarFalla(){
    // FUERZO LA FALLA DE SENSOR DE CALDERA
    valorSensoresControl[PT100_1] = VALOR_ERROR_SENSOR;
    verificarFalla();
    actualizarReles();
    actualizarBuzzer();
}

void test_fallaSensorCaldera_verificarTest(){
    // VERIFICO QUE LA GESTIÓN HAYA SIDO CORRECTA
    if (obtenerPrioridadAlarma(FALLA_SENSOR_1) < obtenerPrioridadAlarma(causaAlarma)){
        TEST_ASSERT_EQUAL_MESSAGE(FALLA_SENSOR_1, causaAlarma, "S1: La causa de alarma no es la correcta");
        TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_5]), "S1: La bomba de calefacción no se apagó");
        TEST_ASSERT_EQUAL_MESSAGE(false, configuracionesControl->calefaccionOn, "S1: No se apagó la calefacción");
    }
    TEST_ASSERT_EQUAL_MESSAGE((int32_t) FALLA_SENSOR_1, alarmasControl[0], "S1: La alarma no fue registrada");
}

void test_fallaSensorACS_forzarFalla(){
    // FUERZO LA FALLA DE SENSOR DE ACS
    valorSensoresControl[PT100_2] = VALOR_ERROR_SENSOR;
    verificarFalla();
    actualizarReles();
    actualizarBuzzer();
}

void test_fallaSensorACS_verificarTest(){
    // VERIFICO QUE LA GESTIÓN HAYA SIDO CORRECTA
    if (obtenerPrioridadAlarma(FALLA_SENSOR_2) < obtenerPrioridadAlarma(causaAlarma)){
        TEST_ASSERT_EQUAL_MESSAGE(FALLA_SENSOR_2, causaAlarma, "S2: La causa de alarma no es la correcta");
        TEST_ASSERT_EQUAL_MESSAGE(false, acsOn, "S2: No se apagó el agua caliente");
        TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_6]), "S2: La bomba de agua caliente no se apagó");
    }
    TEST_ASSERT_EQUAL_MESSAGE((int32_t) FALLA_SENSOR_2, alarmasControl[0], "S2: La alarma no fue registrada");
}

void test_fallaEncendidoBombaCalefaccion_forzarFalla(){
    // FUERZO LA FALLA DE LA BOMBA DE CALEFACCION
    solicitarActivarRele(ETAPA_5, true, TIEMPO_BOMBA_CAL_MS);
    actualizarReles();
    tiempoEncendidoEtapasControl[ETAPA_5] = TIEMPO_BOMBA_CAL_MS;
    estadoEntradasControl[FLOW_SW_BOMBAS] = HIGH;
    tiempoEstadoEntradasControl[FLOW_SW_BOMBAS] = TIEMPO_BOMBA_CAL_MS;
    verificarFalla();
    actualizarReles();
    actualizarBuzzer();
}

void test_fallaEncendidoBombaCalefaccion_verificarTest(){
    // VERIFICO QUE LA GESTIÓN HAYA SIDO CORRECTA
    if (obtenerPrioridadAlarma(BOMBA_CALEFACCION_ON) < obtenerPrioridadAlarma(causaAlarma)){
        TEST_ASSERT_EQUAL_MESSAGE(BOMBA_CALEFACCION_ON, causaAlarma, "BC: La causa de alarma no es la correcta");
    }
    TEST_ASSERT_EQUAL_MESSAGE((int32_t) BOMBA_CALEFACCION_ON, alarmasControl[0], "BC: La alarma no fue registrada");
    TEST_ASSERT_EQUAL_MESSAGE(false, configuracionesControl->calefaccionOn, "BC: No se apagó la calefacción");
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_5]), "BC: La bomba de calefacción no se apagó");
}

void test_fallaEncendidoBombaACS_forzarFalla(){
    // FUERZO LA FALLA DE LA BOMBA DE ACS
    solicitarActivarRele(ETAPA_6, true, TIEMPO_BOMBA_ACS_MS);
    actualizarReles();
    tiempoEncendidoEtapasControl[ETAPA_6] = TIEMPO_BOMBA_ACS_MS;
    estadoEntradasControl[FLOW_SW_BOMBAS] = HIGH;
    tiempoEstadoEntradasControl[FLOW_SW_BOMBAS] = TIEMPO_BOMBA_ACS_MS;
    verificarFalla();
    actualizarReles();
    actualizarBuzzer();
}

void test_fallaEncendidoBombaACS_verificarTest(){
    // VERIFICO QUE LA GESTIÓN HAYA SIDO CORRECTA
    if (obtenerPrioridadAlarma(BOMBA_AGUA_CALIENTE_ON) < obtenerPrioridadAlarma(causaAlarma)){
        TEST_ASSERT_EQUAL_MESSAGE(BOMBA_AGUA_CALIENTE_ON, causaAlarma, "BA: La causa de alarma no es la correcta");
    }
    TEST_ASSERT_EQUAL_MESSAGE((int32_t) BOMBA_AGUA_CALIENTE_ON, alarmasControl[0], "BA: La alarma no fue registrada");
    TEST_ASSERT_EQUAL_MESSAGE(false, acsOn, "BA: No se apagó el agua caliente");
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(pinesReles[ETAPA_6]), "BA: La bomba de agua caliente no se apagó");
}

void test_fallaPresionAlta_forzarFalla(){
    // FUERZO LA FALLA DE PRESION ALTA
    valorSensoresControl[LOOP_CORRIENTE] = LIMITE_SUP_PRESION + 1;
    for (int i=0; i <= CANT_MUESTRAS_FALLA; i++){
        verificarFalla();
        actualizarReles();
        actualizarBuzzer();
    }
}

void test_fallaPresionAlta_verificarTest(){
    // VERIFICO QUE LA GESTIÓN HAYA SIDO CORRECTA
    if (obtenerPrioridadAlarma(PRESION_ALTA) < obtenerPrioridadAlarma(causaAlarma)){
        TEST_ASSERT_EQUAL_MESSAGE(PRESION_ALTA, causaAlarma, "PA: La causa de alarma no es la correcta");
    }
    TEST_ASSERT_EQUAL_MESSAGE((int32_t) PRESION_ALTA, alarmasControl[0], "PA: La alarma no fue registrada");
}

void test_fallaPresionBaja_forzarFalla(){
    // FUERZO LA FALLA DE PRESION BAJA
    valorSensoresControl[LOOP_CORRIENTE] = LIMITE_INF_PRESION - 1;
    for (int i=0; i <= CANT_MUESTRAS_FALLA; i++){
        verificarFalla();
        actualizarReles();
        actualizarBuzzer();
    }
}

void test_fallaPresionBaja_verificarTest(){
    // VERIFICO QUE LA GESTIÓN HAYA SIDO CORRECTA
    if (obtenerPrioridadAlarma(PRESION_BAJA) < obtenerPrioridadAlarma(causaAlarma)){
        TEST_ASSERT_EQUAL_MESSAGE(PRESION_BAJA, causaAlarma, "PB: La causa de alarma no es la correcta");
    }
    TEST_ASSERT_EQUAL_MESSAGE((int32_t) PRESION_BAJA, alarmasControl[0], "PB: La alarma no fue registrada");
}

void test_gestionFallas(void (*forzarFalla)(), void (*testearRespuesta)(), const bool* tests){
    static const bool defaultTest[] = {true, true, true, true, true};

    // Si no se pasa el parámetro, usar el arreglo por defecto.
    if (tests == nullptr) {
        tests = defaultTest;
    }

    if (tests[0]){
        // delay(1000);
        estadoControl = APAGADO;
        forzarFalla();
        // delay(1000);
        RUN_TEST(testearRespuesta);
    }

    // delay(1000);
    if (tests[1]){
        estadoControl = ENCENDIDO_IDLE;
        estadoEntradasControl[TERMOSTATO_AMB] = LOW;
        actualizarControles();
        actualizarReles();
        estadoEntradasControl[FLOW_SW_BOMBAS] = LOW;
        // delay(1000);
        forzarFalla();
        // delay(1000);
        RUN_TEST(testearRespuesta);
    }
    
    
    // delay(1000);
    if (tests[2]){
        estadoControl = ENCENDIDO_IDLE;
        bool calOn = false;
        guardarConfigsNVS(CALEFACCION_ON, &calOn, sizeof(calOn));
        // delay(1000);
        forzarFalla();
        // delay(1000);
        RUN_TEST(testearRespuesta);
    }
    
    
    // delay(1000);
    if (tests[3]){
        estadoControl = ENCENDIDO_ACS;
        solicitarActivarRele(ETAPA_6, true, TIEMPO_BOMBA_ACS_MS);
        actualizarControles();
        actualizarReles();
        estadoEntradasControl[FLOW_SW_BOMBAS] = LOW;
        // delay(1000);
        forzarFalla();
        // delay(1000);
        RUN_TEST(testearRespuesta);
    }
    
    
    // delay(1000);
    if (tests[4]){
        estadoControl = DIAGNOSTICO;
        // delay(1000);
        forzarFalla();
        // delay(1000);
        RUN_TEST(testearRespuesta);
    }
    // delay(1000);
    
}

void test_superposicionFallas(void (*fallaPrimera)(), const bool* tests){
    static const bool defaultTest[] = {true, true, true, true, true, true, true, true, true, true};

    // Si no se pasa el parámetro, usar el arreglo por defecto.
    if (tests == nullptr) {
        tests = defaultTest;
    }

    causaAlarma_t primeraCausa;

    // delay(1000);
    if(tests[0]){
        fallaPrimera();
        // delay(1000);
        test_fallaTermostatoSeguridad_forzarFalla();
        // delay(1000);
        RUN_TEST(test_fallaTermostatoSeguridad_verificarTest);
    }

    // delay(1000);
    if(tests[1]){
        fallaPrimera();
        // delay(1000);
        test_fallaTemperaturaAlta_forzarFalla();
        // delay(1000);
        RUN_TEST(test_fallaTemperaturaAlta_verificarTest);
    }

    // delay(1000);
    if(tests[2]){
        fallaPrimera();
        // delay(1000);
        test_fallaTemperaturaBaja_forzarFalla();
        // delay(1000);
        RUN_TEST(test_fallaTemperaturaBaja_verificarTest);
    }
    
    // delay(1000);
    if(tests[3]){
        fallaPrimera();
        // delay(1000);
        test_fallaApagadoBombas_forzarFalla();
        // delay(1000);
        RUN_TEST(test_fallaApagadoBombas_verificarTest);
    }
    
    // delay(1000);
    if(tests[4]){
        fallaPrimera();
        // delay(1000);
        test_fallaSensorCaldera_forzarFalla();
        // delay(1000);
        RUN_TEST(test_fallaSensorCaldera_verificarTest);
    }
    
    // delay(1000);
    if(tests[5]){
        fallaPrimera();
        // delay(1000);
        test_fallaSensorACS_forzarFalla();
        // delay(1000);
        RUN_TEST(test_fallaSensorACS_verificarTest);
    }
    
    // delay(1000);
    if(tests[6]){
        fallaPrimera();
        // delay(1000);
        test_fallaEncendidoBombaCalefaccion_forzarFalla();
        // delay(1000);
        RUN_TEST(test_fallaEncendidoBombaCalefaccion_verificarTest);
    }
    
    // delay(1000);
    if(tests[7]){
        fallaPrimera();
        // delay(1000);
        test_fallaEncendidoBombaACS_forzarFalla();
        // delay(1000);
        RUN_TEST(test_fallaEncendidoBombaACS_verificarTest);
    }
    
    // delay(1000);
    if(tests[8]){
        fallaPrimera();
        // delay(1000);
        test_fallaPresionAlta_forzarFalla();
        // delay(1000);
        RUN_TEST(test_fallaPresionAlta_verificarTest);
    }
    
    // delay(1000);
    if(tests[9]){
        fallaPrimera();
        // delay(1000);
        test_fallaPresionBaja_forzarFalla();
        // delay(1000);
        RUN_TEST(test_fallaPresionBaja_verificarTest);
    }  
}

#endif