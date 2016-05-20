/* 
 * cloudmonitor24_lib.h / CloudMonitor24 interface library
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

#ifndef _CLOUDMONITOR24_LIB_H
#define _CLOUDMONITOR24_LIB_H

#include "cloudmonitor24_hal.h"
#include "cloudmonitor24.h"


#define CM24_DATALOGGER_PLANT_ID_SIZE			16
#define CM24_DATALOGGER_PLANT_TOKEN_SIZE		16
#define CM24_DATALOGGER_PLANT_CMDTOKEN_SIZE		16

// No pointer allowed on this structure
typedef struct {
	char 		plant_id[CM24_DATALOGGER_PLANT_ID_SIZE+1];					// Unique identifier of the plant in CloudMonitor24 platform
	char 		plant_token[CM24_DATALOGGER_PLANT_TOKEN_SIZE+1];			// Key of the plant in CloudMonitor24 platform
	char 		cmd_token[CM24_DATALOGGER_PLANT_CMDTOKEN_SIZE+1];			// Token for commands
} cm24_config_t;

#endif
