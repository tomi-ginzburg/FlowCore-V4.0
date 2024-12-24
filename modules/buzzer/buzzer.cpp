#include "buzzer.h"

#include "..\contador\contador.h"
#include "Arduino.h"

// =====[Declaracion de defines privados]============

#define BUZZER              7
#define FRECUENCIA          2500
#define TIEMPO_ALARMA_MS    1000 
#define TIEMPO_BOCINA_MS    40

// =====[Declaracion de tipos de datos privados]=====

typedef enum {
    BOCINA_ENCENDIDA,
    ALARMA_ENCENDIDA,
    BUZZER_APAGADO
} estadoBuzzer_t;

estadoBuzzer_t estadoBuzzer;
bool flagActivarAlarma;
bool flagDesactivarAlarma;
bool flagActivarBocina;
uint64_t contador;

// =====[Implementacion de funciones publicas]=====

void inicializarBuzzer(){
    ledcAttach(BUZZER, FRECUENCIA, 8);
    ledcWriteTone(BUZZER,0);
    estadoBuzzer = BUZZER_APAGADO;
}

void solicitarActivarAlarma(){
    flagActivarAlarma = true;
}

void solicitarDesactivarAlarma(){
    flagDesactivarAlarma = true;
}

void solicitarActivarBocina(){
    flagActivarBocina = true;
}

void actualizarBuzzer(){
    switch(estadoBuzzer){
        case BUZZER_APAGADO:

            // Actualiza el estado del buzzer
            if (flagActivarBocina == true){
                ledcWriteTone(BUZZER,FRECUENCIA);
                contarMs(&contador, TIEMPO_BOCINA_MS, LOW);
                estadoBuzzer = BOCINA_ENCENDIDA;
                flagActivarBocina = false;
            }

            if (flagActivarAlarma == true){
                estadoBuzzer = ALARMA_ENCENDIDA;
                flagActivarAlarma = false;
            }
            if (flagDesactivarAlarma == true){
                flagDesactivarAlarma = false;
            }

            break;

        case BOCINA_ENCENDIDA:
            if (contador == 0){
                ledcWriteTone(BUZZER,0);
                estadoBuzzer = BUZZER_APAGADO;
            } 
            break;

        case ALARMA_ENCENDIDA:
            if (contador == 0){
                // Actualiza el sonido del buzzer
                if (ledcRead(BUZZER) == 0){
                    ledcWriteTone(BUZZER,FRECUENCIA);
                } else {
                    ledcWriteTone(BUZZER,0);
                }
                contarMs(&contador, TIEMPO_ALARMA_MS, LOW);
            }
            
            // Actualiza el estado del buzzer
            if (flagDesactivarAlarma == true){
                estadoBuzzer = BUZZER_APAGADO;
                ledcWriteTone(BUZZER,0);
                flagDesactivarAlarma = false;
            }
            if (flagActivarAlarma == true){
                flagActivarAlarma = false;
            }

            break;
        default: estadoBuzzer = BUZZER_APAGADO;
    }
}