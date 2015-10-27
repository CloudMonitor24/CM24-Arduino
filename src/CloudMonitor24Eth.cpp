#include <SdDataFifo.h>
#include <CloudMonitor24Eth.h>
#include <SD.h>
#include <SPI.h>

// Local defines
#define CM24_FRAME_TYPE_UNKNOWN         254
#define CM24_SOCKET_EMPTY               255
#define CM24_FRAME_TYPE_IDENTIFYING     1
#define CM24_FRAME_TYPE_KEEP_ALIVE      2
#define CM24_FRAME_TYPE_ACK             3
#define CM24_FRAME_TYPE_NOT_ACK         4
#define CM24_FRAME_TYPE_TIME_REQUEST    5
#define CM24_FRAME_TYPE_TIME_ANSWER     6

#define CM24_FRAME_TYPE_ALARM           100
#define CM24_FRAME_TYPE_VARIABLE        101

#define CM24_MAGIC_START_CH             0x55
#define CM24_MAGIC_END_CH               0xAA

#define CM24_SOCKET_URL                 "data.cloudmonitor24.com"
#define CM24_SOCKET_PORT                3503
#define CM24_TIMEOUT_SEND_KEEP_ALIVE    240000  // millis (4 minutes)
#define CM24_TIMEOUT_WAIT_ACK           10000   // millis
#define CM24_TIMEOUT_WAIT_RECONNECT     60000   // millis

#define FRAME_DATA_SIZE     14

typedef enum
{
  CM24_WAIT_CONNECT,
  CM24_SEND_DATA,
  CM24_WAIT_ACK,
  CM24_WAIT_RECONNECT
} DataLoggerState_e;

typedef enum
{
  CM24_SOCKET_WAIT_START,
  CM24_SOCKET_WAIT_FRAME_TYPE,
  CM24_SOCKET_CALCULATE_CHECKSUM,
  CM24_SOCKET_WAIT_END
} SocketReadBufferState_e;

CloudMonitor24Eth::CloudMonitor24Eth(const char *identifier, const char *token)
{
    int i;

    // Init the start frame
    _startFrame.start = CM24_MAGIC_START_CH;
    _startFrame.frame_type = CM24_FRAME_TYPE_IDENTIFYING;
    _startFrame.end = CM24_MAGIC_END_CH;

    // Init data frame
    _dataFrame.start = CM24_MAGIC_START_CH;
    _dataFrame.end = CM24_MAGIC_END_CH;

    // Fill identifier
    for(i=0; i<strlen(identifier); i++)
    {
        if (i>=16)
        {
            break;
        }
        _startFrame.identifier[i] = identifier[i];
    }
    for(; i<16; i++)
    {
        _startFrame.identifier[i] = '\0';   
    }

    // Fill token
    for(i=0; i<strlen(token); i++)
    {
        if (i>=16)
        {
            break;
        }
        _startFrame.passcode[i] = token[i];
    }
    for(; i<16; i++)
    {
        _startFrame.passcode[i] = '\0';   
    }

    //Init empty frame
    _emptyFrame.start = CM24_MAGIC_START_CH;
    _emptyFrame.end = CM24_MAGIC_END_CH;

}

