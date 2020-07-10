#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <TaskScheduler.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>


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

const char* temperatureTopic = "valerio/room/temperature";
const char* humidityTopic = "valerio/room/humidity";
const char* heatIndexTopic = "valerio/room/heatIndex";
const char* connectionTopic = "valerio/connected";

PubSubClient client(espClient);



/*
 * IR settings
 */

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

int CODES[8] = {
  0x8808440,
  0x8808541,
  0x8808642,
  0x8808743,
  0x8808844,
  0x8808945,
  0x8808A46,
  0x8808B47
};

int POWER_ON = 0x880064A;
int POWER_OFF = 0x88C0051;

int RECV_PIN = 12;    



/*
 * Temperature settings
 */

DHT dht(D2, DHT22); //Inizializza oggetto chiamato "dht", parametri: pin a cui è connesso il sensore, tipo di dht 11/22
float temperature;
float humidity;
float heatIndex;


/*
 * Other var
 */

Scheduler taskManager;
HTTPClient http;


/*
 * Task declaration
 */

void roomTemperatureTask();
Task t1(1000 * 5, TASK_FOREVER, &roomTemperatureTask, &taskManager, true);

void sendRoomTemperatureTask();
Task t2(1000 * 10, TASK_FOREVER, &sendRoomTemperatureTask, &taskManager, true);

void simulateOutTemperatureTask();
Task t3(5000, TASK_FOREVER, &simulateOutTemperatureTask, &taskManager, true);

void MQTTLoopTask();
Task t4(1000, TASK_FOREVER, &MQTTLoopTask, &taskManager, true);


/*
 * TemperatureTask and functions
 */
 
byte roomTemperatureTaskLog = false;
void roomTemperatureTask() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  heatIndex = dht.computeHeatIndex(temperature, humidity, false);
  //Stampa umidità e temperatura tramite monitor seriale
  if (roomTemperatureTaskLog) {
    Serial.print("Umidità: ");
    Serial.print(humidity);
    Serial.print(" %, Temp: ");
    Serial.print(temperature);
    Serial.println(" Celsius");
    Serial.print(heatIndex);
    Serial.println(" Celsius");
    
  }
}

void simulateOutTemperatureTask() {
  http.begin("http://api.openweathermap.org/data/2.5/weather?lat=45.469706&units=metric&lon=9.237468&appid=4cdeae287c5efcdb83c9503436abf8d5");
  int httpCode = http.GET();
 
  if (httpCode > 0) { //Check the returning code
    String payload = http.getString();   //Get the request response payload
    Serial.println(payload);                     //Print the response payload
  }
 
  http.end();   //Close connection
}

/*
 * MQTTTask and functions
 */

const byte sendRoomTemperatureTaskLog = false;
void sendRoomTemperatureTask() {
  char data[8];
  
  dtostrf(temperature, 6, 2, data);
  client.publish(temperatureTopic, data);

  dtostrf(humidity, 6, 2, data);
  client.publish(humidityTopic, data);

  dtostrf(heatIndex, 6, 2, data);
  client.publish(heatIndexTopic, data);

  if (sendRoomTemperatureTaskLog) {
    Serial.print("Publish message: ");
    Serial.println(temperature);
  }
}

const byte MQTTLoopTaskLog = true;
void MQTTLoopTask() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void reconnect() {
  while (!client.connected()) {
    if (MQTTLoopTaskLog)
      Serial.print("Attempting MQTT connection...");
    
    String clientId = "ValerioESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      if (MQTTLoopTaskLog)
        Serial.println("connected");

      client.publish(connectionTopic, "true");
      
      //client.subscribe("inTopic");
    } else {
      if (MQTTLoopTaskLog) {
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

void initObjects() {
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  dht.begin();
}

void setup() {
  Serial.begin(9600);
  initObjects();
}

void loop() {
  taskManager.execute();  
}
