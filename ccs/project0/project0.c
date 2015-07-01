//*****************************************************************************
//
// project0.c - Example to demonstrate minimal TivaWare setup
//
// Copyright (c) 2012-2015 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 2.1.1.71 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"



//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{

}
#endif


void
Timer0AIntHandler(void)
{
	ROM_TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
}

void
Timer0BIntHandler(void)
{
	ROM_TimerIntClear(TIMER0_BASE, TIMER_CAPB_EVENT);
}

bool g_up = false;
int32_t g_count = 0;
bool g_ascend = true;
//int32_t wavFreq = 1000;
//int32_t pwmFreq = 1024;

int32_t g_sampleDelay = 1;
int32_t g_nextSample = 0;

#define UPPER_LIMIT 0x03FF
#define LOWER_LIMIT 0x0000
#define JUMP 10
int32_t g_output = LOWER_LIMIT;

#define HANDLER_FREQ 78125
float g_freq = 1000; // pop every 3-4 secs == 145;
float g_cycle = 0;

#define TIMER_PERIOD 512
#define PWM_PERIOD 256
#define HALF_AMP 0x7FFF
#define FULL_AMP 0xFFFF

// 78,125 hz freq
// 80 mhz / 2048 = 39,062.50 hz
// 80 mhz / 1024 = 78,125 hz
// 80 mhz / 512 = 156,250 hz
#define PWM_FREQ 156250
float g_period = 0;
float g_triHalfPeriod = 0;
float g_triPeriod = 0;
float g_triInc = 0;
float g_accum = 0;
float g_amp = 1.0;
float g_halfAmp = 0;
float g_wavPeriod;

void
init()
{
	g_period = 1.0f / PWM_FREQ;
	g_triPeriod = 1.0f / g_freq;
	g_triHalfPeriod = g_triPeriod / 2.0f;
	g_accum = 0;
	g_amp = 1.0;
	g_wavPeriod = g_triPeriod;
}
int g_accumInt = 0;

void
Timer1AIntHandler(void)
{
	ROM_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

	/*
	if (g_nextSample <= 0)
	{
		g_nextSample = g_sampleDelay;

		if (g_ascend)
		{
			g_output += JUMP;
			if (g_output >= UPPER_LIMIT)
			{
				g_output = UPPER_LIMIT;
				g_ascend = false;
			}
		}
		else
		{
			g_output -= JUMP;
			if (g_output <= LOWER_LIMIT)
			{
				g_output = LOWER_LIMIT;
				g_ascend = true;
			}
		}
	}
	--g_nextSample;

	//HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = (g_output & 0x000003E0) - 1; // upper 5 bits (992)
	//HWREG(TIMER0_BASE + TIMER_O_TBMATCHR) = ((g_output & 0x0000001F) << 5) - 1; // lower 5 bits (31)
	 */

/*
	++g_accumInt;
	if (g_accumInt >= 1024)
	{
		g_accumInt = 0;
	}

	if (g_accumInt >= 512)
	{
		g_output = 0;
	}
	else
	{
		g_output = 0xFFFF;
	}
*/


	g_accum += g_period;
	if (g_accum >= g_wavPeriod)
	{
		//g_accum -= g_wavPeriod;
		g_accum = 0;
	}
	float xNrm = g_accum / g_wavPeriod;

	bool tri = true;
	if (tri)
	{
		float y = -g_amp + (fabs(0.5 - xNrm) * 2 * g_amp);
		g_output = HALF_AMP + (HALF_AMP * y);
	}
	else
	{
		float y = xNrm >= 0.5 ? 0 : 1;
		g_output = HALF_AMP + (HALF_AMP * y);
	}

    //ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, ((g_output & 0x0000001F) << 5));
    //ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (g_output & 0x000003E0));
	ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, ((g_output & 0x000000FF)));
	ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (g_output & 0x0000FF00) >> 8);
}

void
updatePWMValue(int32_t value)
{
	HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = (value & 0x000003E0) - 1; // upper 5 bits
	HWREG(TIMER0_BASE + TIMER_O_TBMATCHR) = ((value & 0x0000001F) << 5) - 1; // lower 5 bits
}

