#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ---- WiFi ----
const char* ssid = "nsiriii";
const char* password = "123456780000";

// ---- MQTT / HiveMQ ----
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* topic_temp = "nsiri/temperature";
const char* topic_hum = "nsiri/humidite";

WiFiClient espClient;
PubSubClient client(espClient);

// Déclaration : SoftwareSerial(broche_RX, broche_TX)
SoftwareSerial stmSerial(D1, D2);

void setup_wifi() {
  Serial.print("Connexion au WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP); // evite les coupures de SoftwareSerial
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

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Connexion au broker MQTT...");
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println(" connecte !");
    } else {
      Serial.print(" echec, code=");
      Serial.print(client.state());
      Serial.println(" nouvelle tentative dans 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);      // Moniteur Série USB (pour voir les résultats)
  stmSerial.begin(115200);   // Liaison série avec le STM32

  Serial.println();
  Serial.println("=== En attente de trames envoyées par le STM32 ===");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  // Si des données arrivent depuis le STM32...
  if (stmSerial.available()) {
    String trame = stmSerial.readStringUntil('\n');
    trame.trim(); // enlève les espaces/retours chariot en trop

    if (trame.length() > 0) {
      Serial.print("Trame reçue du STM32 : ");
      Serial.println(trame);

      parseAndPublish(trame);
    }
  }
}

// Extrait T et H depuis une trame du type "NOMINAL T=23C H=45%"
// et publie chaque valeur sur son topic MQTT
void parseAndPublish(String line) {
  int tIndex = line.indexOf("T=");
  int hIndex = line.indexOf("H=");

  if (tIndex == -1 || hIndex == -1) {
    return; // trame sans donnees exploitables (ex: "Learning: ...")
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