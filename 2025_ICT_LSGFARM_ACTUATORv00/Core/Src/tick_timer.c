/*
 * xm_timer.c
 *
 *  Created on: 2022. 4. 5.
 *      Author: xmcap
 */

#include "tick_timer.h"

static uint8_t sec_inited = 0;
static uint8_t msec_inited = 0;

// Private Function
static void init_sec_timer()
{
	uint8_t i;

	if(sec_inited == 0)
	{
		for(i = 0; i < SEC_MAX_TIMER; i++)
		{
			memset(&sec_timer[i], 0x00, sizeof(SEC_TIMER_T));
		}
		sec_inited = 1;
	}
}

// Public Function
void sec_timer_inc_tick()  //주의 : Timer Interrupt 내부에서 만 호출 할 것
{
	uint8_t i;

	init_sec_timer();

	for(i = 0; i < SEC_MAX_TIMER; i++)
	{
		sec_timer[i].nSecTimerTick += (sec_timer[i].bEnable ? 1 : 0);
	}
}

void sec_timer_dec_tick()  //주의 : Timer Interrupt 내부에서 만 호출 할 것
{
	uint8_t i;

	init_sec_timer();

	for(i = 0; i < SEC_MAX_TIMER; i++)
	{
		if(sec_timer[i].bEnable && sec_timer[i].nSecTimerTick > 0)
		{
			sec_timer[i].nSecTimerTick--;
		}
	}
}

uint8_t sec_timer_create()
{
	uint8_t i;
//	uint8_t rtn = 0;
	uint8_t rtn = TIMER_INVALID_ID;			// 25.12.31. Fixed by diskept

	init_sec_timer();

	for(i = 0; i < SEC_MAX_TIMER; i++)
	{
		if(sec_timer[i].bEnable == 0)
		{
			rtn = i;
			break;
		}
	}

	// 25.12.31. Added by diskept
    if(rtn == TIMER_INVALID_ID)
    {
        // No free timer slot
        return TIMER_INVALID_ID;
    }

	sec_timer[rtn].bEnable = 1;
	sec_timer[rtn].nSecTimerTick = 0;

	return rtn;
}

uint32_t sec_timer_get_sec(uint8_t tmr)
{
	return sec_timer[tmr].nSecTimerTick;
}

void sec_timer_reset(uint8_t tmr)
{
	sec_timer[tmr].nSecTimerTick = 0;
}

//====================================================================
static void init_msec_timer()
{
	uint8_t i;

	if(msec_inited == 0)
	{
		for(i = 0; i < SEC_MAX_TIMER; i++)
		{
			memset(&msec_timer[i], 0x00, sizeof(MSEC_TIMER_T));
		}
//		sec_inited = 1;
		msec_inited = 1;		// 25.12.31. Fixed by diskept
	}
}

// Public Function
void msec_timer_inc_tick()  //주의 : Timer Interrupt 내부에서 만 호출 할 것
{
	uint8_t i;

	init_msec_timer();

	for(i = 0; i < SEC_MAX_TIMER; i++)
	{
		msec_timer[i].nMsecTimerTick += (msec_timer[i].bEnable ? 1 : 0);
	}
}

void msec_timer_dec_tick()  //주의 : Timer Interrupt 내부에서 만 호출 할 것
{
	uint8_t i;

	init_msec_timer();

	for(i = 0; i < SEC_MAX_TIMER; i++)
	{
		if(msec_timer[i].bEnable && msec_timer[i].nMsecTimerTick > 0)
		{
			msec_timer[i].nMsecTimerTick--;
		}
	}
}

uint8_t msec_timer_create()
{
	uint8_t i;
	//uint8_t rtn = 0;
	uint8_t rtn = TIMER_INVALID_ID;			// 25.12.31. Fixed by diskept

	init_msec_timer();

	for(i = 0; i < SEC_MAX_TIMER; i++)
	{
		if(msec_timer[i].bEnable == 0)
		{
			rtn = i;
			break;
		}
	}

	// 25.12.31. Added by diskept
	if(rtn == TIMER_INVALID_ID)
	{
		// No free timer slot
		return TIMER_INVALID_ID;
	}

	msec_timer[rtn].bEnable = 1;
	msec_timer[rtn].nMsecTimerTick = 0;

	return rtn;
}

uint32_t msec_timer_get_msec(uint8_t tmr)
{
	//return sec_timer[tmr].nSecTimerTick;
	return msec_timer[tmr].nMsecTimerTick;		// 25.12.31. Fixed by diskept
}

void msec_timer_reset(uint8_t tmr)
{
	msec_timer[tmr].nMsecTimerTick = 0;
}
