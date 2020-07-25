/*  
 Test the M5.Lcd.print() viz embedded M5.Lcd.write() function

 This sketch used font 2, 4, 7

 Make sure all the display driver and pin comnenctions are correct by
 editting the User_Setup.h file in the TFT_eSPI library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
 #########################################################################
 */
#include <M5StickC.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define TFT_GREY 0x5AEB // New colour

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

const char* connectionTopic = "valerio/connection/display";

const char* roomTemperatureTopic = "valerio/room/temperature";
const char* roomHumidityTopic = "valerio/room/humidity";
const char* roomHeatIndexTopic = "valerio/room/heatIndex";
const char* roomHumidexTopic = "valerio/room/humidex";
const char* roomSetHumidexTargetTopic = "valerio/room/setHumidexTarget";
const char* roomConditionerStateTopic = "valerio/room/conditionerState";

const char* outTemperatureTopic = "valerio/out/temperature";
const char* outHumidityTopic = "valerio/out/humidity";
const char* outHeatIndexTopic = "valerio/out/heatIndex";
const char* outHumidexTopic = "valerio/out/humidex";

const char* personCounterTopic = "valerio/room/personCounter";

const char* configurationHumidexTopic = "valerio/configuration/humidexTarget";


PubSubClient client(espClient);

/*
 * display settings
 */
 
float humidexTarget = -1;
int conditionerState = -1;

float currentRoomTemperature = -1;
float currentRoomHumidity = -1;
float currentRoomHeatIndex = -1;
float currentRoomHumidex = -1;

float currentOutTemperature = -1;
float currentOutHumidity = -1;
float currentOutHeatIndex = -1;
float currentOutHumidex = -1;

float currentPersonCounter = -1;
int currentHour;
int currentMinute;




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
    
  else if (strcmp(configurationHumidexTopic, topic) == 0)
    humidexTarget = atof((char*)payload);
  else if (strcmp(roomConditionerStateTopic, topic) == 0)
    conditionerState = atof((char*)payload);
    

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

  refreshDisplay();
}

void reconnect() {
  while (!client.connected()) {
    
    String clientId = "ValerioESP8266ClientDisplay-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {

      client.publish(connectionTopic, "true");
      
      client.subscribe("valerio/#");
      client.setCallback(MQTTReceivedMessage);
    } else {
      delay(5000);
    }
  }
}


void MQTTLoop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void setup(void) {
  M5.begin();
  M5.Lcd.setRotation(1);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
}


void refreshDisplay() {
  M5.Lcd.fillScreen(TFT_BLACK);
  
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);  
  M5.Lcd.setTextSize(1);

  M5.Lcd.print(" target humidex: ");
  M5.Lcd.print(humidexTarget);
  M5.Lcd.println("C");

  M5.Lcd.print(" room humidex: ");
  M5.Lcd.print(currentRoomHumidex);
  M5.Lcd.println("C");

  M5.Lcd.print(" out humidex: ");
  M5.Lcd.print(currentOutHumidex);
  M5.Lcd.println("C");

  M5.Lcd.print(" person counter: ");
  M5.Lcd.println(currentPersonCounter);

  M5.Lcd.print(" conditioner state: ");
  M5.Lcd.println(conditionerState);
  
  M5.Lcd.println();
}

void loop() {
  MQTTLoop();
  delay(1000);
}