void CloudMonitor24Eth::loop()
{
    static DataLoggerState_e _cm24LoggerState = CM24_WAIT_CONNECT;
    
    static long startWaitingTimeAck = 0;
    static long startWaitingTimeReconnect = 0;

    static byte lastFrameSizeSent = 0;
    static long lastTimeDataSent = 0;
    static byte lastFrameTypeSent;

    // Calculate current timestamp, depending on delta between current and previous reading
    calcLocalTimestamp();

    switch(_cm24LoggerState)
    {
        case CM24_WAIT_CONNECT:
        {
            if(_channel->connected())
            {                
                _channel->flush();
                // Launch identification packet
                _startFrame.timestamp = _localTimestamp;
                _startFrame.chk = getChecksum( &_startFrame.frame_type, sizeof(Cm24FrameIdentifier)-3 );
                if (_channel->write((byte *)&_startFrame, sizeof(Cm24FrameIdentifier)) > 0)
                {                    
                    startWaitingTimeAck = millis();
                    lastFrameTypeSent = _startFrame.frame_type;
                    _cm24LoggerState = CM24_WAIT_ACK;
                    lastTimeDataSent = millis();
                }
                else
                {
                    //try send identifing next loop..
                }
            }
            else
            {
                //Try connect
                if (!_channel->connect(CM24_SOCKET_URL, CM24_SOCKET_PORT))
                {
                    _channel->stop();
                    //Do nothing, try again next time WARNING LOOP
                    _cm24LoggerState = CM24_WAIT_RECONNECT;
                    startWaitingTimeReconnect = millis();                    
                }
                else
                {
                    //channel-> CONNECTED!
                }
            }
            break;
        }

        case CM24_SEND_DATA:
        {
            if(_channel->connected())
            {
                //Check for alarms
                byte size;
                byte *buffer = _fifo_alarms->pop_data(&size);
                byte frameType = CM24_FRAME_TYPE_ALARM;

                if(!(size > 0)) //alarms not available? then pop var
                {
                    buffer = _fifo_vars->pop_data(&size);
                    frameType = CM24_FRAME_TYPE_VARIABLE;
                }
                
                if(size > 0) //data available?
                {                    
                    if(writeDataToNetwork(buffer, size, frameType))
                    {
                        //data sent successfully
                        _cm24LoggerState = CM24_WAIT_ACK;
                        startWaitingTimeAck = millis();
                        
                        //update sent time
                        lastTimeDataSent = millis();
                        
                        //update sent size
                        lastFrameSizeSent = size;

                        //save last frame-type sent, reading on "popped" data
                        lastFrameTypeSent = buffer[0];
                    }
                    else
                    {
                        //Try again next loop.. WARNING LOOP
                    }
                }
                else
                {
                    //No data to send, handle timeout for keep alive
                    if( (millis() - lastTimeDataSent) >= CM24_TIMEOUT_SEND_KEEP_ALIVE)
                    {
                        //send keep alive
                        _emptyFrame.frame_type = CM24_FRAME_TYPE_KEEP_ALIVE;
                        _emptyFrame.chk = getChecksum( &_emptyFrame.frame_type, sizeof(Cm24FrameEmpty)-3 );
                        
                        if (_channel->write((byte *)&_emptyFrame, sizeof(Cm24FrameEmpty)) > 0)
                        {
                            size = 0;
                            _cm24LoggerState = CM24_WAIT_ACK;
                            startWaitingTimeAck = millis();
                            lastFrameTypeSent = CM24_FRAME_TYPE_KEEP_ALIVE;
                            lastTimeDataSent = millis();

                        }
                        else
                        {
                            //unable to send keep alive.. go to wait connect? WARNING LOOP
                            _cm24LoggerState = CM24_WAIT_RECONNECT;
                        }                        
                    }
                    else
                    {
                        //loop here until timeout fire..
                    }
                }
            }
            else
            {
                _cm24LoggerState = CM24_WAIT_CONNECT;
            }
            break;
        }

        case CM24_WAIT_ACK:
        {
            if(_channel->connected())
            {
                long timeFromStartWaiting =  millis() - startWaitingTimeAck;
                //loop until timeout fire
                if( ( timeFromStartWaiting ) >= CM24_TIMEOUT_WAIT_ACK)
                {
                    //timeout fire.. goto send_data
                    _cm24LoggerState = CM24_SEND_DATA;
                }
                else
                {
                    byte receivedFrameType = readFrameFromSocket();
                    
                    //check for frame type received
                    switch(receivedFrameType)
                    {
                        case CM24_SOCKET_EMPTY:
                        {
                            //Still waiting for something..
                            _cm24LoggerState = CM24_WAIT_ACK;
                            break;
                        }
                        case CM24_FRAME_TYPE_UNKNOWN:
                        {
                            //TODO WARNING.. why received unknown frame ???
                            //WHAT TO DO NOW?
                            //_cm24LoggerState = CM24_SEND_DATA;
                            break;
                        }
                        case CM24_FRAME_TYPE_ACK:
                        {
                            //swith possible frame sent
                            switch(lastFrameTypeSent)
                            {
                                case CM24_FRAME_TYPE_IDENTIFYING:
                                {                                    
                                    break;
                                }
                                case CM24_FRAME_TYPE_KEEP_ALIVE:
                                {
                                    break;
                                }
                                case CM24_FRAME_TYPE_ALARM:
                                {
                                    _fifo_alarms->increment_pointer(lastFrameSizeSent);
                                    break;
                                }
                                case CM24_FRAME_TYPE_VARIABLE:
                                {
                                    _fifo_vars->increment_pointer(lastFrameSizeSent);
                                    break;
                                }
                                
                                /*
                                Features.. right now we never send these frames to server
                                case CM24_FRAME_TYPE_COMMAND:
                                {
                                    break;
                                }
                                case CM24_FRAME_TYPE_TIME_REQUEST:
                                {
                                    break;
                                }*/
                               
                            }
                            
                            _cm24LoggerState = CM24_SEND_DATA;

                            break;
                        }
                        case CM24_FRAME_TYPE_NOT_ACK:
                        {
                            //WARNING: something wrong on server side.. don't increment pointer, data have to re-send
                            _cm24LoggerState = CM24_SEND_DATA;
                            break;
                        }
                    }                    
                }
            }
            else
            {
                _cm24LoggerState = CM24_WAIT_RECONNECT;
                startWaitingTimeReconnect = millis();
            }
            
            break;
        }

        case CM24_WAIT_RECONNECT:
        {          
            if( (millis() - startWaitingTimeReconnect) > CM24_TIMEOUT_WAIT_RECONNECT )
            {
                _cm24LoggerState = CM24_WAIT_CONNECT;
            }
            else
            {
                //loop here until timeout fire.. 
            }

            break;
        }
    }
}

