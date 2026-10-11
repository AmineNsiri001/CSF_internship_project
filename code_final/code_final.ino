#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ---- WiFi ----
const char* ssid = "nsiriii";
const char* password = "123456780000";

// ---- MQTT / HiveMQ ----
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Topics température/humidité (STM32 → ESP → MQTT)
const char* topic_temp = "nsiri/temperature";
const char* topic_hum = "nsiri/humidite";

// Topics LEDs (MQTT → ESP → STM32)
const char* topic_led1 = "nsiri/led/1";
const char* topic_led2 = "nsiri/led/2";
const char* topic_led3 = "nsiri/led/3";
const char* topic_led4 = "nsiri/led/4";

WiFiClient espClient;
PubSubClient client(espClient);

// SoftwareSerial : RX = D1 (GPIO5), TX = D2 (GPIO4)
SoftwareSerial stmSerial(D1, D2);

// ============ SETUP WIFI ============

void setup_wifi() {
  Serial.print("Connexion au WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connecte !");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());
}

// ============ CALLBACK MQTT ============
// Appelé quand un message MQTT arrive

void callback(char* topic, byte* payload, unsigned int length) {
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("MQTT recu [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  if (strcmp(topic, topic_led1) == 0) {
    stmSerial.println("LED1:1");
    Serial.println(">> Envoye au STM32: LED1:1");
  }
  else if (strcmp(topic, topic_led2) == 0) {
    stmSerial.println("LED2:1");
    Serial.println(">> Envoye au STM32: LED2:1");
  }
  else if (strcmp(topic, topic_led3) == 0) {
    stmSerial.println("LED3:1");
    Serial.println(">> Envoye au STM32: LED3:1");
  }
  else if (strcmp(topic, topic_led4) == 0) {
    stmSerial.println("LED4:1");
    Serial.println(">> Envoye au STM32: LED4:1");
  }
  // AJOUTE CECI :
  else if (strcmp(topic, "nsiri/led/all") == 0) {
    stmSerial.print("LEDall:");
    stmSerial.println(message);
    Serial.print(">> Envoye au STM32: LEDall:");
    Serial.println(message);
  }
}

// ============ CONNEXION MQTT ============

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Connexion au broker MQTT...");
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println(" connecte !");
      
      // S'abonne aux 4 topics LED
      client.subscribe(topic_led1);
      client.subscribe(topic_led2);
      client.subscribe(topic_led3);
      client.subscribe(topic_led4);
      
      Serial.println("Abonne aux topics LED");
    } else {
      Serial.print(" echec, code=");
      Serial.print(client.state());
      Serial.println(" nouvelle tentative dans 5s");
      delay(5000);
    }
  }
}

// ============ SETUP ============

void setup() {
  Serial.begin(115200);      // USB debug
  stmSerial.begin(115200);   // STM32 via D1/D2

  Serial.println();
  Serial.println("=== ESP8266 Bidirectionnel ===");
  Serial.println("STM32(D1/D2) <-> ESP8266 <-> MQTT");

  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);  // IMPORTANT : déclare le callback MQTT
}

// ============ LOOP ============

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  // ---- PARTIE 1 : STM32 → ESP → MQTT (ton code existant) ----
  if (stmSerial.available()) {
    String trame = stmSerial.readStringUntil('\n');
    trame.trim();

    if (trame.length() > 0) {
      Serial.print("Trame recue du STM32 : ");
      Serial.println(trame);
      parseAndPublish(trame);
    }
  }
}

// ============ FONCTION EXISTANTE (STM32 → MQTT) ============

void parseAndPublish(String line) {
  int tIndex = line.indexOf("T=");
  int hIndex = line.indexOf("H=");

  if (tIndex == -1 || hIndex == -1) {
    return;
  }

  int tempValue = line.substring(tIndex + 2, line.indexOf('C', tIndex)).toInt();
  int humValue  = line.substring(hIndex + 2, line.indexOf('%', hIndex)).toInt();

  char tempStr[10];
  char humStr[10];
  itoa(tempValue, tempStr, 10);
  itoa(humValue, humStr, 10);

  client.publish(topic_temp, tempStr);
  client.publish(topic_hum, humStr);

  Serial.print("Publie -> ");
  Serial.print(tempStr);
  Serial.print("/");
  Serial.println(humStr);
}