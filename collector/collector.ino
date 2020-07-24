#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <TaskScheduler.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>


/*
 * PIN MAPPING
 */

static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;


/*
 * WIFI/MQTT settings
 */

const char* ssid = "DownStair2.4";
const char* password = "MyEsp8266_&19!";
const char* mqtt_server = "mqtt.atrent.it";

WiFiClient espClient;


/*
 * MQTT settings 
 */

const char* connectionTopic = "valerio/connected/collector";

const char* roomTemperatureTopic = "valerio/room/temperature";
const char* roomHumidityTopic = "valerio/room/humidity";
const char* roomHeatIndexTopic = "valerio/room/heatIndex";
const char* roomHumidexTopic = "valerio/room/humidex";

const char* outTemperatureTopic = "valerio/out/temperature";
const char* outHumidityTopic = "valerio/out/humidity";
const char* outHeatIndexTopic = "valerio/out/heatIndex";
const char* outHumidexTopic = "valerio/out/humidex";

PubSubClient client(espClient);



/*
 * Temperature settings
 */

const int RECV_TEMPERATURE_PIN = D2;

DHT dht(RECV_TEMPERATURE_PIN, DHT22); //Inizializza oggetto chiamato "dht", parametri: pin a cui è connesso il sensore, tipo di dht 11/22
float roomTemperature;
float roomHumidity;
float roomHeatIndex;
float roomHumidex;

float outTemperature;
float outHumidity;
float outHeatIndex;
float outHumidex;


/*
 * Other var
 */

Scheduler taskManager;
HTTPClient http;


/*
 * Task declaration
 */
 
void testCallback();
Task testTask(1000, TASK_FOREVER, &testCallback, &taskManager, false);

void measureRoomTemperatureCallback();
Task measureRoomTemperatureTask(1000 * 20, TASK_FOREVER, &measureRoomTemperatureCallback, &taskManager, true);

void sendRoomTemperatureCallback();
Task sendRoomTemperatureTask(1000 * 60 * 2, TASK_FOREVER, &sendRoomTemperatureCallback, &taskManager, true);

void measureSimulateOutTemperatureCallback();
Task measureSimulateOutTemperatureTask(1000 * 20, TASK_FOREVER, &measureSimulateOutTemperatureCallback, &taskManager, true);

void sendOutTemperatureCallback();
Task sendOutTemperatureTask(1000 * 60 * 2, TASK_FOREVER, &sendOutTemperatureCallback, &taskManager, true);

void MQTTLoopCallback();
Task MQTTLoopTask(1000 * 5, TASK_FOREVER, &MQTTLoopCallback, &taskManager, true);


/*
 * 
 */

void testCallback() {
  
}


/*
 * TemperatureTask and functions
 */

byte measureRoomTemperatureCallbackLog = false;
void measureRoomTemperatureCallback() {
  roomHumidity = dht.readHumidity();
  roomTemperature = dht.readTemperature();
  roomHeatIndex = dht.computeHeatIndex(roomTemperature, roomHumidity, false);
  roomHumidex = calcHumidex(roomTemperature, roomHumidity);

  if (measureRoomTemperatureCallback) {
    Serial.print("Room Humidity: ");
    Serial.print(roomHumidity);
    Serial.println(" %");
    
    Serial.print("Room Temperature: ");
    Serial.print(roomTemperature);
    Serial.println(" Celsius");
    
    Serial.print("Room Humidex: ");
    Serial.print(roomHumidex);
    Serial.println(" Celsius");
    
  }
}


