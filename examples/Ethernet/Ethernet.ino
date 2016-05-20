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
#include <Ethernet.h> //Example using Arduino Ethernet

//Constants
#define TIMEOUT_LOGGING             60000

//Variables
long lastSaving = 0;
EthernetClient client;


void setup() 
{  
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
  if(millis() - lastSaving >= TIMEOUT_LOGGING)
  {
    lastSaving = millis();
    cm24_log_variable( VAR_ID_TEMPERATURE, 1.0f, 0);
  }
   
  cm24_arduino_loop();
}

void command_received(char *test)
{
    //test will contain string sended by cm24 platform
}