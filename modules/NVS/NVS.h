#ifndef NVS_H_
#define NVS_H_

#include <Arduino.h>

// =====[Declaracion de defines publicos]============

#define CANT_ALARMAS    5

// =====[Declaracion de tipos de datos publicos]=====

struct Configs{
    uint16_t calData[5];
    bool calibracion;
    bool calefaccionOn;
    bool encendidoOn;
    bool diagnosticoOn;
    float temperaturaCalefaccion;
    float temperaturaCalefaccionMaximo;
    float temperaturaCalefaccionMinimo;
    float temperaturaACS;
    float temperaturaACSMaximo;
    float temperaturaACSMinimo;
    float histeresisCalefaccion;
    float histeresisACS;
    uint16_t retardoEncendidoMs;
    uint16_t retardoApagadoMs; 
};

enum ConfigID{
    CAL_DATA,
    CALIBRACION,
    CALEFACCION_ON,
    ENCENDIDO_ON,
    DIAGNOSTICO_ON,
    TEMP_CALEFACCION,
    TEMP_CALEFACCION_MAX,
    TEMP_CALEFACCION_MIN,
    TEMP_ACS,
    TEMP_ACS_MAX,
    TEMP_ACS_MIN,
    HISTERESIS_CALEFACCION,
    HISTERESIS_ACS,
    RETARDO_ENCENDIDO,
    RETARDO_APAGADO,
    CANT_ID // El ultimo elemento coincide con la cantidad de IDs
};

extern struct Configs configuraciones;
extern int32_t alarmas[CANT_ALARMAS];

// =====[Declaracion de funciones publicas]==========

#ifdef __cplusplus
extern "C" {
#endif

/*
 * pre: -
 * post: obtiene las configuraciones y las alarmas guardadas en la memoria, y si 
 *       no habuia ningun valor guardado, lo inicia con el valor por defecto
 */
void inicializarNVS();


/*
 * pre: -
 * post: guarda en el espacio de configuraciones, en el lugar correspondiente a id, valor.
 */
void guardarConfigsNVS(enum ConfigID id, void* valor, size_t largoDato);

/*
 * pre: -
 * post: guarda la nueva causa de alarma en el array alarmas, desplazando las anteriores
 *       y actualiza la memoria no volatil con los nuevos valores.
 */
void guardarAlarmaNVS(int32_t causa);

/*
 * pre: -
 * post: devuelve puntero constante a configuraciones
 */
const struct Configs* obtenerConfiguracionesNVS();

/*
 * pre: -
 * post: devuelve un puntero constante al array de alarmas
 */
#ifdef TESTING
int32_t* obtenerAlarmasNVS();
#else
const int32_t* obtenerAlarmasNVS();
#endif
#ifdef __cplusplus
}
#endif
#endif