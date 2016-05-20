/* 
 * cloudmonitor24_platform_prot.cpp / CloudMonitor24 Platform Protocol implementation
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


#include "cloudmonitor24_config.h"
#include "cloudmonitor24_hal.h"
#include "cloudmonitor24_prot.h"


// Local defines
#define CM24_FRAME_TYPE_NONE            253
#define CM24_FRAME_TYPE_UNKNOWN         254
#define CM24_SOCKET_EMPTY               255
#define CM24_FRAME_TYPE_IDENTIFYING     1
#define CM24_FRAME_TYPE_KEEP_ALIVE      2
#define CM24_FRAME_TYPE_ACK             3
#define CM24_FRAME_TYPE_NOT_ACK         4
#define CM24_FRAME_TYPE_CMD             5
#define CM24_FRAME_TYPE_CMD_ACK         6
#define CM24_FRAME_TYPE_CMD_NACK        7
#define CM24_FRAME_TYPE_TIME_SYNC       8

#define CM24_FRAME_TYPE_ALARM           100
#define CM24_FRAME_TYPE_VARIABLE        101
#define CM24_FRAME_TYPE_MULTI_VARIABLE  104

#define CM24_MAGIC_START_CH             0x55
#define CM24_MAGIC_END_CH               0xAA

#define CM24_SOCKET_URL                 "data.cloudmonitor24.com"

#define CM24_SOCKET_PORT                3503
#define CM24_TIMEOUT_SEND_KEEP_ALIVE    240  // seconds (4 minutes)
#define CM24_TIMEOUT_WAIT_ACK           10   // seconds
#define CM24_TIMEOUT_WAIT_RECONNECT     60   // seconds
#define CM24_TIME_SYNC_INTERVAL         600  // Sync every 10 minutes

// Number of variables sent on each multi-frame
// A big value speed-up data issue but require more memory
// Maximum value is 70
#define CM24_MAX_VAR_PER_FRAME      10

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
  CM24_SOCKET_GET_LENGTH,
  CM24_SOCKET_GET_DATA,
  CM24_SOCKET_CALCULATE_CHECKSUM,
  CM24_SOCKET_WAIT_END
} SocketReadBufferState_e;

static void cm24_write_data_to_network(cm24_element_buffer_t *buffer, uint8_t type );
static void cm24_write_var_data_to_network(cm24_element_buffer_t *buffer, uint8_t num_records);
static void cm24_write_multi_var_data_to_network(cm24_element_buffer_t *buffer, uint8_t num_records );
static void cm24_write_alarm_data_to_network(cm24_element_buffer_t *buffer);
static uint8_t getChecksum(uint8_t *buffer, uint8_t size, uint8_t starting_value);

static SocketReadBufferState_e socketReadBufferState;
static cm24_frame_identifier _startFrame;
static cm24_frame_empty _emptyFrame;
static cm24_socket_t sock;
static uint8_t receivedFrameType;

static cm24_frame_cmd_response _cmdFrame;

static void * alarm_buffer;
static void * var_buffer;

extern cm24_config_t cm24_cfg;
extern bool cm24_init_done;

static bool cm24_command_received( cm24_command_t *command, uint8_t length )
{
    uint8_t i;

    // Command should contain at least token and identifier
    if ( length < (CM24_DATALOGGER_PLANT_CMDTOKEN_SIZE + 4))
    {
        return FALSE;
    }

    // Compare token
    for(i=0; i<CM24_DATALOGGER_PLANT_CMDTOKEN_SIZE; i++)
    {
        if (command->token[i] != cm24_cfg.cmd_token[i])
        {
            return FALSE;
        }
    }

    // Execute command
    return cm24_hal_command_received( command->cmd, length - (CM24_DATALOGGER_PLANT_CMDTOKEN_SIZE + 4) );
}

//Temporary return received frame type, will return frame_type and value
static void socket_received(void *arg, uint8_t *pdata, unsigned short len)
{
    uint16_t i;
    static uint8_t chk;
    static uint8_t offset, length;
    static uint8_t frameType;
    static uint8_t data[CM24_MAX_COMMAND_SIZE];

    //loop into all buffer
    for(i=0; i<len; i++)
    {
        uint8_t byteRead = pdata[i];

        switch(socketReadBufferState)
        {
            // Receiving first start byte
            case CM24_SOCKET_WAIT_START :
            {
                if(byteRead == CM24_MAGIC_START_CH)
                {
                    // Byte Frame start FOUND!
                    socketReadBufferState = CM24_SOCKET_WAIT_FRAME_TYPE;   
                    chk = 0;          
                }                

                break;
            }

            // Receiving frame type
            case CM24_SOCKET_WAIT_FRAME_TYPE :
            {

                chk ^= byteRead;

                switch(byteRead)
                {
                    case CM24_FRAME_TYPE_ACK:
                    {
                        socketReadBufferState = CM24_SOCKET_CALCULATE_CHECKSUM;
                        frameType = CM24_FRAME_TYPE_ACK;                        
                        break;
                    }

                    case CM24_FRAME_TYPE_NOT_ACK:
                    {
                        socketReadBufferState = CM24_SOCKET_CALCULATE_CHECKSUM;
                        frameType = CM24_FRAME_TYPE_NOT_ACK;
                        break;
                    }

                    case CM24_FRAME_TYPE_CMD:
                    {
                        socketReadBufferState = CM24_SOCKET_GET_LENGTH;
                        frameType = CM24_FRAME_TYPE_CMD;
                        break;
                    }

                    default:
                    {
                        if(byteRead == CM24_MAGIC_START_CH)
                        {
                            //Byte Frame start FOUND!
                            socketReadBufferState = CM24_SOCKET_WAIT_FRAME_TYPE;                 
                        }
                        else
                        {
                            socketReadBufferState = CM24_SOCKET_WAIT_START;    
                        }
                        break;
                    }                
                }
                break;
            }

            // Receiving command length
            case CM24_SOCKET_GET_LENGTH:
            {
                chk ^= byteRead;
                length = byteRead;
                offset = 0;
                if (length > CM24_MAX_COMMAND_SIZE)
                {
                    socketReadBufferState = CM24_SOCKET_WAIT_START;    
                }
                else
                {
                    socketReadBufferState = CM24_SOCKET_GET_DATA;
                }
                break;
            }

            // Receiving command length
            case CM24_SOCKET_GET_DATA:
            {
                chk ^= byteRead;
                data[offset++] = byteRead;

                // The command should contain at least plant token, the identifier (counter) and 1 byte of true command
                if (offset >= length)
                {
                    socketReadBufferState = CM24_SOCKET_CALCULATE_CHECKSUM;       
                }
                break;
            }
            
            case CM24_SOCKET_CALCULATE_CHECKSUM:
            {                
                //Calculate checksum
                if(chk == byteRead)
                {
                    //Good checksum check for frame magic end
                    socketReadBufferState = CM24_SOCKET_WAIT_END;
                }
                else
                {
                    //something wrong in frame received..
                    socketReadBufferState = CM24_SOCKET_WAIT_START;
                    frameType = CM24_FRAME_TYPE_UNKNOWN;
                }
                break;
            }
            
            case CM24_SOCKET_WAIT_END :
            {
                if(byteRead == CM24_MAGIC_END_CH)
                {
                    //Reset socket parser state..
                    socketReadBufferState = CM24_SOCKET_WAIT_START;   

                    // Check if frame received is a command
                    if (frameType != CM24_FRAME_TYPE_CMD)
                    {
                        receivedFrameType = frameType;    
                    }
                    else
                    {
                        cm24_command_t *command = (cm24_command_t *) data;
                        if (cm24_command_received( command, length ) == TRUE)
                        {
                            // Send cmd ACK
                            _cmdFrame.frame_type = CM24_FRAME_TYPE_CMD_ACK;
                        }
                        else
                        {
                            // Send cmd NACK
                            _cmdFrame.frame_type = CM24_FRAME_TYPE_CMD_NACK;
                        }

                        // Send back answer
                        _cmdFrame.cmd_counter = command->cmd_counter;
                        _cmdFrame.chk = getChecksum( (uint8_t *) &(_cmdFrame.frame_type), sizeof(cm24_frame_cmd_response)-3, 0 );
                        cm24_hal_socket_write(&sock, (uint8_t *)&_cmdFrame, sizeof(cm24_frame_cmd_response));
                    }
                }
                else
                {
                    //something wrong in frame received..
                    if(byteRead == CM24_MAGIC_START_CH)
                    {
                        //Byte Frame start FOUND!
                        socketReadBufferState = CM24_SOCKET_WAIT_FRAME_TYPE;                 
                    }
                    else
                    {
                        socketReadBufferState = CM24_SOCKET_WAIT_START;    
                    }
                    frameType = CM24_FRAME_TYPE_UNKNOWN;
                }
                break;
            }
        }
    }
    
}

void cm24_platform_prot_init(Client *channel)
{
    int i;

    alarm_buffer = cm24_hal_create_buffer(CM24_BUFFER_ALARM_SIZE, "A");
    var_buffer = cm24_hal_create_buffer(CM24_BUFFER_VAR_SIZE, "V");

    // Init the start frame
    _startFrame.start = CM24_MAGIC_START_CH;
    _startFrame.frame_type = CM24_FRAME_TYPE_IDENTIFYING;
    _startFrame.end = CM24_MAGIC_END_CH;

    // Init the command frame
    _cmdFrame.start = CM24_MAGIC_START_CH;
    _cmdFrame.end = CM24_MAGIC_END_CH;


    // Fill plant_id
    for(i=0; i<cm24_hal_strlen(cm24_cfg.plant_id); i++)
    {
        if (i>=CM24_DATALOGGER_PLANT_ID_SIZE)
        {
            break;
        }
        _startFrame.identifier[i] = cm24_cfg.plant_id[i];
    }
    for(; i<CM24_DATALOGGER_PLANT_ID_SIZE; i++)
    {
        _startFrame.identifier[i] = '\0';   
    }

    // Fill plant_token
    for(i=0; i<cm24_hal_strlen(cm24_cfg.plant_token); i++)
    {
        if (i>=CM24_DATALOGGER_PLANT_TOKEN_SIZE)
        {
            break;
        }
        _startFrame.passcode[i] = cm24_cfg.plant_token[i];
    }
    for(; i<CM24_DATALOGGER_PLANT_TOKEN_SIZE; i++)
    {
        _startFrame.passcode[i] = '\0';   
    }

    //Init empty frame
    _emptyFrame.start = CM24_MAGIC_START_CH;
    _emptyFrame.end = CM24_MAGIC_END_CH;

    // Init socket obj
    cm24_hal_socket_init( &sock, channel);
    cm24_hal_socket_readcb( &sock, socket_received );

    socketReadBufferState = CM24_SOCKET_WAIT_START;
}

void cm24_platform_prot_main()
{
    static DataLoggerState_e _cm24LoggerState = CM24_WAIT_CONNECT;
    
    static uint32_t startWaitingTimeAck = 0;
    static uint32_t startWaitingTimeReconnect = 0;

    static uint32_t lastTimeSync = 0;
    static uint32_t lastTimeDataSent = 0;
    static uint8_t lastFrameTypeSent;
    static uint8_t lastFrameRecordNum = 0;

    switch(_cm24LoggerState){

        case CM24_WAIT_CONNECT:
        {
            if(cm24_hal_socket_is_connected(&sock))
            {        
                socketReadBufferState = CM24_SOCKET_WAIT_START;

                // Launch identification packet
                _startFrame.timestamp = cm24_hal_time();
                _startFrame.chk = getChecksum( &_startFrame.frame_type, sizeof(cm24_frame_identifier)-3, 0 );
                cm24_hal_socket_write(&sock, (uint8_t *)&_startFrame, sizeof(cm24_frame_identifier));
                                    
                startWaitingTimeAck = cm24_hal_time();
                lastFrameTypeSent = _startFrame.frame_type;
                _cm24LoggerState = CM24_WAIT_ACK;
                receivedFrameType = CM24_FRAME_TYPE_NONE;
                lastTimeDataSent = cm24_hal_time();
                lastTimeSync = cm24_hal_time();
            }
            else if(cm24_hal_socket_is_disconnected(&sock))
            {
                //Try connect
                if (!cm24_hal_socket_connect(&sock, CM24_SOCKET_URL, CM24_SOCKET_PORT))
                {
                    //Do nothing, try again next time WARNING LOOP
                    _cm24LoggerState = CM24_WAIT_RECONNECT;
                    startWaitingTimeReconnect = cm24_hal_time();                    
                }
            }

            break;
        }

        case CM24_SEND_DATA:
        {
            if(!cm24_hal_socket_is_disconnected(&sock))
            {
                static cm24_element_buffer_t buffer[CM24_MAX_VAR_PER_FRAME];
                
                // Alarm has higher priority
                if(cm24_hal_buffer_pop( alarm_buffer, buffer, 0)) //alarm available?
                {                    

                    cm24_write_alarm_data_to_network(buffer);

                    //data sent successfully
                    _cm24LoggerState = CM24_WAIT_ACK;
                    receivedFrameType = CM24_FRAME_TYPE_NONE;
                    startWaitingTimeAck = cm24_hal_time();
                    
                    //update sent time
                    lastTimeDataSent = cm24_hal_time();
                    
                    //save last frame-type sent, reading on "popped" data
                    lastFrameTypeSent = CM24_FRAME_TYPE_ALARM;
                
                }

                // Time sync is more important than variables!
                else if ((cm24_hal_time() - lastTimeSync) >= CM24_TIME_SYNC_INTERVAL)
                {
                    cm24_frame_time_sync _cmdTimeSync;

                    _cmdTimeSync.start = CM24_MAGIC_START_CH;
                    _cmdTimeSync.frame_type = CM24_FRAME_TYPE_TIME_SYNC;
                    _cmdTimeSync.local_timestamp = cm24_hal_time();
                    _cmdTimeSync.chk = getChecksum( &_cmdTimeSync.frame_type, sizeof(cm24_frame_time_sync)-3, 0 );
                    _cmdTimeSync.end = CM24_MAGIC_END_CH;

                    cm24_hal_socket_write(&sock, (uint8_t *)&_cmdTimeSync, sizeof(cm24_frame_time_sync));

                    // Wait for ACK
                    _cm24LoggerState = CM24_WAIT_ACK;
                    receivedFrameType = CM24_FRAME_TYPE_NONE;
                    startWaitingTimeAck = cm24_hal_time();
                    lastFrameTypeSent = CM24_FRAME_TYPE_TIME_SYNC;
                    lastTimeSync = cm24_hal_time();
                }

                // Then we check for variables
                else if(cm24_hal_buffer_pop( var_buffer, buffer, 0)) //variable available?
                {                    
                    int i;

                    // The first record has already been read
                    for(i=1; i<CM24_MAX_VAR_PER_FRAME; i++)
                    {
                        if(!cm24_hal_buffer_pop( var_buffer, &buffer[i], i))
                        {
                            break;
                        }
                    }

                    //data sent successfully
                    _cm24LoggerState = CM24_WAIT_ACK;
                    receivedFrameType = CM24_FRAME_TYPE_NONE;
                    startWaitingTimeAck = cm24_hal_time();
                    
                    //update sent time
                    lastTimeDataSent = cm24_hal_time();
                    
                    //save last frame-type sent, reading on "popped" data
                    lastFrameTypeSent = CM24_FRAME_TYPE_VARIABLE;
                    lastFrameRecordNum = i;
                    cm24_write_var_data_to_network(buffer, i);
                
                }

                // As a last check we send keepalive
                else
                {
                    //No data to send, handle timeout for keep alive
                    if( (cm24_hal_time() - lastTimeDataSent) >= CM24_TIMEOUT_SEND_KEEP_ALIVE)
                    {
                        //send keep alive
                        _emptyFrame.frame_type = CM24_FRAME_TYPE_KEEP_ALIVE;
                        _emptyFrame.chk = getChecksum( &_emptyFrame.frame_type, sizeof(cm24_frame_empty)-3, 0 );
                        
                        cm24_hal_socket_write(&sock, (uint8_t *)&_emptyFrame, sizeof(cm24_frame_empty));
                    
                        _cm24LoggerState = CM24_WAIT_ACK;
                        receivedFrameType = CM24_FRAME_TYPE_NONE;
                        startWaitingTimeAck = cm24_hal_time();
                        lastFrameTypeSent = CM24_FRAME_TYPE_KEEP_ALIVE;
                        lastTimeDataSent = cm24_hal_time();                
                    }
                    else
                    {
                        //loop here until timeout fire..
                    }
                }
            }
            else
            {
                _cm24LoggerState = CM24_WAIT_RECONNECT;
                startWaitingTimeReconnect = cm24_hal_time();
                socketReadBufferState = CM24_SOCKET_WAIT_START;
            }
            break;
        }

        case CM24_WAIT_ACK:
        {
            if(!cm24_hal_socket_is_disconnected(&sock))
            {
                if (!cm24_hal_socket_is_write_in_progress(&sock))
                {
                    long timeFromStartWaiting =  cm24_hal_time() - startWaitingTimeAck;
                    //loop until timeout fire
                    if( ( timeFromStartWaiting ) >= CM24_TIMEOUT_WAIT_ACK)
                    {
                        cm24_hal_socket_disconnect( &sock );
                        _cm24LoggerState = CM24_WAIT_RECONNECT;
                        startWaitingTimeReconnect = cm24_hal_time();
                        socketReadBufferState = CM24_SOCKET_WAIT_START;
                    }
                    else
                    {
                        //check for frame type received
                        switch(receivedFrameType)
                        {
                            case CM24_FRAME_TYPE_ACK:
                            {
                                //swith possible frame sent
                                switch(lastFrameTypeSent)
                                {
                                    case CM24_FRAME_TYPE_VARIABLE:
                                    case CM24_FRAME_TYPE_MULTI_VARIABLE:
                                        cm24_hal_buffer_increment( var_buffer, lastFrameRecordNum);
                                        break;
                                    case CM24_FRAME_TYPE_ALARM:
                                        cm24_hal_buffer_increment( alarm_buffer, 1);
                                        break;
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
            }
            else
            {
                _cm24LoggerState = CM24_WAIT_RECONNECT;
                startWaitingTimeReconnect = cm24_hal_time();
                socketReadBufferState = CM24_SOCKET_WAIT_START;
            }
            
            break;
        }

        case CM24_WAIT_RECONNECT:
        {
            if( (cm24_hal_time() - startWaitingTimeReconnect) > CM24_TIMEOUT_WAIT_RECONNECT )
            {
                _cm24LoggerState = CM24_WAIT_CONNECT;
            }
            break;
        }
    }
}


bool cm24_log_variable(uint32_t var_id, float value, uint16_t sensor_id)
{
    static cm24_element_buffer_t buffer;
    
    if (!cm24_init_done) return FALSE;

    buffer.sensor_id = sensor_id;
    buffer.var_id = var_id;

    // Value
    uint32_t *float_pt = (uint32_t *) &value;
    buffer.value = *float_pt;
    
    // Timestamp
    buffer.timestamp = cm24_hal_time();

    return cm24_hal_buffer_push(var_buffer, &buffer);
}
    
bool cm24_log_alarm(uint32_t alarm_id, float alarm_info, uint16_t sensor_id)
{
    static cm24_element_buffer_t buffer;

    if (!cm24_init_done) return FALSE;
    
    buffer.sensor_id = sensor_id;
    buffer.var_id = alarm_id;

    // Value
    uint32_t *float_pt = (uint32_t *) &alarm_info;
    buffer.value = *float_pt;
    
    // Timestamp
    buffer.timestamp = cm24_hal_time();

    return cm24_hal_buffer_push(alarm_buffer, &buffer);
}

static void cm24_write_alarm_data_to_network(cm24_element_buffer_t *buffer)
{
    cm24_write_data_to_network(buffer, CM24_FRAME_TYPE_ALARM);
}

static void cm24_write_var_data_to_network(cm24_element_buffer_t *buffer, uint8_t num_records)
{
    if (num_records == 1)
    {
        cm24_write_data_to_network(buffer, CM24_FRAME_TYPE_VARIABLE);    
    }
    else
    {
        cm24_write_multi_var_data_to_network( buffer, num_records );
    }
}


static void cm24_write_multi_var_data_to_network(cm24_element_buffer_t *buffer, uint8_t num_records )
{
    cm24_frame_multivar_header _header;
    cm24_frame_multivar_footer _footer;
    uint32_t data_size;

    // Header
    _header.start = CM24_MAGIC_START_CH;
    _header.frame_type = CM24_FRAME_TYPE_MULTI_VARIABLE;
    _header.num_records = num_records;
    
    data_size = num_records * sizeof(cm24_element_buffer_t);

    _footer.chk = getChecksum( (uint8_t *) &_header.frame_type, sizeof(cm24_frame_multivar_header)-1, 0 );
    _footer.chk = getChecksum( (uint8_t *) buffer, data_size, _footer.chk );
    _footer.end = CM24_MAGIC_END_CH;

    // Splitting into 3 different requests avoid a single buffer 
    cm24_hal_socket_write(&sock, (uint8_t *)&_header, sizeof(cm24_frame_multivar_header));
    cm24_hal_socket_write(&sock, (uint8_t *)buffer, data_size);
    cm24_hal_socket_write(&sock, (uint8_t *)&_footer, sizeof(cm24_frame_multivar_footer));
}


static void cm24_write_data_to_network(cm24_element_buffer_t *buffer, uint8_t type )
{
    static cm24_frame_data _dataFrame;

    // Header
    _dataFrame.start = CM24_MAGIC_START_CH;
    _dataFrame.end = CM24_MAGIC_END_CH;
    _dataFrame.frame_type = type;
    
    // Data
    _dataFrame.data.sensor_id = buffer->sensor_id;
    _dataFrame.data.var_id = buffer->var_id;
    _dataFrame.data.value = buffer->value;
    _dataFrame.data.timestamp = buffer->timestamp;

    _dataFrame.chk = getChecksum( &_dataFrame.frame_type, sizeof(cm24_frame_data)-3, 0 );

    cm24_hal_socket_write(&sock, (uint8_t *)&_dataFrame, sizeof(cm24_frame_data));
}

static uint8_t getChecksum(uint8_t *buffer, uint8_t size, uint8_t starting_value)
{
    uint8_t i;
    uint8_t ret = starting_value;
    for(i=0; i<size; i++)
    {
        ret ^= buffer[i];
    }
    return ret;
}

