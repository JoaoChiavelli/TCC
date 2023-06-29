#include <dummy.h>
#include <OneWire.h>
#include "WiFi.h"
#include <DallasTemperature.h>
#include <OneWire.h>  
#include <DallasTemperature.h>
#include "Esp32MQTTClient.h" 
#include <PubSubClient.h>
#include <esp_system.h>


//Variaveis Conexões
const char* ssid = "CHIAVELLI-2.4";
const char* password = "chiavelli039";
const char* mqtt_server = "192.168.15.9";
WiFiClient espClient;
PubSubClient client(espClient);


//Variaveis sensor de temperatura
#define dados 25
OneWire oneWire(dados);
DallasTemperature sensors(&oneWire);

int contagem = 0;

int ph_pin = 34;


#define TdsSensorPin 36
#define VREF 3.3 
#define SCOUNT  30  
float analogBuffer;
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;
float averageVoltage = 0;
float tdsValue = 0;
float temperature = 0;
float EC;




void setup() {
 
  Serial.begin(115200);
  pinMode(TdsSensorPin, INPUT);
  sensors.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1885);

}


void setup_wifi() {

  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      client.subscribe("event/CC");
      client.subscribe("event/PH");
      client.subscribe("event/TDS");
      client.subscribe("event/Nivel");
      client.subscribe("event/Temp");
    } else {
      delay(5000);
    }
  }
}



void loop(){

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  sensors.requestTemperatures(); 
  float TempC = sensors.getTempCByIndex(0);
  char Temperatura[15];
  dtostrf(TempC, 7, 3, Temperatura);
  client.publish("event/Temp", Temperatura);


  
  int measure = (analogRead(ph_pin))/6;
  double voltage = (measure * 5.0) / 1024.0; 
  float Po = 7 + ((2.5 - voltage) / 0.18);
  char PH_Calculado[15];
  dtostrf(Po, 7, 3, PH_Calculado); 
  client.publish("event/PH", PH_Calculado);




  analogBuffer = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
  averageVoltage = analogBuffer * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
  float compensationCoefficient = 1.0 + 0.02 * (TempC - 25.0); //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
  float compensationVolatge = averageVoltage / compensationCoefficient; //temperature compensation
  tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value

  Serial.print("TDS Value:");
  Serial.print(tdsValue, 0);
  Serial.println("ppm");   
  Serial.print("EC:");
  EC=((((tdsValue*2))/1000));
  Serial.print(EC);
  Serial.println("mS/cm");
  Serial.println("");

  char TDSEnvio[15];
  char ECEnvio[15];
  dtostrf(EC, 7, 3, ECEnvio);  //// convert float to char
  dtostrf(tdsValue, 7, 3, TDSEnvio);  //// convert float to char
  client.publish("event/CC", ECEnvio);
  client.publish("event/TDS", TDSEnvio);
  delay(200);



  if (millis() > 30000) {
    ESP.restart();
  }

}


