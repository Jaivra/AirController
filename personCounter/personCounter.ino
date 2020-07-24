#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <TaskScheduler.h>
#include <DHT.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>


/*
 * PIN MAPPING 1
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
 * personCounter setting
 */

const int PERSON_IN_PIN = D1;
const int PERSON_OUT_PIN = D5;

int personCount = 0;

const int PERSON_COUNTER_IDLE = 0;
const int PERSON_COUNTER_IN_EXCITED = 1;
const int PERSON_COUNTER_OUT_EXCITED = 2;
const int PERSON_COUNTER_JOIN = 3;
const int PERSON_COUNTER_EXIT = 4;
const int PERSON_COUNTER_INCREMENTED = 5;

int PERSON_COUNTER_STATE = PERSON_COUNTER_IDLE;


/*
 * MQTT settings 
 */

const char* connectionTopic = "valerio/connection/personCounter";

const char* personCounterTopic = "valerio/room/personCounter";
const char* setPersonCounterTopic = "valerio/room/setPersonCounter";

PubSubClient client(espClient);


/*
 * Other var
 */

Scheduler taskManager;


/*
 * Task declaration
 */
 
void testCallback();
Task testTask(1000, TASK_FOREVER, &testCallback, &taskManager, false);

void sendPersonCounterCallback();
Task sendPersonCounterTask(1000 * 5, TASK_FOREVER, &sendPersonCounterCallback, &taskManager, true);

void personCounterRestartCallback();
Task personCounterRestartTask(1000 * 10, TASK_FOREVER, &personCounterRestartCallback, &taskManager);

void MQTTLoopCallback();
Task MQTTLoopTask(1000 * 5, TASK_FOREVER, &MQTTLoopCallback, &taskManager, true);


/*
 * testTask
 */
 
void testCallback() {
  
}


/*
 * personCounterTask and functions
 */

byte personCounterInCallbackLog = true;
void ICACHE_RAM_ATTR personCounterInCallback() {
  if (PERSON_COUNTER_STATE == PERSON_COUNTER_INCREMENTED)
    return;
    
  if (PERSON_COUNTER_STATE == PERSON_COUNTER_OUT_EXCITED)
    PERSON_COUNTER_STATE = PERSON_COUNTER_JOIN;
  else {
    PERSON_COUNTER_STATE = PERSON_COUNTER_IN_EXCITED;
    startPersonCounterCountDown(1000 * 4);
  }
  
  if (personCounterInCallbackLog) {
    Serial.print("PersonCounter sensor IN Excited");

    Serial.print("PersonCounter State: ");
    Serial.println(PERSON_COUNTER_STATE);
  }

  calculateNextPersonCounterState();
}


byte personCounterOutCallbackLog = true;
void ICACHE_RAM_ATTR personCounterOutCallback() {
  if (PERSON_COUNTER_STATE == PERSON_COUNTER_INCREMENTED)
    return;
    
  if (PERSON_COUNTER_STATE == PERSON_COUNTER_IN_EXCITED)
    PERSON_COUNTER_STATE = PERSON_COUNTER_EXIT;
  else {
    PERSON_COUNTER_STATE = PERSON_COUNTER_OUT_EXCITED;
    startPersonCounterCountDown(1000 * 2.5);
  }
  
  if (personCounterOutCallbackLog) {
    Serial.print("PersonCounter sensor OUT excited: ");
    
    Serial.print("PersonCounter State: ");
    Serial.println(PERSON_COUNTER_STATE);
  } 

  calculateNextPersonCounterState();
}


byte calculateNextPersonCounterStateLog = true;
void calculateNextPersonCounterState() {
  if (PERSON_COUNTER_STATE == PERSON_COUNTER_JOIN) {
    personCount += 1;
    PERSON_COUNTER_STATE = PERSON_COUNTER_IDLE;
    startPersonCounterCountDown(1000 * 7);
  }
  else if (PERSON_COUNTER_STATE == PERSON_COUNTER_EXIT) {
    personCount -= 1;
    PERSON_COUNTER_STATE = PERSON_COUNTER_IDLE;
    startPersonCounterCountDown(1000 * 7);    
  }

  if (calculateNextPersonCounterStateLog) {
    Serial.print("COUNTER:");
    Serial.println(personCount);
  } 
}


void startPersonCounterCountDown(int mill) {
  personCounterRestartTask.disable();
  personCounterRestartTask.enable();
  personCounterRestartTask.setInterval(mill);
}


byte personCounterRestartCallbackLog = true;
void personCounterRestartCallback() {
  PERSON_COUNTER_STATE = PERSON_COUNTER_IDLE;
  personCounterRestartTask.disable();
  if (personCounterRestartCallbackLog)
    Serial.println("personCounterRestart");
  
}


const byte sendPersonCounterCallbackLog = false;
void sendPersonCounterCallback() {
  char data[8];
  
  dtostrf(personCount, 6, 2, data);
  client.publish(personCounterTopic, data);

  if (sendPersonCounterCallbackLog) {
    Serial.print("Publish message personCounter: ");
    Serial.println(personCounterTopic);
  }
}


/*
 * MQTTTask and functions
 */

const byte MQTTLoopCallbackLog = false;
void MQTTLoopCallback() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}


byte MQTTReceivedMessageLog = false;
void MQTTReceivedMessage(char* topic, byte* payload, unsigned int length) {
 if (strcmp(setPersonCounterTopic, topic) == 0)
    personCount = (int) atof((char*)payload);
    
 if (MQTTReceivedMessageLog) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
  
  
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
 
    Serial.println("-----------------------");
  }
}


void reconnect() {
  while (!client.connected()) {
    if (MQTTLoopCallbackLog)
      Serial.print("Attempting MQTT connection...");
    
    String clientId = "ValerioESP8266ClientPersonCounter-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      if (MQTTLoopCallbackLog)
        Serial.println("connected");

      client.publish(connectionTopic, "true");
      
      client.subscribe("valerio/#");
      client.setCallback(MQTTReceivedMessage);
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
  pinMode(PERSON_IN_PIN, INPUT);
  pinMode(PERSON_OUT_PIN, INPUT);  

  attachInterrupt(digitalPinToInterrupt(PERSON_IN_PIN), personCounterInCallback, RISING);
  attachInterrupt(digitalPinToInterrupt(PERSON_OUT_PIN), personCounterOutCallback, RISING);

}


void initObjects() {
  setup_wifi();
  
  client.setServer(mqtt_server, 1883);
}


void setup() {
  Serial.begin(9600);
  initPin();
  initObjects();
}


void loop() {
  taskManager.execute();  
}
