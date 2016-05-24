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
 *
 * Documentation available at http://www.cloudmonitor24.com/it/iot/docs
 */

//Required
#include "cloudmonitor24.h"
#include "cloudmonitor24_alarms.h"
#include "cloudmonitor24_vars.h"

//Sketch's library
#include <Bridge.h>
#include <YunClient.h>

//Libraries for Sparkfun weather shield
#include <Wire.h> //I2C needed for sensors
#include "MPL3115A2.h" //Pressure sensor
#include "HTU21D.h" //Humidity sensor

//Constants
#define SENSOR_ID_HUM_TEMPERATURE     221  //hum_temp
#define SENSOR_ID_PRESS_TEMPERATURE   222   //press_temp
#define SENSOR_ID_HUMIDITY            223
#define SENSOR_ID_PRESSURE            224
#define SENSOR_ID_LIGHT               225
#define TIMEOUT_LOGGING             60000
#define TIMEOUT_SAMPLING             1000
#define READ_VALUE_ERROR               -1
#define BUILT_IN_LED_PIN               13
#define BUILT_IN_LED2_PIN               7
#define WSHIELD_SENSOR_LIGHT_PIN       A1
#define WSHIELD_3_3_V_PIN              A3 //3.3V pin for light/v partition

//Variables
MPL3115A2 sensorPressure; //Create an instance of the pressure sensor
HTU21D sensorHumidity; //Create an instance of the humidity sensor
float hum_temp = 0;
uint8_t hum_temp_samples = 0;
float press_temp = 0;
uint8_t press_temp_samples = 0;
float humidity = 0;
uint8_t humidity_samples = 0;
float pressure = 0;
uint8_t pressure_samples = 0;
float light;
uint8_t light_samples = 0;
long lastSample = 0;
YunClient *client;


void setup() 
{
  Bridge.begin();
  client = new YunClient();
  pinMode(BUILT_IN_LED_PIN, OUTPUT);
  pinMode(BUILT_IN_LED2_PIN, OUTPUT);
  digitalWrite(BUILT_IN_LED_PIN, HIGH);
  digitalWrite(BUILT_IN_LED2_PIN, HIGH);

  //Setup for Sparkfun weather shield
  pinMode(WSHIELD_SENSOR_LIGHT_PIN, INPUT); //Light pin sensor
  pinMode(WSHIELD_3_3_V_PIN, INPUT); //3.3V pin for light/v partition
  sensorPressure.begin();                // Get sensor online
  sensorPressure.setModeBarometer();     // Measure pressure in Pascals from 20 to 110 kPa
  sensorPressure.setOversampleRate(128); // Set Oversample to the recommended 128
  sensorPressure.enableEventFlags();     // Enable all three pressure and temp event flags 
  sensorHumidity.begin();

  // Initialize CM24 library
  cm24_init("PLANT IDENTIFIER","PLANT TOKEN","COMMAND TOKEN",client);
  cm24_register_command_callback(command_received);

  digitalWrite(BUILT_IN_LED_PIN, LOW);
  digitalWrite(BUILT_IN_LED2_PIN, LOW);
}

void loop() 
{
  if(millis() - lastSample >= TIMEOUT_SAMPLING)
  {
    digitalWrite(BUILT_IN_LED2_PIN, HIGH);
    
    lastSample = millis();
    
    float hum = sensorHumidity.readHumidity();
    if(hum != READ_VALUE_ERROR)
    {
      humidity += hum;
      humidity_samples++;  
    }    

    float htemp = sensorHumidity.readTemperature();
    if(htemp != READ_VALUE_ERROR)
    {
      hum_temp += htemp;
      hum_temp_samples++;  
    }
    
    float ptemp = sensorPressure.readTemp();
    if(ptemp != READ_VALUE_ERROR)
    {
      press_temp += ptemp;
      press_temp_samples++;  
    }
    
    float press = sensorPressure.readPressure() / 100;
    if(press != READ_VALUE_ERROR)
    {
      pressure += press;
      pressure_samples++;  
    }
    
    float lig = get_light_level();
    if(lig != READ_VALUE_ERROR)
    {
      light += lig;
      light_samples++;  
    }

    digitalWrite(BUILT_IN_LED2_PIN, LOW);
  }
  
  static uint32_t lastSaving = 0;

  if(millis() - lastSaving >= TIMEOUT_LOGGING)
  {
    lastSaving = millis();

    if(hum_temp_samples > 0)
    {
      cm24_log_variable( VAR_ID_TEMPERATURE, hum_temp/hum_temp_samples, SENSOR_ID_HUM_TEMPERATURE);
    }

    if(press_temp_samples > 0)
    {
      cm24_log_variable( VAR_ID_TEMPERATURE, press_temp/press_temp_samples, SENSOR_ID_PRESS_TEMPERATURE);
    }
    
    if(humidity_samples > 0)
    {
      cm24_log_variable( VAR_ID_HUMIDITY, humidity/humidity_samples, SENSOR_ID_HUMIDITY);
    }
    
    if(pressure_samples > 0)
    {
      cm24_log_variable( VAR_ID_PRESSURE, pressure/pressure_samples, SENSOR_ID_PRESSURE);
    }
    
    if(light_samples > 0)
    {
      cm24_log_variable( VAR_ID_AMBIENT_LIGHT, light/light_samples, SENSOR_ID_LIGHT);
    }

    //Reset    
    hum_temp = 0;
    hum_temp_samples = 0;
    
    press_temp = 0;
    press_temp_samples = 0;
    
    humidity = 0;
    humidity_samples = 0;
    
    pressure = 0;
    pressure_samples = 0;
    
    light = 0;
    light_samples = 0;
  }

  cm24_arduino_loop();
}

float get_light_level()
{
  float operatingVoltage = analogRead(WSHIELD_3_3_V_PIN);
  float lightSensor = analogRead(WSHIELD_SENSOR_LIGHT_PIN);
  operatingVoltage = 3.3 / operatingVoltage; //The reference voltage is 3.3V
  lightSensor = operatingVoltage * lightSensor;
  return(lightSensor);
}

void command_received(char *command)
{
  if(sizeof(command) > 0)
  {
    //sending 1 or a from CM24 platform will turn on build in led
    if((command[0] == '1') || (command[0] == 'a'))
    {
      digitalWrite(BUILT_IN_LED_PIN, HIGH);
    }

    //sending 0 or s from CM24 platform will turn on build in led
    else if((command[0] == '0') || (command[0] == 's'))
    {
      digitalWrite(BUILT_IN_LED_PIN, LOW);
    }
  }
}
