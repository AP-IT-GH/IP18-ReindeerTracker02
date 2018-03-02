/**
 * This is template for main module created by New Kinetis SDK 2.x Project Wizard. Enjoy!
 **/

#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"

#include "fsl_gpio.h"
#include "fsl_uart.h"
#include "fsl_port.h"
#include "fsl_common.h"
#include "fsl_i2c.h"
#include "fsl_smc.h"
#include "fsl_lptmr.h"
#include "fsl_dspi.h"

#include <stdio.h>
#include <stdlib.h>

#include "acc_func.h"
#include "i2c_func.h"
#include "adc_func.h"
#include "fsl_rtc.h"

lptmr_config_t lptmr_config;
smc_power_mode_vlls_config_t smc_power_mode_vlls_config;

volatile uint8_t wake = 0;

void delay(uint32_t del) {
	for (; del > 1; del--) {
		__asm("nop");
	}
}
void initTimer() {

	LPTMR_GetDefaultConfig(&lptmr_config);
	lptmr_config.bypassPrescaler = true;
	lptmr_config.value = kLPTMR_Prescale_Glitch_0;
	lptmr_config.prescalerClockSource = kLPTMR_PrescalerClock_1;
	//EnableIRQ(LPTMR0_IRQn);
	LPTMR_Init(LPTMR0, &lptmr_config);
	LPTMR_SetTimerPeriod(LPTMR0, 5000);  // 3000 for 20hz data rat
}

/*
void LPTMR0_IRQHandler() {

	LPTMR0 -> CSR |= LPTMR_CSR_TCF_MASK;		// Clear the interrupt flag
	while ( LPTMR0 -> CSR & LPTMR_CSR_TCF_MASK  ) {

	}

	PMC ->REGSC |= 0x08;

	GPIO_PortToggle(GPIOB, 1<<21u); //light blue LED
}
*/

int main(void) {


	//uint8_t temp = LLWU -> F3;
	PMC ->REGSC |= 0x08;
	EnableIRQ(LLWU_IRQn);

	/* Init board hardware. */
	BOARD_InitPins();
	BOARD_BootClockRUN();
	BOARD_InitDebugConsole();

  initI2C();
  initAdc();
  configure_acc();
  acc_init();
  initTimer();




  SMC_SetPowerModeProtection(SMC, kSMC_AllowPowerModeVlls);
  smc_power_mode_vlls_config.subMode = kSMC_StopSub1; 			/*!< Stop submode 1, for VLLS1/LLS1. */

  LLWU -> ME |= 0x01; 											// Enable wakeup module for LPTMR
  LLWU -> PE3 |= 0x20;
  LLWU -> FILT1 |= 0x4A;
  //LLWU -> RST |= 0x01;

  //EnableIRQ(PORTC_IRQn);

  LPTMR_EnableInterrupts(LPTMR0, LPTMR_CSR_TIE_MASK);		//Sets Timer Interrupt Enable bit to 1
  LPTMR_StartTimer(LPTMR0);


  static const gpio_pin_config_t LED_configOutput = { kGPIO_DigitalOutput, /* use as output pin */
  	1, /* initial value */
  	};


  printf("Wakeup by LLWU: %x \r\n", wake);  // 1 = LPTMR interrupt, 2 = Accel interrupt, 0 = No interrupts

  GPIO_PinInit(GPIOB, 21u, &LED_configOutput);
  GPIO_PinInit(GPIOB, 22u, &LED_configOutput);

  GPIO_ClearPinsOutput(GPIOB, 1<<22u); //light red to indicate interrupt LED
  delay(100000);
  GPIO_SetPinsOutput(GPIOB, 1<<22u); //light red to indicate interrupt LED

  while (true) {

	  LPTMR0 -> CNR = 0;				// Prints the timer value, write anything on counter before reading it
	  printf("Timer value: %ld \r\n", LPTMR0 -> CNR);

	  SMC_PreEnterStopModes();

	  SMC_SetPowerModeVlls(SMC, &smc_power_mode_vlls_config);

	  SMC_PostExitStopModes();

  }
}
/*
void PORTC_IRQHandler() {

	PORTC -> PCR[6] |= 0x01000000;

	while ( PORTC -> PCR[6] & 0x01000000 ) {

	}

	LPTMR_Deinit(LPTMR0);			// Deinitiate timer to reset timer counte
	LPTMR_Init(LPTMR0, &lptmr_config);
	LPTMR_SetTimerPeriod(LPTMR0, 5000);  // 3000 for 20hz data rat
	LPTMR_EnableInterrupts(LPTMR0, LPTMR_CSR_TIE_MASK);	//Sets Timer Interrupt Enable bit to 1
	LPTMR_StartTimer(LPTMR0);

}
*/

void LLWU_IRQHandler() {


	if ( LLWU -> F3 & 0x01 ) {
		wake = 1;
		CLOCK_EnableClock(kCLOCK_Lptmr0);
		LPTMR0 -> CSR |= LPTMR_CSR_TCF_MASK;
	}
	else if ( LLWU -> F2 & 0x04) {
		wake = 2;
	}

	LLWU -> F2 = 0x04;
}