boolean CloudMonitor24Eth::begin(String sdCardFilePath, const int chipSelect, Client &client)
{
    //Add end slash if not exist
    if (sdCardFilePath.endsWith("/"))
    {
        sdCardFilePath.remove( sdCardFilePath.length() - 1 );
    }

    // see if the card is present and can be initialized
    // NOTE: SD.begin have to be called only one time in all application
    if(SD.begin(chipSelect))
    {
        _fifo_alarms = (SdDataFifo *) new SdDataFifo(sdCardFilePath+"/ALARMS");
        _fifo_vars = (SdDataFifo *) new SdDataFifo(sdCardFilePath+"/VARS");
        _channel = &client;
        return true;
    }
    else
    {
        return false;
    }
}

boolean CloudMonitor24Eth::logVariable(uint32_t var_id, float value, uint16_t subcomp)
{
    static byte buffer[FRAME_DATA_SIZE];

    // Subcomp
    buffer[0] = subcomp & 0xFF;
    buffer[1] = (subcomp >> 8) & 0xFF;

    // Var id
    buffer[2] = var_id & 0xFF;
    buffer[3] = (var_id >> 8) & 0xFF;
    buffer[4] = (var_id >> 16) & 0xFF;
    buffer[5] = (var_id >> 24) & 0xFF;

    // Value
    byte *float_pt = (byte *) &value;
    buffer[6] = *float_pt++;
    buffer[7] = *float_pt++;
    buffer[8] = *float_pt++;
    buffer[9] = *float_pt;

    // Timestamp
    uint32_t time = _localTimestamp;
    buffer[10] = time & 0xFF;
    buffer[11] = (time >> 8) & 0xFF;
    buffer[12] = (time >> 16) & 0xFF;
    buffer[13] = (time >> 24) & 0xFF;

    return _fifo_vars->push_data( buffer, FRAME_DATA_SIZE );
}
    
boolean CloudMonitor24Eth::logAlarm(uint32_t alarm_id, uint32_t alarm_info, uint16_t subcomp)
{
    byte buffer[FRAME_DATA_SIZE];

    // Subcomp
    buffer[0] = subcomp & 0xFF;
    buffer[1] = (subcomp >> 8) & 0xFF;

    // Alarm id
    buffer[2] = alarm_id & 0xFF;
    buffer[3] = (alarm_id >> 8) & 0xFF;
    buffer[4] = (alarm_id >> 16) & 0xFF;
    buffer[5] = (alarm_id >> 24) & 0xFF;

    // Alarm Info
    byte *float_pt = (byte *) &alarm_info;
    buffer[6] = *float_pt++;
    buffer[7] = *float_pt++;
    buffer[8] = *float_pt++;
    buffer[9] = *float_pt;

    // Timestamp
    uint32_t time = _localTimestamp;
    buffer[10] = time & 0xFF;
    buffer[11] = (time >> 8) & 0xFF;
    buffer[12] = (time >> 16) & 0xFF;
    buffer[13] = (time >> 24) & 0xFF;

    return _fifo_alarms->push_data( buffer, FRAME_DATA_SIZE );
}

void CloudMonitor24Eth::calcLocalTimestamp()
{
    static uint32_t last_millis_time = 0;
    uint32_t curr_millis_time = millis();
    uint32_t delta_millis_time = curr_millis_time - last_millis_time;    

    if (delta_millis_time < 1000)
    {
        return;
    }

    _localTimestamp += delta_millis_time / 1000;
    last_millis_time = curr_millis_time;
    last_millis_time -= delta_millis_time % 1000;
}

