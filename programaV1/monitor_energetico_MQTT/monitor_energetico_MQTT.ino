#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>



const char* ssid = "Desarrollo";
const char* password = "Desarrollo2022*";


// Credenciales MQTT
const char* mqtt_server = "18.212.130.131";
const int mqtt_port = 1883;
const char* mqtt_clientID = "Monitor energetico";
const char* mqtt_username = "test";
const char* mqtt_password = "CloudTech*";
const char* mqtt_topic = "Admin/test/Dispositivo_MonitorEnergetico/Datos";


WiFiClient espClient;
PubSubClient mqttClient(espClient);

#include <PZEM004Tv30.h>
#define PZEM_RX_PIN 25
#define PZEM_TX_PIN 26


#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 25
#define PZEM_TX_PIN 26
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif


#if defined(ESP32)
/*************************
 *  ESP32 initialization is defined
 * ---------------------
 */
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);

#elif defined(ESP8266)
/*************************
 *  ESP8266 initialization is defined
 * ---------------------
 */
PZEM004Tv30 pzem(Serial1);

#else
/*************************
 *  other board initialization
 * ---------------------
*/
PZEM004Tv30 pzem(PZEM_SERIAL);
#endif

#include <ArduinoJson.h>
char sendbuffer[120];

void setup() {
  Serial.begin(115200);
  delay(3000);

  bool res;
  // Conexión WiFi
  WiFiManager wifiManager;
  res = wifiManager.autoConnect(mqtt_clientID, mqtt_password);
  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }

  mqttClient.setKeepAlive(120);
  mqttClient.setServer(mqtt_server, mqtt_port);
  

reconectar:
  if (mqttClient.connect(mqtt_clientID, mqtt_username, mqtt_password)) {
    Serial.println("Conectado!");
    mqttClient.subscribe(mqtt_topic);
    mqttClient.setCallback(mqttCallback);
  } else {
    Serial.print("Falló la conexión, rc=");
    Serial.println(mqttClient.state());
    delay(2000);
    goto reconectar;
  }
}

void loop() {

  mqttClient.loop();

  // Print the custom address of the PZEM
  Serial.print("Custom Address:");
  Serial.println(pzem.readAddress(), HEX);

  // Read the data from the sensor
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy = pzem.energy();
  float frequency = pzem.frequency();
  float pf = pzem.pf();

  // Check if the data is valid
  if (isnan(voltage)) {
    Serial.println("Error reading voltage");
  } else if (isnan(current)) {
    Serial.println("Error reading current");
  } else if (isnan(power)) {
    Serial.println("Error reading power");
  } else if (isnan(energy)) {
    Serial.println("Error reading energy");
  } else if (isnan(frequency)) {
    Serial.println("Error reading frequency");
  } else if (isnan(pf)) {
    Serial.println("Error reading power factor");
  } else {

    // Print the values to the Serial console
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println("V");
    Serial.print("Current: ");
    Serial.print(current);
    Serial.println("A");
    Serial.print("Power: ");
    Serial.print(power);
    Serial.println("W");
    Serial.print("Energy: ");
    Serial.print(energy, 3);
    Serial.println("kWh");
    Serial.print("Frequency: ");
    Serial.print(frequency, 1);
    Serial.println("Hz");
    Serial.print("PF: ");
    Serial.println(pf);
  }

  crearJson(voltage,current,power,energy,frequency,pf);
  mqttClient.publish(mqtt_topic, sendbuffer);

  Serial.println();
  delay(10000);
}

void reiniciarDatos() {
  pzem.resetEnergy();
}


void crearJson(float voltage,float current,float power,float energy,float frequency,float pf){

  JsonDocument doc;

      // Agregar las variables al objeto JSON
      doc["voltage"] = String(voltage);
      doc["current"] = String(current);
      doc["power"] = String(power);
      doc["energy"] = String(energy);
      doc["frequency"] = String(frequency);
      doc["pf"] = String(pf);

      String json = doc.as<String>();

      uint8_t buffer[120];
      size_t n = json.length() + 1;
      json.toCharArray((char *)buffer, n);

      for (int i = 0; i < n; i++) {
        sendbuffer[i] = (char)buffer[i];
      }

      sendbuffer[n - 1] = '\0';


}

void mqttCallback(char *topic, byte *payload, unsigned int len) {

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();
}