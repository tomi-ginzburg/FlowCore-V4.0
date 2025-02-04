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

#include <WebServer.h>  // Primero WebServer
#include <WiFiManager.h>  // Después WiFiManager
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define TIMEPO_REFRESCO_DISPLAY_MS 10

SemaphoreHandle_t xSemaphore, isrSemaphore, initSemaphore;

const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_subscribe_topic = "boiler/update";
const char* mqtt_publish_topic = "boiler/control";

WiFiClient espClient;
PubSubClient client(espClient);

int boiler_id = 1;
int temperature = 25;
String status = "off";

void medir(void* parameters) {
  while (1) {
    if (xSemaphoreTake(isrSemaphore, portMAX_DELAY) == pdTRUE) {
      if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
        actualizarSensores();
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void presentar(void* parameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  while (1) {
    actualizarDisplay(TIMEPO_REFRESCO_DISPLAY_MS);
    vTaskDelayUntil(&xLastWakeTime, TIMEPO_REFRESCO_DISPLAY_MS / portTICK_PERIOD_MS);
  }
}

void controlar(void* parameters) {
  while (1) {
    if (uxSemaphoreGetCount(xSemaphore) == 0) {
      for (int i = 0; i < CICLOS_PT100_1 + CICLOS_PT100_2 + CICLOS_LOOP_I; i++) {
        xSemaphoreGive(xSemaphore);
      }
      actualizarEntradasMecanicas();
      actualizarControles();
      actualizarReles();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void conectar(void* parameters){
  while(1){
    if (!client.connected()) {
        reconnect();
      }
    client.loop();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void IRAM_ATTR isr_mediciones() {
  BaseType_t xHigherPriorityTaskWoken;
  xSemaphoreGiveFromISR(isrSemaphore, &xHigherPriorityTaskWoken);
}

//---------------------------------------------------
void setup_wifi() {
  WiFiManager wifiManager;
  
  wifiManager.setBreakAfterConfig(true); // No reiniciar si ya tiene Wi-Fi configurado
  
  if (!wifiManager.autoConnect("ESP32-Boiler")) {
    Serial.println("No se pudo conectar al Wi-Fi. Reiniciando...");
    ESP.restart();
  }

  Serial.println("Conexión Wi-Fi establecida.");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == mqtt_subscribe_topic) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);

    if (!error) {
      boiler_id = doc["boiler_id"];
      temperature = doc["temperature"];
      status = String((const char*)doc["status"]);

      Serial.printf("ID: %d, Temperatura: %d, Estado: %s\n", boiler_id, temperature, status.c_str());

      publish_status();
    } else {
      Serial.println("Error al deserializar el mensaje JSON.");
    }
  }
}

void publish_status() {
  DynamicJsonDocument doc(1024);
  doc["boiler_id"] = boiler_id;
  doc["temperature"] = temperature;
  doc["status"] = status;
  doc["wifi_signal"] = WiFi.RSSI(); // Agregar intensidad de señal WiFi

  String message;
  serializeJson(doc, message);

  client.publish(mqtt_publish_topic, message.c_str());
  Serial.println("Estado publicado:");
  Serial.println(message);
}

void reconnect() {
  int attempts = 0;
  while (!client.connected() && attempts < 5) {
    Serial.print("Conectando al broker MQTT...");
    if (client.connect(("ESP32Client_" + String(random(0xffff))).c_str())) {
      Serial.println("Conectado.");
      client.subscribe(mqtt_subscribe_topic);
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(". Intentando de nuevo en 5 segundos...");
      delay(5000);
      attempts++;
    }
  }
  if (!client.connected()) {
    Serial.println("No se pudo conectar al broker MQTT después de 5 intentos.");
  }
}

void check_wifi_signal() {
  long rssi = WiFi.RSSI();
  Serial.print("Intensidad de la señal WiFi (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

//---------------------------------------------------

void setup() {
  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  xSemaphore = xSemaphoreCreateCounting(CICLOS_PT100_1 + CICLOS_PT100_2 + CICLOS_LOOP_I + 1, CICLOS_PT100_1 + CICLOS_PT100_2 + CICLOS_LOOP_I + 1);
  isrSemaphore = xSemaphoreCreateBinary();
  initSemaphore = xSemaphoreCreateBinary();

  inicializarNVS();
  inicializarSensores();
  inicializarDisplay();
  inicializarEntradasMecanicas();
  inicializarReles();
  inicializarBuzzer();
  inicializarControles();
  inicializarContador();

  attachInterrupt(digitalPinToInterrupt(2), isr_mediciones, FALLING);

  xTaskCreate(medir, "Task 1", 4000, NULL, 3, NULL);
  xTaskCreate(controlar, "Task 2", 5000, NULL, 2, NULL);
  xTaskCreate(presentar, "Task 3", 50000, NULL, 1, NULL);
  xTaskCreate(conectar, "Task 4", 5000, NULL, 1, NULL);
}

void loop() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck >= 10000) {
    check_wifi_signal();
    lastCheck = millis();
  }
}
