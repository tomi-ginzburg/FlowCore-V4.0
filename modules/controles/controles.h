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
    FALLA_SENSOR_2,
    FALLA_SENSOR_I
} causaAlarma_t;

// =====[Declaracion de funciones publicas]==========

#ifdef __cplusplus
extern "C" {
#endif

void inicializarControles();

void actualizarControles();

void solicitarPurgarBomba(int bomba);

void solicitarDiagnosticoEtapa(int etapa, bool encendido);

const estadoControl_t* obtenerEstadoControl();

const causaAlarma_t* obtenerCausaAlarma();


// TESTS
#ifdef TESTING
void reiniciarEstadoControles();
void test_verificarFallaTermostatoSeguridad();
void test_verificarFallaTemperaturaAlta();
void test_verificarFallaTemperaturaBaja();
void test_verificarFallaApagadoBombas();
void test_verificarFallaSensorCaldera();
void test_verificarFallaSensorACS();
void test_verificarFallaEncendidoBombaCalefaccion();
void test_verificarFallaEncendidoBombaACS();
void test_verificarFallaPresionAlta();
void test_verificarFallaPresionBaja();
#endif

#ifdef __cplusplus
}
#endif
#endif