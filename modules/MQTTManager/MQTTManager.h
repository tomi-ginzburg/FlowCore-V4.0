#ifndef MQTT_MANAGER_H_
#define MQTT_MANAGER_H_

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

void inicializarMQTT();
void mqttLoop();
void publicarConfiguracionesMQTT();
void publicarAlarmasMQTT();
void procesarMensajeMQTT(const char* mensaje);

#ifdef __cplusplus
}
#endif

#endif