//*****************************************************************************
//
// Main 'C' Language entry point.  Toggle an LED using TivaWare.
// See http://www.ti.com/tm4c123g-launchpad/project0 for more information and
// tutorial videos.
//
//*****************************************************************************
int
main(void)
{
	init();

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // for analog input
    //ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    //ROM_GPIOPinConfigure(GPIO_PE2_U1DCD);
    //ROM_GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_2);


    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinConfigure(GPIO_PB6_M0PWM0);
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_6);

	GPIOPinConfigure(GPIO_PB7_M0PWM1);
	GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_7);

    // pwm config
	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PWM_PERIOD);
    ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 0);
    ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMOutputState(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT), true);


    // to sync across generators
    //PWMOutputUpdateMode(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT), PWM_OUTPUT_MODE_SYNC_LOCAL);
    //PWMSyncUpdate(PWM0_BASE, PWM_GEN_0_BIT);

    //
    // for the pwm timer output
    //
    /*
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_GPIOPinConfigure(GPIO_PB6_T0CCP0);
    ROM_GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_6);
    ROM_GPIOPinConfigure(GPIO_PB7_T0CCP1);
    ROM_GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_7);
*/
    //
    // for the wav generator timer
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();
/*
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);
    HWREG(TIMER0_BASE + TIMER_O_CFG)   = 0x04; // 16bit mode
    HWREG(TIMER0_BASE + TIMER_O_TAMR)  =
    		0x0A 		// periodic
			+ 0x0200 	// PWM
			+ 0x0800 	// TAPLO for sync
    		+ 0x0400;	// TAMRSU for sync
    HWREG(TIMER0_BASE + TIMER_O_TBMR)  =
    		0x0A 		// periodic
			+ 0x0200	// PWM
			+ 0x0800	// TBPLO for sync
			+ 0x0400;	// TBMRSU for sync
    HWREG(TIMER0_BASE + TIMER_O_CTL)  = 0x4040; //invert a & b - TAPWML TBPWML
    HWREG(TIMER0_BASE + TIMER_O_TAPR)  = 0x00; // prescaler
    HWREG(TIMER0_BASE + TIMER_O_TBPR)  = 0x00; // prescaler
    HWREG(TIMER0_BASE + TIMER_O_TAILR) = 0x000003FF; // 10 bit counter (1024 cycles) -- Interval Load Register e.g. counter for timer a
    HWREG(TIMER0_BASE + TIMER_O_TBILR) = 0x000003FF;
    HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = 0x000003FF; // match value
    HWREG(TIMER0_BASE + TIMER_O_TBMATCHR) = 0x000003FF; // match value
    HWREG(TIMER0_BASE + TIMER_O_SYNC) = 0x03; // sync timer a & b

    //ROM_TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
    //ROM_TimerIntClear(TIMER0_BASE, TIMER_CAPB_EVENT);
    //ROM_TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
    //ROM_TimerIntEnable(TIMER0_BASE, TIMER_CAPB_EVENT);
    //ROM_TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
    //ROM_TimerControlEvent(TIMER0_BASE, TIMER_B, TIMER_EVENT_POS_EDGE);
    //ROM_IntEnable(INT_TIMER0A);
    //ROM_IntEnable(INT_TIMER0B);
    ROM_TimerEnable(TIMER0_BASE, TIMER_A | TIMER_B);

    SysCtlDelay(50);
    */

    //
    // timer 1
    ROM_TimerConfigure(TIMER1_BASE, TIMER_CFG_A_PERIODIC);
    ROM_TimerLoadSet(TIMER1_BASE, TIMER_A, TIMER_PERIOD);
    ROM_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    ROM_IntEnable(INT_TIMER1A);
    ROM_TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(TIMER1_BASE, TIMER_A);

    //
    // Loop forever while the timers run.
    //
    //bool ascend = true;
    //int32_t wavFreq = 1000;
    //int32_t pwmFreq = 1024;
    //int32_t delay = (80000000 / (wavFreq * 2 * pwmFreq)) / 3;
    //int32_t delay = (int32_t) ((80000000.0 / 3.0) / 8192.0);
    //int32_t output = 0;

    while(1)
    {
    	/*
    	SysCtlDelay(30000000);
    	HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = 0x000003FF; // = 0 inverted

    	SysCtlDelay(10000000);
    	HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = 0x00000000; // = 1 inverted

    	SysCtlDelay(10000000);
    	HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = 0x000003FE; // = Vbus-1 inverted

    	SysCtlDelay(10000000);
    	HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = 0x00000400; // = 0 inverted
    	continue;
    	*/
    	/*
    	SysCtlDelay(delay);
    	output = 0;
    	HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = (output & 0x000003E0) - 1; // upper 5 bits (992)
    	HWREG(TIMER0_BASE + TIMER_O_TBMATCHR) = ((output & 0x0000001F) << 5) - 1; // lower 5 bits (31)

    	SysCtlDelay(delay);
    	output = 0x03FF;
    	HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = (output & 0x000003E0) - 1; // upper 5 bits (992)
    	HWREG(TIMER0_BASE + TIMER_O_TBMATCHR) = ((output & 0x0000001F) << 5) - 1; // lower 5 bits (31)
    	continue;
    	*/
/*
    	if (ascend)
    	{
    		++output;
    		if (output == 0x03FF)
    		{
    			ascend = false;
    		}
    	}
    	else
    	{
    		--output;
    		if (output == 0)
    		{
    			ascend = true;
    		}
    	}
    	//HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = (output - 1);
    	HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = (output & 0x000003E0) - 1; // upper 5 bits (992)
    	HWREG(TIMER0_BASE + TIMER_O_TBMATCHR) = ((output & 0x0000001F) << 5) - 1; // lower 5 bits (31)

    	SysCtlDelay(delay);*/
    }
}
