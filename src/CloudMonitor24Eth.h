
/*
  CloudMonitor24Eth.h - Library to handle data to send into sd card
  Created by iMotion software, October 27, 2014.
  Released into the public domain.
*/

#ifndef CloudMonitor24Eth_h //this prevents problems if someone accidently #include's your library twice
#define CloudMonitor24Eth_h

#include <Arduino.h>
#include <DataFifo.h>
#include <Client.h>
#include <SPI.h>

//WARNING: don't change these values    Units
#define VAR_ID_TEMPERATURE        20   // °C
#define VAR_ID_SOLAR_RADIATON     21   // W/m2
#define VAR_ID_HUMIDITY           69   // %
#define VAR_ID_PRESSURE           70   // Bar
#define VARI_ID_SPEED              6
#define VARI_ID_CURRENT           22
#define VARI_ID_POWER             25

#define VARI_ID_LATITUDE           4
#define VARI_ID_LONGITUDE          5
#define VARI_ID_ALTITUDE          63
#define VARI_ID_GPS_HEADING        7
  
#define VAR_ID_DEVICE_DIG_IN1     52
#define VAR_ID_DEVICE_DIG_IN2     53
#define VAR_ID_DEVICE_DIG_IN3     54
#define VAR_ID_DEVICE_DIG_IN4     55

#define VAR_ID_DEVICE_ANALOG_IN1  56
#define VAR_ID_DEVICE_ANALOG_IN2  57
#define VAR_ID_DEVICE_ANALOG_IN3  58
#define VAR_ID_DEVICE_ANALOG_IN4  59
#define VAR_ID_DEVICE_ANALOG_IN5  64
#define VAR_ID_DEVICE_ANALOG_IN6  65
#define VAR_ID_DEVICE_ANALOG_IN7  66
#define VAR_ID_DEVICE_ANALOG_IN8  67

#pragma pack(1)

typedef struct 
{
  byte start;
  byte frame_type;
  byte identifier[16];
  byte passcode[16];
  uint32_t  timestamp;
  byte chk;
  byte end;
} Cm24FrameIdentifier;

typedef struct 
{
  byte start;
  byte data[15];
  byte chk;
  byte end;
} Cm24FrameData;

typedef struct
{
  byte start;
  byte frame_type;
  byte chk;
  byte end;
} Cm24FrameEmpty;

#pragma pack()

class CloudMonitor24Eth
{
  public:

    // Setup functions
    CloudMonitor24Eth(const char *username, const char *password);
    void setSdCardFolder(String sdCardFilePath);
    void begin(String sdCardFilePath, const int chipSelect, Client &channel);

    // Lopp functions
    void loop();
    boolean logVariable(uint32_t var_id, float value, uint16_t subcomp);
    boolean logAlarm(uint32_t alarm_id, float alarm_info, uint16_t subcomp);

  private:
    
    uint32_t getLininoTimestamp();
    byte readFrameFromSocket();
    boolean writeDataToNetwork(byte *buffer, byte size );
  	byte getChecksum(byte *buffer, byte size);
    void stateLogging();
    uint16_t sadd16(uint16_t a, uint16_t b);
    uint32_t sadd32(uint32_t a, uint32_t b);
    void calcLocalTimestamp();
    int freeRam();

    DataFifo *_fifo;
    Cm24FrameIdentifier _startFrame;
    Cm24FrameData _dataFrame;
    Cm24FrameEmpty _emptyFrame;
    Client *_channel;

    uint32_t _localTimestamp;
};


#endif