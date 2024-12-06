#ifndef CONTROLES_H_
#define CONTROLES_H_


// =====[Declaracion de defines publicos]============

#define CANT_BOMBAS 2

#define BOMBA_CAL   4
#define BOMBA_ACS   5
// =====[Declaracion de tipos de datos publicos]=====

typedef enum{
    INICIO,
    APAGADO,
    ENCENDIDO_IDLE,
    ENCENDIDO_ACS,
    ENCENDIDO_SUPERVISION,
    DIAGNOSTICO,
    ALARMA
} estadoControl_t;

typedef enum{
    NO_FALLA,
    BOMBA_CALEFACCION_ON,       // No circula la calefaccion
    BOMBA_AGUA_CALIENTE_ON,     // No circula el ACS
    BOMBAS_ON, 
    BOMBAS_OFF,
    PRESION_ALTA,               
    PRESION_BAJA,
    TEMPERATURA_ALTA,
    TEMPERATURA_BAJA,
    TERMOSTATO_SEGURIDAD,
    FALLA_SENSOR_1,
    FALLA_SENSOR_2
} causaAlarma_t;

// =====[Declaracion de funciones publicas]==========

#ifdef __cplusplus
extern "C" {
#endif

void inicializarControles();

void actualizarControles();

void solicitarPurgarBomba(int bomba);

const estadoControl_t* obtenerEstadoControl();

const causaAlarma_t* obtenerCausaAlarma();

#ifdef __cplusplus
}
#endif
#endif