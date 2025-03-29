#include "NVS.h"

#include <Preferences.h>

// =====[Declaracion de defines privados]============


// =====[Declaracion de tipos de datos privados]=====

typedef struct {
    ConfigID id;
    const char* clave;      
    int tipo;        
    void* miembro;         
    size_t largo;          
} ParametrosConfiguracion_t;

Preferences nvs;

Configs configuraciones = {
    {0, 0, 0, 0, 0},
    false,
    false,
    true,
    false,
    60.0,
    80.0,
    30.0,
    50.0,
    70.0,
    20.0,
    10.0,
    1.0,
    3000,
    1000
};

int32_t  alarmas[CANT_ALARMAS] = {0, 0, 0, 0, 0};
char clavesAlarma[CANT_ALARMAS][15];

ParametrosConfiguracion_t parametros[CANT_ID] = {
    {CAL_DATA, "calData", PT_BLOB, configuraciones.calData, sizeof(configuraciones.calData)},
    {CALIBRACION, "calibracion", PT_U8, &configuraciones.calibracion, sizeof(configuraciones.calibracion)},
    {CALEFACCION_ON, "calefaccion", PT_U8, &configuraciones.calefaccionOn, sizeof(configuraciones.calefaccionOn)},
    {ENCENDIDO_ON, "encendido", PT_U8, &configuraciones.encendidoOn, sizeof(configuraciones.encendidoOn)},
    {DIAGNOSTICO_ON, "diagnostico", PT_U8, &configuraciones.diagnosticoOn, sizeof(configuraciones.diagnosticoOn)},
    {TEMP_CALEFACCION, "tempCal", PT_BLOB, &configuraciones.temperaturaCalefaccion, sizeof(configuraciones.temperaturaCalefaccion)},
    {TEMP_CALEFACCION_MAX, "tempCalMax", PT_BLOB, &configuraciones.temperaturaCalefaccionMaximo, sizeof(configuraciones.temperaturaCalefaccionMaximo)},
    {TEMP_CALEFACCION_MIN, "tempCalMin", PT_BLOB, &configuraciones.temperaturaCalefaccionMinimo, sizeof(configuraciones.temperaturaCalefaccionMinimo)},
    {TEMP_ACS, "tempAcs", PT_BLOB, &configuraciones.temperaturaACS, sizeof(configuraciones.temperaturaACS)},
    {TEMP_ACS_MAX, "tempAcsMax", PT_BLOB, &configuraciones.temperaturaACSMaximo, sizeof(configuraciones.temperaturaACSMaximo)},
    {TEMP_ACS_MIN, "tempAcsMin", PT_BLOB, &configuraciones.temperaturaACSMinimo, sizeof(configuraciones.temperaturaACSMinimo)},
    {HISTERESIS_CALEFACCION, "histCal", PT_BLOB, &configuraciones.histeresisCalefaccion, sizeof(configuraciones.histeresisCalefaccion)},
    {HISTERESIS_ACS, "histAcs", PT_BLOB, &configuraciones.histeresisACS, sizeof(configuraciones.histeresisACS)},
    {RETARDO_ENCENDIDO, "RetEncen", PT_U16, &configuraciones.retardoEncendidoMs, sizeof(configuraciones.retardoEncendidoMs)},
    {RETARDO_APAGADO, "RetApag", PT_U16, &configuraciones.retardoApagadoMs, sizeof(configuraciones.retardoApagadoMs)}
};

// =====[Declaracion de funciones privadas]==========

void leerConfigsNVS(ConfigID id, void* valor, size_t largoDato=1);

// =====[Implementacion de funciones publicas]=======

