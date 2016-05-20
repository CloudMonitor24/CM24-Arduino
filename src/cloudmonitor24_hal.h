/* 
 * cloudmonitor24_hal.h / Hardware abstraction level of the CloudMonitor24 library
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

#ifndef _CLOUDMONITOR24_HAL_H
#define _CLOUDMONITOR24_HAL_H

#include <Arduino.h>
#include <Client.h>
#include "cloudmonitor24_ring_buffer.h"

// Socket status
typedef enum {
  CM24_SOCKET_DISCONNECTED = 0,
  CM24_SOCKET_CONNECTING,
  CM24_SOCKET_CONNECTED,
  CM24_SOCKET_LISTENING,
} cm24_socket_status_e;

// Socket type
typedef enum 
{
  CM24_SOCKET_TCP,
  CM24_SOCKET_UDP
} cm24_socket_type_e;

#pragma pack(1)

typedef struct 
{
  uint16_t sensor_id;
  uint32_t var_id;
  uint32_t value;
  uint32_t timestamp;
} cm24_element_buffer_t;

#pragma pack()

typedef void (* cm24_socket_recv_callback)(void *arg, uint8_t *pdata, unsigned short len);

// Socket struct
typedef struct cm24_socket_t {
    cm24_socket_status_e status;
    Client *channel;
    cm24_socket_recv_callback read;
} cm24_socket_t;

#ifndef TRUE
#define TRUE              true
#endif

#ifndef FALSE
#define FALSE             false
#endif

// Buffering functions
void * cm24_hal_create_buffer( uint32_t size, char *name );
bool cm24_hal_buffer_push(void *queue, cm24_element_buffer_t *);
bool cm24_hal_buffer_pop(void *queue, cm24_element_buffer_t *, uint8_t offset);
bool cm24_hal_buffer_increment(void *queue, uint8_t num_elements );

// Generic init function for the whole hal
void cm24_hal_init();

char * cm24_strncpy ( char * destination, const char * source, size_t num );

uint32_t cm24_hal_time();
size_t cm24_hal_strlen(char *);

void cm24_arduino_loop();

// Socket functions
void cm24_hal_socket_init( cm24_socket_t *sock, Client *channel );
bool cm24_hal_socket_connect( cm24_socket_t *sock, char *host, uint16_t port );
bool cm24_hal_socket_is_disconnected( cm24_socket_t * sock );
bool cm24_hal_socket_is_connected( cm24_socket_t * sock );
bool cm24_hal_socket_is_write_in_progress( cm24_socket_t * sock );
void cm24_hal_socket_write( cm24_socket_t * sock, uint8_t *buff, uint16_t size );
void cm24_hal_socket_disconnect( cm24_socket_t *sock );
void cm24_hal_socket_readcb( cm24_socket_t * sock, cm24_socket_recv_callback recv_cb);

bool cm24_hal_command_received( uint8_t *cmd, uint8_t length );

#endif