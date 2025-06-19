#include "MQTTManager.h"
#include <WiFi.h>
#include <FS.h>          // Para FS
using fs::FS;            // Para compatibilidad con WebServer
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "../NVS/NVS.h"
#include "ui.h"
#include "ui_helpers.h"

const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_subscribe_topic = "boiler/update";
const char* mqtt_publish_topic = "boiler/control";

WiFiClient espClient;
PubSubClient client(espClient);

void check_wifi_signal() {
  Serial.printf("RSSI: %ld dBm\n", WiFi.RSSI());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Mensaje recibido en topic: %s\n", topic);

  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  Serial.printf("Contenido del mensaje: %s\n", message);

  // Llamar a función que procesa y guarda
  procesarMensajeMQTT(message);
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");

    String clientId = "ESP32S3_Client-";
    clientId += String(random(0xffff), HEX);  // clientId unico

    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado MQTT");
      client.subscribe(mqtt_subscribe_topic);
    } else {
      Serial.printf("Error MQTT: %d\n", client.state());
      delay(5000);
    }
  }
}

void inicializarMQTT() {
  WiFiManager wm;
  Serial.println("Conectando WIFI...");

  WiFi.begin();
  if(WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Fallo conexion WIFI, abriendo portal");
    wm.autoConnect("ESP32S3-Flowing");
  }

  Serial.println("WiFi conectado");
  Serial.print("IP Local: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void mqttLoop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void publicarConfiguracionesMQTT() {
  DynamicJsonDocument doc(1024);
  const Configs* configs = obtenerConfiguracionesNVS();

  doc["boiler_id"] = 2;
  doc["tempCal"] = configs->temperaturaCalefaccion;
  doc["temperature"] = configs->temperaturaACS;
  doc["calefaccionOn"] = configs->calefaccionOn;
  doc["status"] = configs->encendidoOn;
  doc["diagnosticoOn"] = configs->diagnosticoOn;

  String json;
  serializeJson(doc, json);
  client.publish(mqtt_publish_topic, json.c_str());
  client.loop(); 
  Serial.println("[MQTT] Config enviado: " + json);
}

void publicarAlarmasMQTT() {
  DynamicJsonDocument doc(1024);
  const int32_t* alarmas = obtenerAlarmasNVS();

  for(int i=0; i<CANT_ALARMAS; i++) {
    doc["boiler_id"] = 2;
    doc["alarma"+String(i)] = alarmas[i];
  }

  String json;
  serializeJson(doc, json);
  client.publish(mqtt_publish_topic, json.c_str());
  client.loop(); 
  Serial.println("[MQTT] Alarmas enviadas: " + json);
}

void procesarMensajeMQTT(const char* mensaje) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, mensaje);
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println("[MQTT] Configuracion Recibida: " + jsonString);


  if (error) {
    Serial.println("Error al parsear JSON MQTT");
    return;
  }

  if (doc.containsKey("tempCal")) {
    float tempConfig = lv_spinbox_get_value(ui_SpinboxCalefaccion)/10.0;
    guardarConfigsNVS(TEMP_CALEFACCION, &tempConfig, sizeof(tempConfig));
  }

  // if (doc.containsKey("tempAcs")) {
  //   float tempConfig = lv_spinbox_get_value(ui_SpinboxACS)/10.0;
  //   guardarConfigsNVS(TEMP_ACS, &tempConfig, sizeof(tempConfig));
  // }

  if (doc.containsKey("calefaccionOn")) {
    bool toggleCalefaccion = !configuracionesUI_->calefaccionOn;
    guardarConfigsNVS(CALEFACCION_ON, &toggleCalefaccion, sizeof(toggleCalefaccion));
  }

  // if (doc.containsKey("encendidoOn")) {
  //   bool toggleEncendido = !configuracionesUI_->encendidoOn;
  //   guardarConfigsNVS(ENCENDIDO_ON, &toggleEncendido, sizeof(toggleEncendido));
  // }

  if (doc.containsKey("status")) {  // STATUS LLEGA DESDE MQTT BACKEND
    bool toggleEncendido = !configuracionesUI_->encendidoOn;
    guardarConfigsNVS(ENCENDIDO_ON, &toggleEncendido, sizeof(toggleEncendido));
}

    // Si llega temperature -> guardar en TEMP_ACS
    if (doc.containsKey("temperature")) {
      float tempAcs = doc["temperature"].as<float>();
      guardarConfigsNVS(TEMP_ACS, &tempAcs, sizeof(tempAcs));
    }
  

  // if (doc.containsKey("diagnosticoOn")) {
  //   bool diag = doc["diagnosticoOn"];
  //   guardarConfigsNVS(ID_DIAGNOSTICO, &diag, sizeof(diag));
  // }
}