void inicializarNVS(){

    // Creo un array para saber si cada clave esta guardada o no
    // No veo la condicioin dentro del  segundo for porque necesito abrir nvs, y las otras funciones ya lo abren dentro
    bool clavesGuardadas[CANT_ID];

    // Obtengo los que ya estan guardados
    nvs.begin("Configs", true);
    // nvs.begin("Configs", false);
    // nvs.clear();
    for (int i = 0; i < CANT_ID; i++) {
        clavesGuardadas[i] = nvs.isKey(parametros[i].clave);
    }
    nvs.end();

    // Si ya estaban guardados los leo, sino los escirbo con los parametros por default
    for (int i=0; i < CANT_ID; i++){ 
        if (clavesGuardadas[i]) {
            leerConfigsNVS(parametros[i].id, parametros[i].miembro, parametros[i].largo);
        } else {
            guardarConfigsNVS(parametros[i].id, parametros[i].miembro, parametros[i].largo);
        }
    }

    nvs.begin("Alarmas", true);
    for (int i=0; i < CANT_ALARMAS; i++){
        // Creo las claves
        snprintf(clavesAlarma[i], sizeof(clavesAlarma[i]), "alarma%d", i+1);

        // Leo los valores, si no estan los inicio en 0
        if (nvs.isKey(clavesAlarma[i])){
            alarmas[i] = nvs.getInt(clavesAlarma[i]);

        } else{
            nvs.putInt(clavesAlarma[i],(int32_t) 0);
        }
    }
    nvs.end();
    
}

void guardarConfigsNVS(ConfigID id, void* valor, size_t largoDato){
    
    nvs.begin("Configs", false);

    switch (parametros[id].tipo){
        case PT_U8:
            nvs.putUChar(parametros[id].clave, *(uint8_t*)valor);
            *(bool*)(parametros[id].miembro) = *(bool*)valor;
            break;
        case PT_BLOB:
            nvs.putBytes(parametros[id].clave, valor, largoDato);
            memcpy(parametros[id].miembro, valor, largoDato);
            break;
        case PT_U16:
            nvs.putUShort(parametros[id].clave, *(uint16_t*)valor);
            *(uint16_t*)(parametros[id].miembro) = *(uint16_t*)valor;
            break;
    }

    nvs.end();
}

void guardarAlarmaNVS(int32_t causa){
    
    // Desplazar las alarmas una posición hacia atrás
    for (int i = CANT_ALARMAS - 1; i > 0; i--) {
        alarmas[i] = alarmas[i-1];
    }
    // Guardar la nueva alarma en la primera posición
    alarmas[0] = causa;

    // Guardar las alarmas en NVS
    nvs.begin("Alarmas", false);
    for (int i = 0; i < CANT_ALARMAS; i++) {
        nvs.putInt(clavesAlarma[i], alarmas[i]);
    }
    nvs.end();
    
}

void borrarAlarmasNVS(){
    nvs.begin("Alarmas", false);
    for (int i=0; i < CANT_ALARMAS; i++){
        nvs.putInt(clavesAlarma[i],(int32_t) 0);
        alarmas[i] = 0;
    }
    nvs.end();
}

const Configs* obtenerConfiguracionesNVS(){
    return &configuraciones;
}

#ifdef TESTING
int32_t* obtenerAlarmasNVS(){
#else
const int32_t* obtenerAlarmasNVS(){
#endif
    return alarmas;
}

// =====[Implementacion de funciones privadas]=======

void leerConfigsNVS(ConfigID id, void* valor, size_t largoDato) {
    bool esClave;
    
    nvs.begin("Configs", true);
    esClave = nvs.isKey(parametros[id].clave);
    nvs.end();

    if (!esClave || valor == NULL) {
        return;
    }

    nvs.begin("Configs", true);
    
    switch (parametros[id].tipo) {
        case PT_U8:
            *(bool*) valor = nvs.getUChar(parametros[id].clave);
            break;
        case PT_I32:
            *(int32_t*)valor = nvs.getInt(parametros[id].clave);
            break;
        case PT_U16:
            *(uint16_t*)valor = nvs.getUShort(parametros[id].clave);
            break;
        case PT_BLOB:
            nvs.getBytes(parametros[id].clave, valor, largoDato); 
            break;
    }

    nvs.end(); // Finaliza la sesión de NVS
}
