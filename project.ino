#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <TaskScheduler.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


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
 * 
 */

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
 * DHT settings
 */

DHT dht(D2, DHT22); //Inizializza oggetto chiamato "dht", parametri: pin a cui è connesso il sensore, tipo di dht 11/22

/*
 * TaskScheduler var
 */

Scheduler taskManager;


/*
 * Task declaration
 */

void temperature();
Task t1(2000, TASK_FOREVER, &temperature, &taskManager, true);


/*
 * Task implementation
 */
 
void temperature() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  //Stampa umidità e temperatura tramite monitor seriale
  Serial.print("Umidità: ");
  Serial.print(humidity);
  Serial.print(" %, Temp: ");
  Serial.print(temperature);
  Serial.println(" Celsius");
}



/*
 * INIT
 */

 
void initObjects() {
  dht.begin();
}

void setup() {
  Serial.begin(9600);
}

void loop() {
  taskManager.execute();
 

}
