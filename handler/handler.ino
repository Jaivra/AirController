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
 * personCounter setting
 */

const int PERSON_IN_PIN = D1;
const int PERSON_OUT_PIN = D2;

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

const char* connectionTopic = "valerio/connectedHandler";

const char* roomTemperatureTopic = "valerio/room/temperature";
const char* roomHumidityTopic = "valerio/room/humidity";
const char* roomHeatIndexTopic = "valerio/room/heatIndex";
const char* roomHumidexTopic = "valerio/room/humidex";

const char* outTemperatureTopic = "valerio/out/temperature";
const char* outHumidityTopic = "valerio/out/humidity";
const char* outHeatIndexTopic = "valerio/out/heatIndex";
const char* outHumidexTopic = "valerio/out/humidex";

const char* personCounterTopic = "valerio/room/personCounter";
PubSubClient client(espClient);



/*
 * IR settings
 */

const int RECV_IR_PIN = D6;
const int SEND_IR_PIN = D5;    

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

int POWER_ON = 0x880064A;
int POWER_OFF = 0x88C0051;


/*
 * NextState settings
 */
float humidexTarget;


float currentRoomTemperature;
float currentRoomHumidity;
float currentRoomHeatIndex;
float currentRoomHumidex;

float currentOutTemperature;
float currentOutHumidity;
float currentOutRoomHeatIndex;
float currentOutHumidex;

float currentPersonCounter;
float currentHour;

const int POWER_OFF_STATE = 0;
const int POWER_ON_STATE = 1;
 
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

void sendPersonCounterCallback();
Task sendPersonCounterTask(1000 * 5, TASK_FOREVER, &sendPersonCounterCallback, &taskManager, true);

void personCounterRestartCallback();
Task personCounterRestartTask(1500, TASK_FOREVER, &personCounterRestartCallback, &taskManager);

void MQTTLoopCallback();
Task MQTTLoopTask(1000 * 2, TASK_FOREVER, &MQTTLoopCallback, &taskManager, true);

void IRRecvCallback();
Task IRRecvTask(300, TASK_FOREVER, &IRRecvCallback, &taskManager, false);

void IRSendCallback();
Task IRSendTask(300, TASK_FOREVER, &IRSendCallback, &taskManager, false);

void nextStateCallback();
Task nextStateTask(1000 * 10, TASK_FOREVER, &nextStateCallback, &taskManager, true);


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
    startPersonCounterCountDown(1200);
  }
  
  if (personCounterInCallbackLog) {
    Serial.print("PersonCounter sensor IN Excited");

    Serial.print("PersonCounter State: ");
    Serial.println(PERSON_COUNTER_STATE);
  }

  calculateNextState();
}

byte personCounterOutCallbackLog = true;
void ICACHE_RAM_ATTR personCounterOutCallback() {
  if (PERSON_COUNTER_STATE == PERSON_COUNTER_INCREMENTED)
    return;
    
  if (PERSON_COUNTER_STATE == PERSON_COUNTER_IN_EXCITED)
    PERSON_COUNTER_STATE = PERSON_COUNTER_EXIT;
  else {
    PERSON_COUNTER_STATE = PERSON_COUNTER_OUT_EXCITED;
    startPersonCounterCountDown(1200);
  }
  
  if (personCounterOutCallbackLog) {
    Serial.print("PersonCounter sensor OUT excited: ");
    
    Serial.print("PersonCounter State: ");
    Serial.println(PERSON_COUNTER_STATE);
  } 

  calculateNextState();
}


byte calculateNextStateLog = true;
void calculateNextState() {
  if (PERSON_COUNTER_STATE == PERSON_COUNTER_JOIN) {
    personCount += 1;
    PERSON_COUNTER_STATE = PERSON_COUNTER_IDLE;
    startPersonCounterCountDown(3000);
  }
  else if (PERSON_COUNTER_STATE == PERSON_COUNTER_EXIT) {
    personCount -= 1;
    PERSON_COUNTER_STATE = PERSON_COUNTER_IDLE;
    startPersonCounterCountDown(3000);    
  }

  if (calculateNextStateLog) {
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

const byte MQTTLoopCallbackLog = true;
void MQTTLoopCallback() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}


void MQTTReceivedMessage(char* topic, byte* payload, unsigned int length) {


  
  if (strcmp(personCounterTopic, topic) == 0) {
    char* PauseStr;
    PauseStr = (char*)payload;
    float value = atof((char*)payload);
    
    
    Serial.print("Person Counter");
    Serial.println(value);
  }
  else
    Serial.print("Message arrived in topic: ");
  Serial.print("*");
  Serial.print(topic);
  Serial.println("*");

  Serial.print("*");
  Serial.print(personCounterTopic);
  Serial.println("*");
  
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
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
 * NextStateTask and functions
 */

void nextStateCallback() {
  int nextState;
  if (currentPersonCounter > 0) {
      
  }
  else {
    nextState = POWER_OFF;
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
  
  irrecv.enableIRIn();
  
  irsend.begin();
}

void setup() {
  Serial.begin(9600);
  initPin();
  initObjects();
}

void loop() {
  taskManager.execute();  
}