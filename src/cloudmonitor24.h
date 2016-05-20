/* 
 * cloudmonitor24.h / CloudMonitor24 interface library
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

#ifndef _CLOUDMONITOR24_H
#define _CLOUDMONITOR24_H

#include <cloudmonitor24_hal.h>
#include <Client.h>

bool cm24_init( char *plant_id, char *plant_token, char *cmd_token, Client *channel);
bool cm24_log_variable(uint32_t var_id, float value, uint16_t sensor_id);
bool cm24_log_alarm(uint32_t alarm_id, float alarm_info, uint16_t sensor_id);

// Command received callback
typedef void (* cm24_command_received_callback)(char *command);

void cm24_register_command_callback(cm24_command_received_callback callback);

#endif
