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
 * PIN MAPPING 6
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

const char* connectionTopic = "valerio/connection/handler";

const char* roomTemperatureTopic = "valerio/room/temperature";
const char* roomHumidityTopic = "valerio/room/humidity";
const char* roomHeatIndexTopic = "valerio/room/heatIndex";
const char* roomHumidexTopic = "valerio/room/humidex";
const char* roomSetHumidexTargetTopic = "valerio/room/setHumidexTarget";

const char* outTemperatureTopic = "valerio/out/temperature";
const char* outHumidityTopic = "valerio/out/humidity";
const char* outHeatIndexTopic = "valerio/out/heatIndex";
const char* outHumidexTopic = "valerio/out/humidex";

const char* personCounterTopic = "valerio/room/personCounter";

const char* configurationTopic = "valerio/configuration";

PubSubClient client(espClient);


/*
 * IR settings
 */

const int RECV_IR_PIN = D6;
const int SEND_IR_PIN = D2;    

IRrecv irrecv(RECV_IR_PIN);
decode_results results;

IRsend irsend(SEND_IR_PIN);  // Set the GPIO to be used to sending the message.

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

const int POWER_ON_SIGNAL = 0x880064A;
const int POWER_OFF_SIGNAL = 0x88C0051;


/*
 * NTP settings
 */

const long utcOffsetInSeconds = 3600 * 2;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

 
/*
 * NextState settings
 */
float humidexTarget = 26.5;

float currentRoomTemperature;
float currentRoomHumidity;
float currentRoomHeatIndex;
float currentRoomHumidex;

float currentOutTemperature;
float currentOutHumidity;
float currentOutHeatIndex;
float currentOutHumidex;

float currentPersonCounter;
int currentHour;
int currentMinute;

const int POWER_OFF_STATE = 0;
const int POWER_ON_STATE = 1;

int CONDITIONER_STATE = POWER_OFF_STATE;

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

void MQTTLoopCallback();
Task MQTTLoopTask(1000 * 2, TASK_FOREVER, &MQTTLoopCallback, &taskManager, true);

//void IRRecvCallback();
//Task IRRecvTask(300, TASK_FOREVER, &IRRecvCallback, &taskManager, false);

//void IRSendCallback();
//Task IRSendTask(300, TASK_FOREVER, &IRSendCallback, &taskManager, false);

void updateTimeClientCallback();
Task updateTimeClientTask(1000 * 30, TASK_FOREVER, &updateTimeClientCallback, &taskManager, true);

void nextConditionerStateCallback();
Task nextConditionerStateTask(1000 * 8, TASK_FOREVER, &nextConditionerStateCallback, &taskManager, true);

void sendConfigurationCallback();
Task sendConfigurationTask(1000 *  60, TASK_FOREVER, &sendConfigurationCallback, &taskManager, true);


/*
 * testTask
 */
 
void testCallback() {
  
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

  if (strcmp(roomSetHumidexTargetTopic, topic) == 0)
    humidexTarget = atof((char*)payload);
    
  else if (strcmp(roomTemperatureTopic, topic) == 0)
    currentRoomTemperature = atof((char*)payload);
  else if (strcmp(roomHumidityTopic, topic) == 0)
    currentRoomHumidity = atof((char*)payload);
  else if (strcmp(roomHeatIndexTopic, topic) == 0)
    currentRoomHeatIndex = atof((char*)payload);
  else if (strcmp(roomHumidexTopic, topic) == 0)
    currentRoomHumidex = atof((char*)payload);

  else if (strcmp(outTemperatureTopic, topic) == 0)
    currentOutTemperature = atof((char*)payload);
  else if (strcmp(outHumidityTopic, topic) == 0)
    currentOutHumidity = atof((char*)payload);
  else if (strcmp(outHeatIndexTopic, topic) == 0)
    currentOutHeatIndex = atof((char*)payload);
  else if (strcmp(outHumidexTopic, topic) == 0)
    currentOutHumidex = atof((char*)payload);

  else if (strcmp(personCounterTopic, topic) == 0)
    currentPersonCounter = atof((char*)payload);
 

  if (MQTTReceivedMessageLog) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.println(currentOutHumidex);
  
  
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
    
    String clientId = "ValerioESP8266ClientHandler-";
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
 * IRTask
 */

void IRRecvCallback() {
 if (irrecv.decode(&results)){
    int value = results.value;
    Serial.println(value, HEX);
    irrecv.resume();
    Serial.println();
  }
  else {
    Serial.println("PASSATO");
  }
}


void IRSendCallback() {
  irsend.sendNEC(0x880064A, 28);
}


/*
 * NTPTask and functions 
 */

byte updateTimeClientCallbackLog = false;
void updateTimeClientCallback() {
  timeClient.update();
  currentHour = timeClient.getHours();
  currentMinute = timeClient.getMinutes();
  
  if (updateTimeClientCallbackLog) {

    //Serial.print(daysOfTheWeek[timeClient.getDay()]);
    //Serial.print(", ");
    Serial.print(currentHour);
    Serial.print(":");
    Serial.print(currentMinute);
    Serial.print(":");
    Serial.println(timeClient.getSeconds());
  }
}
 
/*
 * configurationTask and functions
 */

byte sendConditionerSignalLog = true;
void sendConditionerSignal(int nextState) {
  if (nextState != CONDITIONER_STATE) {
    switch(nextState) {
      case POWER_OFF_STATE: irsend.sendNEC(POWER_OFF_SIGNAL, 28); break;
      case POWER_ON_STATE: irsend.sendNEC(POWER_ON_SIGNAL, 28); break;
    }
    CONDITIONER_STATE = nextState;
    if (sendConditionerSignalLog) {
      Serial.println("Sto inviando un segnale");
      Serial.println(nextState);
    }
  }
}

byte nextConditionerStateCallbackLog = true;
void nextConditionerStateCallback() {
  int nextState;
  if (currentPersonCounter > 0 && currentHour > 13) {
    if (currentOutHumidex > 33 && currentRoomHumidex > 31)
      nextState = POWER_ON_STATE;
    else
      nextState = POWER_OFF_STATE;
  }
  else {
    nextState = POWER_OFF_STATE;
  }
  nextState = !CONDITIONER_STATE;
  if (nextConditionerStateCallback) { 
    Serial.print("Next state: ");
    Serial.println(nextState);
  }
  sendConditionerSignal(nextState);
  
}

void sendConfigurationCallback() {
  String configuration = "";

  configuration.concat("{");
  
  configuration.concat("humidexTarget:");
  configuration.concat(String(humidexTarget));

  configuration.concat("}");

  char data[configuration.length() + 1];
  configuration.toCharArray(data, configuration.length() + 1);
  client.publish(configurationTopic, data);
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
  pinMode(RECV_IR_PIN, INPUT);
  pinMode(SEND_IR_PIN, OUTPUT);  
}


void initObjects() {
  setup_wifi();
  
  client.setServer(mqtt_server, 1883);
  
  irrecv.enableIRIn();
  
  irsend.begin();

  timeClient.begin();

}

void setup() {
  Serial.begin(9600);
  initPin();
  initObjects();
}

void loop() {
  taskManager.execute();  
}
