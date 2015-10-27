
#include <CloudMonitor24Eth.h>
#include <DataFifo.h>
#include <SdDataFifo.h>

#include <Ethernet.h>
#include <SD.h>
#include <SPI.h>
#include "DHT.h"

//Sub component values examples
#define SENSOR_ID_DEFAULT       0

#define PIN_SDCARD_CS               4
#define PIN_BOARD_STATUS            9
#define TIMEOUT_LOGGING             60000
#define TIMEOUT_SAMPLING            1000

//Pin sensors
#define DHT_PIN           A0  // what pin we're connected to
#define MOISTURE_PIN      A1
#define LUMINANCE_PIN     A2
#define DHTTYPE        DHT11  // DHT 11 

//Variables CM24
CloudMonitor24Eth cm24("YOUR-IDENTIFIER","YOUR-TOKEN");
EthernetClient client;
DHT dht(DHT_PIN, DHTTYPE);

float temperature = 0;
uint8_t temperature_samples = 0;
float humidity = 0;
uint8_t humidity_samples = 0;
float moisture = 0;
uint8_t moisture_samples = 0;
float light;
uint8_t light_samples = 0;
long lastSample = 0;
long lastSaving = 0;

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  pinMode(PIN_BOARD_STATUS, OUTPUT);

  // Initialize ethernet
  // assign a MAC address for the ethernet controller.
  // fill in your address here:
  byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0xC4, 0x5E};
  
  // fill in an available IP address on your network here,
  // for manual configuration:
  IPAddress ip(192,168,2,53);

  // fill in your Domain Name Server address here:
  IPAddress myDns(192,168,2,254);
  Ethernet.begin(mac,ip, myDns);

  // see if the card is present and can be initialized:
  cm24.begin("f", PIN_SDCARD_CS, client);  

}

void loop()
{
  
  static uint32_t lastSaving = 0;
  
  if(millis() - lastSample >= TIMEOUT_SAMPLING)
  {
        digitalWrite(PIN_BOARD_STATUS, HIGH);
    
        lastSample = millis();
        
        temperature += getTemperature();
        temperature_samples++;

        humidity += getHumidity();
        humidity_samples++;        
        
        moisture += getMoistureSensorValue(); //WARNING: getMoistureSensorValue return int, moisture is float !
        moisture_samples++;
        
        light += getLuminance(); //WARNING: getLuminance return int, moisture is float !
        light_samples++;
        
         digitalWrite(PIN_BOARD_STATUS, LOW);
  }
  
  // put your main code here, to run repeatedly:
  if(millis() - lastSaving >= TIMEOUT_LOGGING)
  {
        lastSaving = millis();
     
        if(!cm24.logVariable( VAR_ID_TEMPERATURE, temperature/temperature_samples, SENSOR_ID_STANZA))
        {
            Serial.println("Error log varTemp");
        }
        else
        {
          Serial.println("Logged temp");
        }

        if(!cm24.logVariable( VAR_ID_HUMIDITY, humidity/humidity_samples, SENSOR_ID_STANZA))
        {
            Serial.println("Error log varHum");
        }
        else
        {
          Serial.println("Logged humidity");
        }
        
        if(!cm24.logVariable( VAR_ID_HUMIDITY, moisture/moisture_samples, SENSOR_ID_VASO)) //TODO create subcomponent for moisture..
        {
            Serial.println("Error log varMoist");
        }
        else
        {
          Serial.println("Logged moist");
        }

        if(!cm24.logVariable( VAR_ID_SOLAR_RADIATON, light/light_samples, SENSOR_ID_STANZA))
        {
            Serial.println("Error log varLight");
        }else
        {
          Serial.println("Logged light");
        }

        //Reset
        temperature = 0;
        temperature_samples = 0;

        humidity = 0;
        humidity_samples = 0;       

        moisture = 0;
        moisture_samples = 0;

        light = 0;
        light_samples = 0;
   }

   cm24.loop();
}

int freeRam() 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

int getMoistureSensorValue()
{
    return analogRead(MOISTURE_PIN);
}

int getLuminance()
{
    return analogRead(LUMINANCE_PIN);
}

// Reading temperature or humidity takes about 250 milliseconds!
// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
float getHumidity()
{
  float h = dht.readHumidity();
  if (isnan(h)) 
    {
        return -1;
    } 
    else 
    {
        return h;
    }
}

float getTemperature()
{
  float t = dht.readTemperature();
  if (isnan(t)) 
    {
        return -1;
    } 
    else 
    {
        return t;
    }
}