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
#define LIMITE_SUP_PRESION      25
#define LIMITE_INF_TEMPERATURA  5
#define LIMITE_SUP_TEMPERATURA  90
#define CANT_RESISTENCIAS       2

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

uint64_t tiempoDuracionMantenimiento = 5000;//5000;
uint64_t tiempoEsperaMantenimiento = 2592000000;

//---------------------------------------------------

estadoControl_t estadoControl;
// estadoControl_t estadoControlAnterior;
causaAlarma_t causaAlarma;

// Punteros a variables del modulo de Entradas Mecanicas
const int *estadoEntradasControl;
const uint64_t *tiempoEstadoEntradasControl;

// Punteros a variables del modulo de Reles
const bool *etapasControl;
const uint64_t *tiempoEncendidoEtapasControl;
const uint64_t *tiempoApagadoEtapasControl;

// Punteros a variables del modulo de Sensores
const float *valorSensoresControl;

const Configs *configuracionesControl;

uint64_t contadorMantenimiento = 0;
uint64_t contadorFalla = 0;
int relesDeResistencias[CANT_RESISTENCIAS] = {ETAPA_1, ETAPA_2, ETAPA_3, ETAPA_4};

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
            static int contadorFallaTempAlta = 0;

            if (valorSensoresControl[PT100_1] > LIMITE_SUP_TEMPERATURA){
                contadorFallaTempAlta++;
                if (contadorFallaTempAlta > 2){
                    causaAlarma = TERMOSTATO_SEGURIDAD;
                    estadoControl = ALARMA;

                    contarMs(&contadorFalla, 1000, LOW);
                    for (int i=0; i < CANT_RESISTENCIAS; i++){
                        solicitarDesactivarRele(i);
                    }
                    solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
                    solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
                }
            }

            // Si se pide ACS se controlan las resisitencias
            if (estadoEntradasControl[FLOW_SW_ACS] == LOW && supervisionOn == false){
                supervisionOn = true;
            } 

            // Si se deja de pedir ACS se apagan las resistencias
            else if (estadoEntradasControl[FLOW_SW_ACS] == HIGH && supervisionOn == true){
                supervisionOn = false;
                for (int i=0; i < CANT_RESISTENCIAS; i++){
                    solicitarDesactivarRele(i);
                }

            }

            break;
            
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

            if (valorSensoresControl[LOOP_CORRIENTE] < LIMITE_SUP_PRESION){
                causaAlarma = NO_FALLA;
            }
            break;

        case PRESION_BAJA:

            if (valorSensoresControl[LOOP_CORRIENTE] > LIMITE_INF_PRESION){
                causaAlarma = NO_FALLA;
            }
            break;

        case TEMPERATURA_ALTA:

            if (valorSensoresControl[PT100_1] < LIMITE_SUP_TEMPERATURA){
                causaAlarma = NO_FALLA;
                estadoControl = ENCENDIDO_IDLE;

                solicitarDesactivarAlarma();
                solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
                solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);

            }
            break; 

        case TEMPERATURA_BAJA:
            if (valorSensoresControl[PT100_1] > LIMITE_INF_TEMPERATURA){
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
        for (int i=0; i < CANT_RESISTENCIAS; i++){
            solicitarDesactivarRele(i);
        }

        // Prendo ambas bombas con apagado automatico para intentar bajar la temperatura
        solicitarActivarRele(ETAPA_5, true, TIEMPO_BOMBAS_EMERGENCIA, true);
        solicitarActivarRele(ETAPA_6, true, TIEMPO_BOMBAS_EMERGENCIA, true);
    }

    // Sobretemperatura se da sensor N°1
    if (valorSensoresControl[PT100_1] > LIMITE_SUP_TEMPERATURA && valorSensoresControl[PT100_1] != VALOR_ERROR_SENSOR){

        contadorFallaTempAlta++;

        if (contadorFallaTempAlta > 3 && obtenerPrioridadAlarma(TEMPERATURA_ALTA) < obtenerPrioridadAlarma(causaAlarma)){
            causaAlarma = TEMPERATURA_ALTA; 
            guardarAlarmaNVS((int32_t) causaAlarma);

            // Encienddo la alarma
            estadoControl = ALARMA;
            solicitarActivarAlarma();

            // APAGO RESISTENCIAS
            for (int i=0; i < CANT_RESISTENCIAS; i++){
                solicitarDesactivarRele(i);
            }

            // Prendo ambas bombas para intentar bajar la temperatura
            solicitarActivarRele(ETAPA_5, true, TIEMPO_BOMBAS_EMERGENCIA, true);
            solicitarActivarRele(ETAPA_6, true, TIEMPO_BOMBAS_EMERGENCIA, true);
            
        }
    } else {
        contadorFallaTempAlta = 0;
    }

    // Baja temperatura
    if (valorSensoresControl[PT100_1] < LIMITE_INF_TEMPERATURA && valorSensoresControl[PT100_1] != VALOR_ERROR_SENSOR){

        contadorFallaTempBaja++;

        if (contadorFallaTempBaja > 3 && obtenerPrioridadAlarma(TEMPERATURA_BAJA) < obtenerPrioridadAlarma(causaAlarma)){
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
            solicitarActivarRele(ETAPA_5, true, TIEMPO_BOMBA_CAL_MS);
            solicitarActivarRele(ETAPA_6, true, TIEMPO_BOMBA_ACS_MS);
            
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
        }

        guardarAlarmaNVS((int32_t) BOMBAS_OFF);
        
        // Enciendo la alarma
        solicitarActivarAlarma();
        estadoControl = ALARMA;

        // APAGO RESISTENCIAS Y BOMBAS
        for (int i=0; i < CANT_RESISTENCIAS; i++){
            solicitarDesactivarRele(i);
        }
        desactivarEtapa(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
        desactivarEtapa(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
        
    }

    // Falla de sensores RTD
    if (valorSensoresControl[PT100_1] == VALOR_ERROR_SENSOR
        && (obtenerPrioridadAlarma(FALLA_SENSOR_1) < obtenerPrioridadAlarma(causaAlarma) || causaAlarma == FALLA_SENSOR_2)){

        causaAlarma = FALLA_SENSOR_1;
        guardarAlarmaNVS((int32_t) causaAlarma);

        // Apago el sistema de calefaccion
        bool val = false;
        guardarConfigsNVS(CALEFACCION_ON, &val, sizeof(val));
        solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
    }
    
    if (valorSensoresControl[PT100_2] == VALOR_ERROR_SENSOR
        && (obtenerPrioridadAlarma(FALLA_SENSOR_2) < obtenerPrioridadAlarma(causaAlarma) || causaAlarma == FALLA_SENSOR_1)){
        
        // Si tengo como causa de alarma el sensor 1, guardo la falla pero no cambio la causa
        if (causaAlarma != FALLA_SENSOR_1){
            causaAlarma = FALLA_SENSOR_2;
        } 
        
        guardarAlarmaNVS((int32_t) FALLA_SENSOR_2);
        
        // Desactivo el circuito de acs y vuevlo a idle
        acsOn = false;
        if (estadoControl == ENCENDIDO_ACS){
            estadoControl = ENCENDIDO_IDLE;
        }
        solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
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
    if (valorSensoresControl[LOOP_CORRIENTE] > LIMITE_SUP_PRESION){
        contadorFallaPresionAlta++;
        if (contadorFallaPresionAlta > 2 && obtenerPrioridadAlarma(PRESION_ALTA) < obtenerPrioridadAlarma(causaAlarma)){
            causaAlarma = PRESION_ALTA;
            guardarAlarmaNVS((int32_t) causaAlarma);
        }
        } else {
        contadorFallaPresionAlta = 0;
    }

    if (valorSensoresControl[LOOP_CORRIENTE] < LIMITE_INF_PRESION){

        contadorFallaPresionBaja++;

        if (contadorFallaPresionBaja > 2 && obtenerPrioridadAlarma(PRESION_BAJA) < obtenerPrioridadAlarma(causaAlarma)){
            causaAlarma = PRESION_BAJA;
            guardarAlarmaNVS((int32_t) causaAlarma);
        }

        } else {
        contadorFallaPresionBaja = 0;
    }

}

int obtenerPrioridadAlarma(causaAlarma_t causa){
    switch (causa){
    case NO_FALLA:
        return 6;
    case PRESION_ALTA:
    case PRESION_BAJA:
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
        case ALARMA: break;
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
    if (etapasControl[ETAPA_1] == HIGH){
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
    // Si la 4ta etapa esta apagada es que podria prenderse alguna
    if (etapasControl[ETAPA_4] == LOW ){
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
    // switch(causaAlarma){
    //     case TEMPERATURA_ALTA:
    //         if (contadorFalla == 0){
    //             solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
    //             solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
    //         }
    //         break;

    //     case TERMOSTATO_SEGURIDAD:

    //         if (contadorFalla == 0){
    //             cortarFalla();
    //         }
    //         else if (contadorFalla <= 1000){
    //             solicitarDesactivarRele(ETAPA_5, true, TIEMPO_APAGADO_BOMBAS_MS);
    //             solicitarDesactivarRele(ETAPA_6, true, TIEMPO_APAGADO_BOMBAS_MS);
    //             forzarFalla();
    //         }
    //         break;

    // }
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
