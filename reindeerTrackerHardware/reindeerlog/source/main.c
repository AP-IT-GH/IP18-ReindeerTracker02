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
#include <stdio.h>
#include "acc_func.h"
#include "fsl_dspi.h"
#include "i2c_func.h"

#include "sdcard_io.h"
#include "ff.h"
#include "stdlib.h"


#include "adc_func.h"

#define RING_BUFFER_SIZE 64
#define RX_DATA_SIZE     64
#define X_AXIS 	0
#define Y_AXIS 	1
#define Z_AXIS 	2
#define USE_SD 0

#define SINGLE_ENTRY_SIZE 50
#define ENTRY_HOWMANY 30

char receiveData[RX_DATA_SIZE];
char rxBuffer[RING_BUFFER_SIZE];

char SimcomRecBuf[RING_BUFFER_SIZE];

volatile uint8_t buf_ptr = 0, simcom_buf_ptr = 0;
volatile uint8_t pc_str_rdy = 0, simcom_str_rdy = 0, carriages = 0;

void delay(uint32_t del) {
	for (; del > 1; del--) {
		__asm("nop");
	}
}



/*
 * InitPcUart
 *
 * Initialize UART0 that is connected through the bootloader chip to USB serial
 * so we can have communication with computer terminal
 *
 * Will use UART interrupt for receiving
 *
 */

void InitPcUart() {
	CLOCK_EnableClock(kCLOCK_PortB);
	CLOCK_EnableClock(kCLOCK_Uart0);

	PORT_SetPinMux(PORTB, 16u, kPORT_MuxAlt3); /* PORTB16 (pin 62) is configured as UART0_RX */
	PORT_SetPinMux(PORTB, 17u, kPORT_MuxAlt3); /* PORTB17 (pin 63) is configured as UART0_TX */

	//UART_Deinit(UART0);

	uart_config_t user_config;
	UART_GetDefaultConfig(&user_config);
	user_config.baudRate_Bps = 57600U;
	user_config.enableTx = true;
	user_config.enableRx = true;
	UART_Init(UART0, &user_config, 20971520U); //initialize with default clock speed 20,971520 Mhz

	//UART_EnableInterrupts(UART0, kUART_RxDataRegFullInterruptEnable);
}

int main(void) {

	BOARD_InitPins();
	BOARD_InitDebugConsole();
	initI2C();
	initAdc();

	static const gpio_pin_config_t LED_configOutput = { kGPIO_DigitalOutput, /* use as output pin */
	1, /* initial value */
	};

	GPIO_PinInit(GPIOB, 22u, &LED_configOutput);
	GPIO_PinInit(GPIOB, 21u, &LED_configOutput);
	//GPIO_ClearPinsOutput(GPIOB, 1 << 21u);

	 /*
	  * allocate space from RAM to store logging data block
	  *
	  * one log line is SINGLE_ENTRY_SIZE bytes long:
	  *
	  * X(frdm);Y(frdm);Z(frdm);X(GY61);Y(GY61);Z(GY61)\r\n
	  *
	  * one value takes 7 bytes: 6 digit value + ; separator
	  * \r\n takes 2 bytes
	  * so one entry line is 43 bytes
	  *
	  * ENTRY_HOWMANY is how many lines we want to store at once
	  *
	  */
	char logresult_buffer[SINGLE_ENTRY_SIZE * ENTRY_HOWMANY];

	//flush space with zeros just to be sure
	memset(logresult_buffer,0,sizeof(logresult_buffer));

	acc_init(); //init accelerometer

	InitPcUart();
	initSPI();

#if USE_SD
	/*
	 * Declare FatFs objects
	 */
	FRESULT res; //FatFs operation result code object
	FIL fil; //working file object
	static FATFS fss; //declare a fatfs object

    res = f_mount(&fss, "0:", 0); //Mount sd card
    SD_error(res,"mount"); //check for operation error

    res = f_mkdir("testi");
    res = f_open(&fil, "testi/testilog.csv", FA_WRITE | FA_READ | FA_CREATE_ALWAYS );
    SD_error(res,"file open"); //check for operation error

    //FSIZE_t size = f_size(&fil);
    //f_expand(&fil, 20000,1);
   // res = f_lseek(&fil, size);
    //SD_error(res, "seek");

    char text[] = "Restart logging\r\n";

    f_printf(&fil, text); //print text to file

    res = f_close(&fil); //close file for now
    SD_error(res, "close");

#endif

	while (1) {

		for(;;) //Do a 200 cycle logging run
		{

			for(uint8_t k = 0; k < ENTRY_HOWMANY; k++) //Read all 6 values from accelerometers and save to logging buffer for ENTRY_HOWMANY times
			{

				/*int16_t adc_acc_x = ADC_read16b(1) - 32900;
				int16_t adc_acc_y = ADC_read16b(2) - 32900;		//Accelerometer GY-61
				int16_t adc_acc_z = ADC_read16b(3) - 32900;*/

				uint16_t temp = (65535 - ADC_read16b(4) - 22750) / 473;
				/* 22c temperature, Vout = 1,6V, Vcc = 3,3V,
				R = 10,69ohm @23.5c
				R = 3893ohm @50c Vout = 0.925V @50c ADC = 18369
				R = 135.2K @- 40c Vout = 3.073V     ADC = 61027
				ADC = 48933 @ 0c
				ADC = 473 per celsius
				 */

				int16_t acc_val_x = read_acc_axis(X_AXIS); //read accelerometer X axis
				int16_t acc_val_y = read_acc_axis(Y_AXIS); //read accelerometer y axis	//FRDM integrated accelerometer
				int16_t acc_val_z = read_acc_axis(Z_AXIS); //read accelerometer z axis

				//printf("X: %d\tY: %d\tZ: %d\t", adc_acc_x, adc_acc_y, adc_acc_z );  //Accelerometer GY-61
				printf("X:%d\tY: %d\tZ: %d Temp: %d\r\n", acc_val_x, acc_val_y, acc_val_z, temp);  //FRDM integrated accelerometer

				uint32_t buffer_pointer = strlen(logresult_buffer); //get pointer to last value in RAM buffer
				sprintf(logresult_buffer+buffer_pointer,"%d;%d;%d\r\n",acc_val_x, acc_val_y, acc_val_z);// adc_acc_x, adc_acc_y, adc_acc_z); //write new log value line

				delay(150000);
			}

			GPIO_ClearPinsOutput(GPIOB, 1<<22u);

			/*
			 * Save log
			 */
#if USE_SD
			res = f_open(&fil, "testi/testilog.csv", FA_WRITE | FA_READ | FA_OPEN_APPEND );
			SD_error(res,"file open"); //check for operation error

			f_printf(&fil, logresult_buffer);
			res = f_close(&fil);
			SD_error(res, "close");

			GPIO_SetPinsOutput(GPIOB, 1<<22u);
#endif
			memset(logresult_buffer,0,sizeof(logresult_buffer)); //flush logging buffer

		}

	printf("ready\r\n");

	while(1){ //stop here after logging run
		;
	}
	}


}



