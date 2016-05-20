 /* 
 * cloudmonitor24_platform_prot.h / CloudMonitor24 Platform Protocol implementation
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

#ifndef _CLOUDMONITOR24_PROT_H
#define _CLOUDMONITOR24_PROT_H

#include "cloudmonitor24.h"
#include "cloudmonitor24_lib.h"
#include "cloudmonitor24_hal.h"
#include <Client.h>


#define CM24_MAX_COMMAND_SIZE           64
#define CM24_MAX_COMMAND_STRING         (CM24_MAX_COMMAND_SIZE - 4 - CM24_DATALOGGER_PLANT_CMDTOKEN_SIZE)

// Socket read callback
typedef void (* cm24_socket_recv_callback)(void *arg, uint8_t *pdata, unsigned short len);


#pragma pack(1)

typedef struct 
{
  uint8_t start;
  uint8_t frame_type;
  uint8_t identifier[16];
  uint8_t passcode[16];
  uint32_t  timestamp;
  uint8_t chk;
  uint8_t end;
} cm24_frame_identifier;


typedef struct 
{
  uint8_t start;
  uint8_t frame_type;
  uint32_t cmd_counter;
  uint8_t chk;
  uint8_t end;
} cm24_frame_cmd_response;


typedef struct 
{
  uint8_t start;
  uint8_t frame_type;
  uint32_t local_timestamp;
  uint8_t chk;
  uint8_t end;
} cm24_frame_time_sync;

typedef struct 
{
  uint8_t start;
  uint8_t frame_type;
  cm24_element_buffer_t data;
  uint8_t chk;
  uint8_t end;
} cm24_frame_data;

typedef struct 
{
  uint8_t start;
  uint8_t frame_type;
  int8_t num_records;
} cm24_frame_multivar_header;

typedef struct 
{
  uint8_t chk;
  uint8_t end;
} cm24_frame_multivar_footer;

typedef struct
{
  uint8_t start;
  uint8_t frame_type;
  uint8_t chk;
  uint8_t end;
} cm24_frame_empty;

// This is the struct of the received command
typedef struct
{
  char token[CM24_DATALOGGER_PLANT_CMDTOKEN_SIZE];
  uint32_t cmd_counter;
  uint8_t cmd[CM24_MAX_COMMAND_STRING + 1];    
} cm24_command_t;

#pragma pack()




// Setup functions
void cm24_platform_prot_init(Client *channel);
void cm24_platform_prot_main();


#endif