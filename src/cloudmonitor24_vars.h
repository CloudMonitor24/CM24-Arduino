/* 
 * cloudmonitor24_vars.h / CloudMonitor24 interface library
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

#ifndef _CLOUDMONITOR24_VARS_H
#define _CLOUDMONITOR24_VARS_H

// List of available variable ids
// WARNING: don't change these values   
#define VAR_ID_LATITUDE				4
#define VAR_ID_LONGITUDE			5
#define VAR_ID_SPEED				6
#define VAR_ID_GPS_HEADING			7
#define VAR_ID_GPS_NUM_SAT			8
#define VAR_ID_BATTERY_CHARGE		17
#define VAR_ID_TEMPERATURE			20   // °C
#define VAR_ID_SOLAR_RADIATON		21   // W/m2
#define VAR_ID_CURRENT				22
#define VAR_ID_OUTPUT_ENERGY		23
#define VAR_ID_INPUT_ENERGY			24
#define VAR_ID_POWER				25
#define VAR_ID_DATALOGGER_BATT_VOLT	38
#define VAR_ID_DEVICE_DIG_IN1		52
#define VAR_ID_DEVICE_DIG_IN2		53
#define VAR_ID_DEVICE_DIG_IN3		54
#define VAR_ID_DEVICE_DIG_IN4		55
#define VAR_ID_DEVICE_ANALOG_IN1	56
#define VAR_ID_DEVICE_ANALOG_IN2	57
#define VAR_ID_DEVICE_ANALOG_IN3	58
#define VAR_ID_DEVICE_ANALOG_IN4	59
#define VAR_ID_ALTITUDE				63
#define VAR_ID_DEVICE_ANALOG_IN5	64
#define VAR_ID_DEVICE_ANALOG_IN6	65
#define VAR_ID_DEVICE_ANALOG_IN7	66
#define VAR_ID_DEVICE_ANALOG_IN8	67
#define VAR_ID_HUMIDITY				69   // %
#define VAR_ID_PRESSURE				70   // Bar
#define VAR_ID_DEVICE_DIG_OUT1		75
#define VAR_ID_DEVICE_DIG_OUT2		76
#define VAR_ID_DEVICE_DIG_OUT3		77
#define VAR_ID_DEVICE_DIG_OUT4		78
#define VAR_ID_COUNTER				79
#define VAR_ID_PWM_OUT1				80
#define VAR_ID_PWM_OUT2				81
#define VAR_ID_PWM_OUT3				82
#define VAR_ID_PWM_OUT4				83
#define VAR_ID_ACCELERATION_X		84
#define VAR_ID_ACCELERATION_Y		85
#define VAR_ID_ACCELERATION_Z		86
#define VAR_ID_AMBIENT_LIGHT		87



#endif