//Temporary return received frame type, will return frame_type and value
byte CloudMonitor24Eth::readFrameFromSocket()
{
    static SocketReadBufferState_e socketReadBufferState = CM24_SOCKET_WAIT_START;
 
    static byte byteRead;

    static byte arrayReceivedFrame[sizeof(Cm24FrameEmpty)-3]; //to update with max possible frame size
    static byte receivedFrameSize;

    byte frameTypeReceived = CM24_SOCKET_EMPTY;
    uint16_t avail = _channel->available();

    //loop into all buffer
    //TODO: FIND WHERE IS DEFINED TRANSFER_TIMEOUT out of bridge.. while(avail > 0 && avail != BridgeClass::TRANSFER_TIMEOUT)
    while(avail > 0)
    {
        switch(socketReadBufferState)
        {
            case CM24_SOCKET_WAIT_START :
            {
                byteRead = _channel->read();
                if(byteRead == CM24_MAGIC_START_CH)
                {
                    //Byte Frame start FOUND!
                    socketReadBufferState = CM24_SOCKET_WAIT_FRAME_TYPE;                 
                }                

                break;
            }
            case CM24_SOCKET_WAIT_FRAME_TYPE :
            {
                byteRead = _channel->read();
                switch(byteRead)
                {
                    case CM24_FRAME_TYPE_ACK:
                    {
                        socketReadBufferState = CM24_SOCKET_CALCULATE_CHECKSUM;
                        
                        frameTypeReceived = CM24_FRAME_TYPE_ACK;                        
                        
                        arrayReceivedFrame[0] = frameTypeReceived;
                        
                        break;
                    }

                    case CM24_FRAME_TYPE_NOT_ACK:
                    {
                        socketReadBufferState = CM24_SOCKET_CALCULATE_CHECKSUM;
                        
                        frameTypeReceived = CM24_FRAME_TYPE_NOT_ACK;
                        
                        arrayReceivedFrame[0] = frameTypeReceived;
                        
                        break;
                    }

                    //Features CM24_FRAME_TYPE_TIME_ANSWER..
                    /*case CM24_FRAME_TYPE_TIME_ANSWER:
                    {
                        for(int f = 0; f < (sizeof(Cm24FrameAnswer)-3); f++)
                        {

                        }
                        socketReadBufferState = CM24_SOCKET_READ_ALL_FRAME;
                        break;
                    }*/

                    default:
                    {
                        socketReadBufferState = CM24_SOCKET_WAIT_START;
                        //TODO: reset array -> arrayReceivedFrame = NULL;

                        break;
                    }                
                }
                break;
            }

            /*features TODO 
            case CM24_SOCKET_READ_ALL_FRAME:
            {
                socketReadBufferState = CM24_SOCKET_WAIT_START;
            }*/
            
            case CM24_SOCKET_CALCULATE_CHECKSUM :
            {                
                byteRead = _channel->read();


                //Calculate checksum
                byte receivedFrameChecksum = getChecksum(arrayReceivedFrame, 1 ); //1 lenght Temporary, we receive only pack without frame
                if(receivedFrameChecksum == byteRead)
                {
                    //Good checksum check for frame magic end
                    socketReadBufferState = CM24_SOCKET_WAIT_END;
                }
                else
                {
                    //something wrong in frame received..
                    socketReadBufferState = CM24_SOCKET_WAIT_START;
                    //TODO: reset array -> arrayReceivedFrame = NULL;
                    frameTypeReceived = CM24_FRAME_TYPE_UNKNOWN;
                }
                break;
            }
            
            case CM24_SOCKET_WAIT_END :
            {
                byteRead = _channel->read();
                if(byteRead == CM24_MAGIC_END_CH)
                {
                    //Good frame parsed!

                    //Here, We will have to set pointer of frame-value too, before return frametype

                    //Reset socket parser state..
                    socketReadBufferState = CM24_SOCKET_WAIT_START;
                    
                    return frameTypeReceived;
                 
                }
                else
                {
                    //something wrong in frame received..
                    socketReadBufferState = CM24_SOCKET_WAIT_START;
                    //TODO: reset array -> arrayReceivedFrame = NULL;
                    frameTypeReceived = CM24_FRAME_TYPE_UNKNOWN;
                }
                break;
            }
        }

        avail = _channel->available();
    }
    
    return frameTypeReceived;
}

boolean CloudMonitor24Eth::writeDataToNetwork(byte *buffer, byte size, byte ftype)
{
    // Data should always be FRAME_DATA_SIZE bytes!
    if (size != sizeof(Cm24FrameData)-4)
    {
        return false;
    }

    // Write data
    for(int t=0; t<size; t++)
    {
        _dataFrame.data[t] = buffer[t];
    }

    _dataFrame.ftype = ftype;
    _dataFrame.chk = getChecksum( &(_dataFrame.ftype), sizeof(Cm24FrameData)-3  );

    if (_channel->write((byte *)&_dataFrame, sizeof(Cm24FrameData)) == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

byte CloudMonitor24Eth::getChecksum(byte *buffer, byte size)
{
    byte ret = 0;
    for(size_t i=0; i<size; i++)
    {
        ret ^= buffer[i];
    }
    return ret;
}

uint16_t CloudMonitor24Eth::sadd16(uint16_t a, uint16_t b)
{ 
    return (a > 0xFFFF - b) ? 0xFFFF : a + b; 
}

uint32_t CloudMonitor24Eth::sadd32(uint32_t a, uint32_t b)
{ 
    return (a > 0xFFFFFFFF - b) ? 0xFFFFFFFF : a + b;
}

int CloudMonitor24Eth::freeRam() 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