byte measureSimulateOutTemperatureCallbackLog = false;
void measureSimulateOutTemperatureCallback() {
  http.begin("http://api.openweathermap.org/data/2.5/weather?lat=45.469706&units=metric&lon=9.237468&appid=4cdeae287c5efcdb83c9503436abf8d5");
  int httpCode = http.GET();

  if (httpCode > 0) { //Check the returning code
    
    StaticJsonDocument<64> filter;
    filter["main"]["temp"] = true;
    filter["main"]["humidity"] = true;
    
    String json = http.getString();   //Get the request response payload

    // https://arduinojson.org/v6/assistant/
    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(13);
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, json);
    JsonObject root = doc["main"].as<JsonObject>();
    

    outHumidity= root["humidity"].as<float>();
    outTemperature = root["temp"].as<float>();
    outHeatIndex = dht.computeHeatIndex(outTemperature, outHumidity, false);
    outHumidex = calcHumidex(outTemperature, outHumidity);
    
    if (measureSimulateOutTemperatureCallbackLog) {
      //serializeJsonPretty(doc, Serial);
      Serial.print("Out Humidity: ");
      Serial.print(outHumidity);
      Serial.println(" %");
    
      Serial.print("Out Temperature: ");
      Serial.print(outTemperature);
      Serial.println(" Celsius");
    
      Serial.print("Out Humidex: ");
      Serial.print(outHumidex);
      Serial.println(" Celsius");
    
    }
  }
 
  http.end();   //Close connection
}


// https://create.arduino.cc/projecthub/somenjana/sensing-the-comfort-level-of-atmosphere-using-humidex-3ee0df
float calcHumidex(float temperature, float humidity) {
  float B=(log(humidity/100)+((17.27*temperature)/(237.3+temperature)))/17.27; // value of "B", the intermediate dimentionless parameter has been calculated.
  float dew=(237.3*B)/(1-B); // The value of dew point has been calculated
  float humidex=temperature+0.5555*(6.11*exp(5417.753*(1/273.16-1/(273.15+dew)))-10); // the value of HUMIDEX has been claculated.
  return humidex;
}


/*
 * MQTTTask and functions
 */

const byte sendRoomTemperatureCallbackLog = false;
void sendRoomTemperatureCallback() {
  char data[8];
  
  dtostrf(roomTemperature, 6, 2, data);
  client.publish(roomTemperatureTopic, data);

  dtostrf(roomHumidity, 6, 2, data);
  client.publish(roomHumidityTopic, data);

  dtostrf(roomHeatIndex, 6, 2, data);
  client.publish(roomHeatIndexTopic, data);

  dtostrf(roomHumidex, 6, 2, data);
  client.publish(roomHumidexTopic, data);

  if (sendRoomTemperatureCallbackLog) {
    Serial.print("Publish message roomHeatIndex: ");
    Serial.println(roomHeatIndex);
  }
}


const byte sendOutTemperatureCallbackLog = false;
void sendOutTemperatureCallback() {
  char data[8];
  
  dtostrf(outTemperature, 6, 2, data);
  client.publish(outTemperatureTopic, data);

  dtostrf(outHumidity, 6, 2, data);
  client.publish(outHumidityTopic, data);

  dtostrf(outHeatIndex, 6, 2, data);
  client.publish(outHeatIndexTopic, data);

  dtostrf(outHumidex, 6, 2, data);
  client.publish(outHumidexTopic, data);

  if (sendOutTemperatureCallbackLog) {
    Serial.print("Publish message outHeatIndex: ");
    Serial.println(roomHeatIndex);
  }
}


const byte MQTTLoopCallbackLog = true;
void MQTTLoopCallback() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}


void reconnect() {
  while (!client.connected()) {
    if (MQTTLoopCallbackLog)
      Serial.print("Attempting MQTT connection...");
    
    String clientId = "ValerioESP8266ClientCollector-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      if (MQTTLoopCallbackLog)
        Serial.println("connected");

      client.publish(connectionTopic, "true");
      
      //client.subscribe("inTopic");
    } else {
      if (MQTTLoopCallbackLog) {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
      }
      delay(5000);
    }
  }
}


/*
 * INIT
 */

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void initPin() {
  pinMode(RECV_TEMPERATURE_PIN, INPUT);
}


void initObjects() {
  setup_wifi();
  
  client.setServer(mqtt_server, 1883);
  
  dht.begin();
}


void setup() {
  Serial.begin(9600);
  initPin();
  initObjects();
}


void loop() {
  taskManager.execute();  
}
