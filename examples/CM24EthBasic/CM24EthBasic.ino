
#include <CloudMonitor24Eth.h>
#include <DataFifo.h>
#include <SdDataFifo.h>

#include <Ethernet2.h> //Example using Ethernet shield v.2
#include <SD.h>
#include <SPI.h>

//Sub component values examples
#define SENSOR_ID_1          1

#define PIN_SDCARD_CS               4 //Arduino Ethernet chip select
#define PIN_BOARD_STATUS            9
#define TIMEOUT_LOGGING             60000
#define TIMEOUT_SAMPLING            1000

//Pin sensors
#define PIN_SENSOR_1            A0

//Variables CM24
CloudMonitor24Eth cm24("YOUR-IDENTIFIER","YOUR-TOKEN");
EthernetClient client;

float sensor_1 = 0;
uint8_t sensor_1_samples = 0;
long lastSample = 0;
long lastSaving = 0;



void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);

  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }
  
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  pinMode(PIN_BOARD_STATUS, OUTPUT);

  // Initialize ethernet
  // assign a MAC address for the ethernet controller.
  // fill in your address here:
  byte mac[] = { 0xYO, 0xUR, 0xMA, 0xCH, 0xER, 0xE0};
  
  // fill in an available IP address on your network here:
  IPAddress ip(192,168,X,XXX);

  IPAddress myDns(8,8,8,8);
  // fill in with your gateway IP address here:
  IPAddress gateway(192,168,X,XXX);
  // fill in with your subnet here:
  IPAddress subnet(255,255,255,0);
  
  Ethernet.begin(mac, ip, myDns, gateway, subnet);
  // see if the card is present and can be initialized, NOTE: main path have to already exist
  if(!cm24.begin("CM24/", PIN_SDCARD_CS, client))
  {
    Serial.println("WARNING SD initialization failed! CM24 library need it to be able to to work.");
  }
}

void loop()
{
  
  static uint32_t lastSaving = 0;
  
  if(millis() - lastSample >= TIMEOUT_SAMPLING)
  {
        digitalWrite(PIN_BOARD_STATUS, HIGH);
    
        lastSample = millis();
        
        sensor_1 += analogRead(PIN_SENSOR_1);
        sensor_1_samples++;
        
        digitalWrite(PIN_BOARD_STATUS, LOW);
  }
  
  // put your main code here, to run repeatedly:
  if(millis() - lastSaving >= TIMEOUT_LOGGING)
  {
        lastSaving = millis();
     
        if(!cm24.logVariable( VAR_ID_TEMPERATURE, sensor_1/sensor_1_samples, SENSOR_ID_1))
        {
            //Error log variable
        }

        /*if(!cm24.logAlarm( ALARM_ID_USER_ALARM, 20, SENSOR_ID_1))
        {
            //Error logging alarm
        }*/

        //Reset
        sensor_1 = 0;
        sensor_1_samples = 0;
   }
   cm24.loop();
}
