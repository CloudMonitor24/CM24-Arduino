/* 
 * cloudmonitor24_hal.cpp / CloudMonitor24 interface library
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

#include "cloudmonitor24_hal.h"
#include "cloudmonitor24_config.h"
#include "cloudmonitor24.h"
#include "cloudmonitor24_prot.h"

static uint32_t local_timestamp;

static RINGBUFF_T alarm_buffer;
static RINGBUFF_T var_buffer;
static cm24_socket_t *current_sock = NULL;

static cm24_element_buffer_t alarm_buffer_data[CM24_BUFFER_ALARM_SIZE];
static cm24_element_buffer_t var_buffer_data[CM24_BUFFER_VAR_SIZE];

static cm24_command_received_callback command_callback;

void cm24_task();

char * cm24_strncpy ( char * destination, const char * source, size_t num )
{
	return strncpy( destination, source, num );
}


// Generic init function for the whole hal
void cm24_hal_init()
{
	local_timestamp = 0;
	command_callback = NULL;
}

// Buffering functions
void * cm24_hal_create_buffer( uint32_t size, char *name )
{
	// Init alarm filesystem
	if (name[0] == 'A')
	{
		cm24_ring_buffer_init(&alarm_buffer,alarm_buffer_data,sizeof(cm24_element_buffer_t),CM24_BUFFER_ALARM_SIZE);
		return 0;
	}
	// Init variable filesystem
	if (name[0] == 'V')
	{
		cm24_ring_buffer_init(&var_buffer,var_buffer_data,sizeof(cm24_element_buffer_t),CM24_BUFFER_VAR_SIZE);
		return (void *) 1;
	}
	
	return (void *) -1;
}

bool cm24_hal_buffer_push(void *queue, cm24_element_buffer_t *elem)
{
	switch((int) queue)
	{
		case 0:
			return cm24_ring_buffer_push(&alarm_buffer,elem);

		case 1:
			return cm24_ring_buffer_push(&var_buffer,elem);
	}
    return false;
}

bool cm24_hal_buffer_pop(void *queue, cm24_element_buffer_t *elem, uint8_t offset)
{
	// this should not increase pointer
	switch((int) queue)
	{
		case 0:
			return cm24_ring_buffer_pop(&alarm_buffer, elem, offset);
		
		case 1:
			return cm24_ring_buffer_pop(&var_buffer, elem, offset);
	}
    return false;

}

bool cm24_hal_buffer_increment(void *queue, uint8_t num_elements )
{
	switch((int) queue)
	{
		case 0:
			return cm24_ring_buffer_increment_tail( &alarm_buffer, num_elements );
		
		case 1:
			return cm24_ring_buffer_increment_tail( &var_buffer, num_elements );
	}
    return false;
}

// Generic functions
uint32_t cm24_hal_time()
{
	return local_timestamp;
}

size_t cm24_hal_strlen(char *str)
{
    return strlen(str);
}

static void update_local_timestamp()
{
	static uint32_t last_millis_time = 0;
    uint32_t curr_millis_time = millis();
    uint32_t delta_millis_time = curr_millis_time - last_millis_time;    

    if (delta_millis_time < 1000)
    {
        return;
    }

    local_timestamp += delta_millis_time / 1000;
    last_millis_time = curr_millis_time;
    last_millis_time -= delta_millis_time % 1000;
}

void cm24_arduino_loop()
{
	// Read from socket
	if (current_sock != NULL && current_sock->status == CM24_SOCKET_CONNECTED && current_sock->read != NULL)
	{
	    //loop into all buffer
	    while(current_sock->channel->available() > 0)
	    {
	    	uint8_t read;

	    	read = current_sock->channel->read();
	    	current_sock->read(current_sock, &read, 1);
	    }
	}

	cm24_task();
	update_local_timestamp();
}

// Socket functions
void cm24_hal_socket_init( cm24_socket_t *sock, Client *channel )
{
    sock->channel = channel;
    sock->read = NULL;
	sock->status = CM24_SOCKET_DISCONNECTED;
}
bool cm24_hal_socket_connect( cm24_socket_t *sock, char *host, uint16_t port )
{
    if(sock->channel->connected())
    {
    	sock->status = CM24_SOCKET_CONNECTED;
		return true;
    }
    else
    {
		//Try connect
        if (sock->channel->connect(host, port))
        {
        	sock->status = CM24_SOCKET_CONNECTED;
        	current_sock = sock;
			return true;
        }
        else
        {
            sock->channel->stop();
            sock->status = CM24_SOCKET_DISCONNECTED;
        	return false;
        }
	}
}
bool cm24_hal_socket_is_disconnected( cm24_socket_t * sock )
{
	return (sock->status == CM24_SOCKET_DISCONNECTED) ? true : false;
}
bool cm24_hal_socket_is_connected( cm24_socket_t * sock )
{
	return (sock->status == CM24_SOCKET_CONNECTED) ? true : false;
}
bool cm24_hal_socket_is_write_in_progress( cm24_socket_t * sock )
{
	return false;
}
void cm24_hal_socket_write( cm24_socket_t * sock, uint8_t *buff, uint16_t size )
{
    sock->channel->write((byte *)buff, size);
}
void  cm24_hal_socket_disconnect( cm24_socket_t *sock )
{
    sock->channel->stop();
    sock->status = CM24_SOCKET_DISCONNECTED;
}

void cm24_hal_socket_readcb( cm24_socket_t * sock, cm24_socket_recv_callback recv_cb)
{
    sock->read = recv_cb;
}

// TRUE = ACK / FALSE = NACK
bool cm24_hal_command_received( uint8_t *cmd, uint8_t length )
{
    if(command_callback != NULL)
    {
    	if(length <= CM24_MAX_COMMAND_STRING)
    	{
    		cmd[length] = '\0';
    		command_callback((char*)cmd);
    		return true;
    	}
    }
    
    return false;
}

void cm24_register_command_callback(cm24_command_received_callback callback)
{
    command_callback = callback;
}
