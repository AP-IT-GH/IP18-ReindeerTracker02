/*
 * acc_func.c
 *
 *  Created on: Feb 9, 2018
 *      Author: nks
 */

#include <stdint.h>
#include <stdio.h>
#include "i2c_func.h"
#define X_AXIS 	0
#define Y_AXIS 	1
#define Z_AXIS 	2

#include "adc_func.h"


extern void UART_print(char *data);

unsigned char buffer[50];

void acc_init(){
	 accWriteReg(0x2a,0x01); //write accelerometer CTRL_REG1 (active mode)


}
int16_t read_acc_axis(uint8_t axis) {

	switch ( axis ) {
	case 0:
		axis = 0x01;
		break;

	case 1:
		axis = 0x03;
		break;

	case 2:
		axis = 0x05;
		break;

	}

	  uint16_t acc_val = 0; //init a 16-bit variable to store 14-bit acceleration value

	  uint8_t acc_buf = accReadReg(axis); //read MSB bits of acceleration value

	  acc_val = acc_buf; //read MSB bits to the 16 bit variable

	  //0000 0000 MMMM MMMM at this stage acc_val looks like this in memory

	  acc_val <<= 8; // shift MSB bits left to have them in right place

	  //MMMM MMMM 0000 0000

	  acc_buf = accReadReg(axis + 1); //read LSB values to buffer
	  acc_val |= acc_buf; //OR LSB values to the acc_value

	  //MMMM MMMM LLLL LL00
	  acc_val >>= 2; //shift right 2 bits to right-justify

	  //00MM MMMM MMLL LLLLpaskaa

	  int16_t out = 0;

	  if(acc_val & (1 << 13)) //test if value is negative by masking 14th bit
	  {
		  acc_val = 0xffff - acc_val +1;
		  out = 0 - (acc_val & 0x1fff);

	  }

	  else
	  {
		  out = acc_val & 0x1fff;
	  }
	  return out;
}

int16_t print_ext_acc_axis(void) {


	int16_t adc_acc_x = ADC_read16b(1) - 32900;
	int16_t adc_acc_y = ADC_read16b(2) - 32900;		//Accelerometer GY-61
	int16_t adc_acc_z = ADC_read16b(3) - 32900;
	int16_t acc_val_x = read_acc_axis(X_AXIS); //read accelerometer X axis
	int16_t acc_val_y = read_acc_axis(Y_AXIS);
    int16_t acc_val_z = read_acc_axis(Z_AXIS);

	printf("X: %d\tY: %d\tZ: %d\t", adc_acc_x, adc_acc_y, adc_acc_z );  //Accelerometer GY-61
	printf("X:%d\tY: %d\tZ: %d\r\n", acc_val_x, acc_val_y, acc_val_z);  //FRDM integrated accelerometer

	return 0;

}

