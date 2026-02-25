/*
 * xm_timer.h
 *
 *  Created on: 2022. 4. 5.
 *      Author: xmcap
 */

#ifndef INC_SEC_TIMER_H_
#define INC_SEC_TIMER_H_

#include "define.h"
#include "struct.h"

#define		TIMER_INVALID_ID (0xFFu)		// 25.12.31 Added by diskept

void     	sec_timer_inc_tick();
void     	sec_timer_dec_tick();

uint8_t  	sec_timer_create();
uint32_t 	sec_timer_get_sec(uint8_t tmr);
void     	sec_timer_reset(uint8_t tmr);

void     	msec_timer_inc_tick();
void     	msec_timer_dec_tick();
uint8_t  	msec_timer_create();
uint32_t 	msec_timer_get_msec(uint8_t tmr);
void     	msec_timer_reset(uint8_t tmr);

#endif
