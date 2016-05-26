# CloudMonitor24 Arduino Library
CloudMonitor24 is fully compatible with any Arduino-compatible board. Integrating CloudMonitor24 Arduino Library into your project, you can add very powerful features like:<br>
* Real time logging of variables
* Alerting and notification in case of any alarm or abnormal situation
* Remote commands

It requires a valid CloudMonitor24 account.<br>
Please read [general concepts about CloudMonitor24 platform](http://www.cloudmonitor24.com/it/iot/docs/#concepts) before proceeding.

## Requirements
CloudMonitor24 Arduino Library is compatible with any Arduino board. Due to the fact the board must be able to communicate with the platform, an Internet connection is required. You can connect your Arduino board to the Internet in any way:<br>
* with an official or unofficial Ethernet shield
* with any WiFi shield or in general any WiFi module
* with any board that supports Ethernet or WiFi natively (like Arduino Yun, Arduino Ethernet or similars)

## Getting started
In order to use CloudMonitor24 Arduino library you should install a valid Arduino IDE on your computer.<br>Both *Arduino.cc IDE* and *Arduino.org IDE* are fully supported.
Once IDE is working and can communicate with your Arduino board, follow these instructions to install CloudMonitor24 Arduino Library:<br>
1. Clone or download [CloudMonitor Arduino Library from Github](https://github.com/CloudMonitor24/CM24-Arduino).<br>
2. Install library into your IDE. For detailed information about adding an external library just follow [this link](https://www.arduino.cc/en/Guide/Libraries).<br>
3. Restart your IDE and check if library is successfully detected.<br>
4. Go to *File -> Example* and load one of the samples contained into the the library for quick information about how to use it.

## Standard usage
For a standard usage, CloudMonitor24 Arduino Library requires an instance of a generic *Client* object in order to connect to the Internet. Any class that extends *Client* can work properly (like EthernetClinet or WiFiClient).

###### Init function
```c++
// This will init the library
cm24_init("MY_PLANT_IDENTIFIER", "MY_PLANT_TOKEN", "MY_COMMAND_TOKEN", clientObjectInstance);

// This is for setting a specific callback function called once Arduino board receives a remote command
cm24_register_command_callback( my_callback_function );
```
Init of the library should be done within *setup* function of your sketch. This is the place where you have to specify your plant identifier and tokens. During the setup execution you can also specifiy a custom remote command callback, which is a function called every time CloudMonitor24 platform sends a command to the board.


###### Loop function
```c++
void loop()
{
	// Don't block loop in any way
	cm24_arduino_loop();
}
```
CloudMonitor24 loop function must be called within standard sketch loop function. This is a mandatory requirements, because loop function is responsible to transfer local data to the cloud platform. This function has an internal timing, so it should be called at the maximum speed possibile.

**NOTE**: try to avoid as much as possible any blocking operation within the main sketch loop function. If loop function is blocked by any other function, CloudMonitor24 Arduino Library cannot transfer local data to the platform nor receive any command.


###### Logging variables or alarms
```c++
cm24_log_variable( uint32_t VARIABLE_ID, float value, uint16_t sensor_id );
cm24_log_alarm( uint32_t ALARM_ID, float value, uint16_t sensor_id );
```
Logging variables and alarms have not been so easy! Just call *cm24_log_variable* and *cm24_log_alarm* function. The library will localy save the value of the variable or the code of the alarm and transfer it to the cloud platform as soon as possibile.

**NOTE**: you should always place a delay between two log function calls. Logging at the maximum speed without any delay will lead to fill local buffer and lose data.


###### Receiving remote commands
```c++
void setup()
{
	....
	cm24_register_command_callback( callback_function );
}

void callback_function( char *command )
{
	// This function will be called every time you get a new command.
}
```
Remote commands are a very interesting feature of the library, because they grant you control of your Arduino board from any place of the world. Remote commands are represented by standard text strings. It's in charge to your application to understand the command and execute it.<br>

In order to receive remote commands, you firstly have to register a callback function during the setup process. This function will be called every time your board receives a remote command.<br>

If you want to disable remote commands you can both avoid to specifiy the command callback or pass an empty command token to the *cm24_init* function.
