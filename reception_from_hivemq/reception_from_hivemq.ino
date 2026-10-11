#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ============ CONFIGURATION ============

const char* ssid = "nsiriii";
const char* password = "123456780000";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Topics LEDs
const char* topic_led1 = "nsiri/led/1";
const char* topic_led2 = "nsiri/led/2";
const char* topic_led3 = "nsiri/led/3";
const char* topic_led4 = "nsiri/led/4";

WiFiClient espClient;
PubSubClient client(espClient);

// ============ CALLBACK MQTT ============

void callback(char* topic, byte* payload, unsigned int length) {
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("MQTT recu [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  // Envoie la commande au STM32 via UART
  // Format: "LED1:1\n" ou "LED2:1\n"
  if (strcmp(topic, topic_led1) == 0) {
    Serial.println("LED1:1");
  }
  else if (strcmp(topic, topic_led2) == 0) {
    Serial.println("LED2:1");
  }
  else if (strcmp(topic, topic_led3) == 0) {
    Serial.println("LED3:1");
  }
  else if (strcmp(topic, topic_led4) == 0) {
    Serial.println("LED4:1");
  }
}

// ============ FONCTIONS ============

void setup_wifi() {
  Serial.print("Connexion WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" OK");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("MQTT...");
    String clientId = "ESP8266-LED-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connecte !");
      
      // S'abonne aux 4 topics
      client.subscribe(topic_led1);
      client.subscribe(topic_led2);
      client.subscribe(topic_led3);
      client.subscribe(topic_led4);
      
      Serial.println("Abonne aux 4 topics LED");
    } else {
      Serial.print("echec, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

// ============ SETUP ============

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=======================");
  Serial.println("ESP8266 - Relai LED");
  Serial.println("=======================");
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ============ LOOP ============

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}