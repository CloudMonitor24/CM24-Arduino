/* 
 * CloudMonitor24
 *
 * Copyright (c) 2016  Federico Mosca <federico.mosca@imotion.it>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

//Required
#include "cloudmonitor24.h"
#include "cloudmonitor24_alarms.h"
#include "cloudmonitor24_vars.h"

//Sketch's library
#include <SPI.h>
#include <Ethernet.h> //Example using Arduino Ethernet and one DHT11 sensor
#include "DHT.h"

//Constants
#define SENSOR_ID_TEMPERATURE       100
#define SENSOR_ID_HUMIDITY          101
#define TIMEOUT_LOGGING             60000
#define TIMEOUT_SAMPLING            1000
#define READ_VALUE_ERROR            -1

#define BUILT_IN_LED_PIN    9
#define DHT_PIN            A0
// Uncomment whatever type you're using!
#define DHT_TYPE DHT11     // DHT 11 
//#define DHT_TYPE DHT22   // DHT 22  (AM2302)
//#define DHT_TYPE DHT21   // DHT 21 (AM2301)

//Variables
EthernetClient client;
DHT dht(DHT_PIN, DHT_TYPE);
float sensor_temp = 0;
uint8_t sensor_temp_samples = 0;
float sensor_hum = 0;
uint8_t sensor_hum_samples = 0;
long lastSample = 0;


void setup() 
{
  pinMode(BUILT_IN_LED_PIN, OUTPUT);

  // Initialize ethernet
  byte macAddr[] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC }; //physical mac address
  byte ipAddr[] = { 192, 168, 1, 20 }; // ip in lan assigned to arduino
  byte dnsAddr[] = { 8, 8, 8, 8 }; // ip in lan assigned to arduino
  byte gatewayAddr[] = { 192, 168, 1, 1 }; // internet access via router
  byte subnetAddr[] = { 255, 255, 255, 0 }; //subnet mask
  Ethernet.begin(macAddr, ipAddr, dnsAddr, gatewayAddr, subnetAddr);
  
  // Initialize CM24 library
  cm24_init("PLANT IDENTIFIER","PLANT TOKEN","COMMAND TOKEN",&client);
  cm24_register_command_callback(command_received);
}

void loop()
{
  if(millis() - lastSample >= TIMEOUT_SAMPLING)
  {
    digitalWrite(BUILT_IN_LED_PIN, HIGH);

    lastSample = millis();
    
    float temp = getTemperatureFromDHT();
    if(temp != READ_VALUE_ERROR)
    {
      sensor_temp += temp;
      sensor_temp_samples++;  
    }
    
    float hum = getHumidityFromDHT();
    if(hum != READ_VALUE_ERROR)
    {
      sensor_hum += getHumidityFromDHT();
      sensor_hum_samples++;
    }

    digitalWrite(BUILT_IN_LED_PIN, LOW);
  }
  
  static uint32_t lastSaving = 0;

  if(millis() - lastSaving >= TIMEOUT_LOGGING)
  {
    lastSaving = millis();

    if(sensor_temp_samples > 0)
    {
      cm24_log_variable( VAR_ID_TEMPERATURE, sensor_temp/sensor_temp_samples, SENSOR_ID_TEMPERATURE);
    }

    if(sensor_hum_samples > 0)
    {
      cm24_log_variable( VAR_ID_HUMIDITY, sensor_hum/sensor_hum_samples, SENSOR_ID_HUMIDITY);
    }

    //Reset
    sensor_temp = 0;
    sensor_temp_samples = 0;
    sensor_hum = 0;
    sensor_hum_samples = 0;
   }
   cm24_arduino_loop();
}

float getTemperatureFromDHT()
{
  float t = dht.readTemperature();

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t))
  {
    //Failed to read from DHT
    return READ_VALUE_ERROR;
  } 
  else 
  {
    return t;
  }
}

float getHumidityFromDHT()
{
  float h = dht.readHumidity();

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(h)) 
  {
    //Failed to read from DHT
    return READ_VALUE_ERROR;
  } 
  else 
  {
    return h;
  }
}

void command_received(char *test)
{
    //test will contain string sended by cm24 platform
}