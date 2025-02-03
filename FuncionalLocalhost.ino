#include <WiFiManager.h> // Biblioteca WiFiManager
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_subscribe_topic = "boiler/update";
const char* mqtt_publish_topic = "boiler/control";

WiFiClient espClient;
PubSubClient client(espClient);

int boiler_id = 1;
int temperature = 25;
String status = "off";

void setup_wifi() {
  WiFiManager wifiManager;

  // Inicia el portal cautivo si no hay conexión Wi-Fi
  if (!wifiManager.autoConnect("ESP32-Boiler")) {
    Serial.println("No se pudo conectar al Wi-Fi. Reiniciando...");
    ESP.restart(); // Reinicia si no se conecta
  }

  // Conectado con éxito
  Serial.println("Conexión Wi-Fi establecida.");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en el tema: ");
  Serial.println(topic);

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Mensaje: ");
  Serial.println(message);

  if (String(topic) == mqtt_subscribe_topic) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);

    if (!error) {
      boiler_id = doc["boiler_id"];
      temperature = doc["temperature"];
      status = String((const char*)doc["status"]);

      Serial.println("Configuración recibida:");
      Serial.printf("ID: %d, Temperatura: %d, Estado: %s\n", boiler_id, temperature, status.c_str());

      if (status == "on") {
        digitalWrite(2, LOW);
      } else {
        digitalWrite(2, HIGH);
      }

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
      Serial.printf("Suscrito al tema: %s\n", mqtt_subscribe_topic);
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

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
