#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
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
#include "drivers/buttons.h"
#include "utils/uartstdio.h"

//*****************************************************************************
// The error routine that is called if the driver library encounters an error.
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{

}
#endif

#define LEFT_BUTTON             GPIO_PIN_4
#define RIGHT_BUTTON            GPIO_PIN_0
#define ALL_BUTTONS             (LEFT_BUTTON | RIGHT_BUTTON)
#define TWO_POW_ONE_OVER_TWELVE 1.059463094359f

#define TIMER_PERIOD 512
#define PWM_PERIOD 256
#define HALF_AMP 0x7FFF
#define HALF_AMP_FLOAT 32767.0f
#define FULL_AMP 0xFFFF
#define FULL_AMP_FLOAT 65535.0f

// 78,125 hz freq
// 80 mhz / 2048 = 39,062.50 hz
// 80 mhz / 1024 = 78,125 hz
// 80 mhz / 512 = 156,250 hz
#define PWM_FREQ 156250
float g_period = 0;
float g_freq = 440;
float g_triHalfPeriod = 0;
float g_triPeriod = 0;
float g_accum = 0;
float g_amp = 1.0;
float g_wavPeriod;
float g_xNrm = 0.0;
int32_t g_output = 0;
uint8_t g_ui8ButtonStates = 0;
uint32_t g_counter = 0;
void setFreq(float f)
{
	if (f < 0)
	{
		return;
	}

	// disable interrupts?
	g_freq = f;
	g_triPeriod = 1.0f / g_freq;
	g_triHalfPeriod = g_triPeriod / 2.0f;
	g_wavPeriod = g_triPeriod;
}

void
init()
{
	g_period = 1.0f / PWM_FREQ;
	g_accum = 0;
	g_amp = 1.0f;
	setFreq(0.01f);

	//remove
	//g_output = 254;
}

void
Timer1AIntHandler(void)
{
	ROM_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

	g_accum += g_period;
	while (g_accum >= g_wavPeriod)
	{
		g_accum -= g_wavPeriod;
		//g_accum = 0;
	}
	g_xNrm = g_accum / g_wavPeriod;

	bool tri = true;
	if (tri)
	{
		// fabs(0.5f - xNrm);
		g_xNrm = 0.5f - g_xNrm;
		if (g_xNrm < 0)
		{
			g_xNrm *= -1.0f;
		}
		float y = -g_amp + (g_xNrm * 2.0f * g_amp);
		//float y = -g_amp + (fabs(0.5f - xNrm) * 2 * g_amp);
		g_output = (int32_t) (HALF_AMP_FLOAT + (HALF_AMP_FLOAT * y));
	}
	else
	{
		float y = g_xNrm >= 0.5 ? 0 : 1;
		g_output = HALF_AMP + (HALF_AMP * y);
	}


    //ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, ((g_output & 0x0000001F) << 5));
    //ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (g_output & 0x000003E0));
	uint32_t hiByte = (g_output & 0x0000FF00) >> 8;
	uint32_t loByte = (g_output & 0x000000FF);
	ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, loByte);
	ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, hiByte);

	++g_counter;
}

void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);
}

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

    // 80 mhz
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // for analog input
    //ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    //ROM_GPIOPinConfigure(GPIO_PE2_U1DCD);
    //ROM_GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_2);

    // pwm config
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinConfigure(GPIO_PB6_M0PWM0);
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_6);
	GPIOPinConfigure(GPIO_PB7_M0PWM1);
	GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_7);

	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PWM_PERIOD);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);
    HWREG(PWM0_BASE + 0x60) |= 0x2;
    HWREG(PWM0_BASE + 0x64) |= 0x2;
    PWMSyncTimeBase(PWM0_BASE, PWM_OUT_0 | PWM_OUT_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMOutputState(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT), true);

    //
    // for the wav generator timer
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    ROM_IntMasterEnable();
    ROM_TimerConfigure(TIMER1_BASE, TIMER_CFG_A_PERIODIC);
    ROM_TimerLoadSet(TIMER1_BASE, TIMER_A, TIMER_PERIOD);
    ROM_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    ROM_IntEnable(INT_TIMER1A);
    ROM_TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(TIMER1_BASE, TIMER_A);

    ButtonsInit();
    ConfigureUART();
    UARTprintf("Welcome to Rythmatico!\n");

    //
    // Loop forever while the timers run.
    //
    //float freq = 200;
    uint8_t buttonDelta = 0;
    uint8_t buttonRawState = 0;

	int32_t notePos = 0;
    bool freqChange = false;
    char buf[256];
    while(1)
    {
    	/*
    	SysCtlDelay(3000000); // 10hz poll
    	buttonDelta = ButtonsPoll(&buttonDelta, &buttonRawState);
    	if (buttonDelta & LEFT_BUTTON)
    	{
    		notePos -= 1;
    		freqChange = true;
    	}
    	else if (buttonDelta & RIGHT_BUTTON)
		{
    		notePos += 1;
    		freqChange = true;
		}
    	if (freqChange)
    	{
    		//fn = f0 * (a)n
    		//f0 = the frequency of one fixed note which must be defined. A common choice is setting the A above middle C (A4) at f0 = 440 Hz.
    		//n = the number of half steps away from the fixed note you are. If you are at a higher note, n is positive. If you are on a lower note, n is negative.
    		//fn = the frequency of the note n half steps away.
    		//a = (2)1/12 = the twelth root of 2 = the number which when multiplied by itself 12 times equals 2 = 1.059463094359...
    		float newFreq = 440.0f * powf(TWO_POW_ONE_OVER_TWELVE, notePos);
    		setFreq(newFreq);
    		freqChange = false;

    		//sprintf(buf, "New freq: %f", newFreq);
    		//UARTprintf("New freq: %d\n", (int) newFreq);
    	}*/

    	SysCtlDelay(30000000);
    	int32_t c = g_counter;
    	//UARTprintf("Counter: %d\n", c);

    	/*if (freq >= 5000)
    	{
    		freq = 50;
    	}
    	freq += 1;
    	setFreq(freq);*/
    	//SysCtlDelay(30000000);
    }
}
