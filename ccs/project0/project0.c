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
// The error routine that is called if the driver library encounters an error.
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{

}
#endif

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
float g_freq = 5000;
float g_triHalfPeriod = 0;
float g_triPeriod = 0;
float g_accum = 0;
float g_amp = 1.0;
float g_wavPeriod;
int32_t g_output = 0;

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
    ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 0);
    ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);
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

    //
    // Loop forever while the timers run.
    //
    while(1)
    {
    	/*
    	SysCtlDelay(30000000);
    	HWREG(TIMER0_BASE + TIMER_O_TAMATCHR) = 0x000003FF; // = 0 inverted
		*/
    }
}
