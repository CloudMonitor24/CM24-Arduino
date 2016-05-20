/* 
 * cloudmonitor24_lib.cpp / CloudMonitor24 interface library
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

#include "cloudmonitor24_lib.h"
#include "cloudmonitor24_prot.h"


cm24_config_t cm24_cfg;
bool cm24_init_done = FALSE;

bool cm24_init( char *plant_id, char *plant_token, char *cmd_token, Client *channel)
{
	// Already initialized! Only one call allowed
	if (cm24_init_done == TRUE) return FALSE;

	if (plant_id != NULL && plant_token != NULL && cmd_token != NULL)
	{
		cm24_strncpy( cm24_cfg.plant_id, plant_id, CM24_DATALOGGER_PLANT_ID_SIZE );
		cm24_strncpy( cm24_cfg.plant_token, plant_token, CM24_DATALOGGER_PLANT_TOKEN_SIZE );
		cm24_strncpy( cm24_cfg.cmd_token, cmd_token, CM24_DATALOGGER_PLANT_CMDTOKEN_SIZE );

		cm24_cfg.plant_id[CM24_DATALOGGER_PLANT_ID_SIZE] = '\0';
		cm24_cfg.plant_token[CM24_DATALOGGER_PLANT_TOKEN_SIZE] = '\0';
		cm24_cfg.cmd_token[CM24_DATALOGGER_PLANT_CMDTOKEN_SIZE] = '\0';

		// Init HAL
		cm24_hal_init();

		// Initialize fifo and platform protocol
		cm24_platform_prot_init(channel);
		cm24_init_done = TRUE;

		return TRUE;
	}

	return FALSE;
}

/****************************************************
 * Task functions
 ****************************************************/

// Called onced at task start
void cm24_task()
{
	if (cm24_init_done == TRUE)
	{
		cm24_platform_prot_main();
	}
}


