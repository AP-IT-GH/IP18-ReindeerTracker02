/*
 * ubx_func.c
 *
 *  Created on: Mar 19, 2018
 *      Author: nks
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ubx_func.h"

char PMC_set[] = { 0XB5, 0X62, 0X06, 0X86, 0X08, 0X00, 0X00, 0X01, 0X00, 0X00,
		0X00, 0X00, 0X00, 0X00 };
char ubx_cfg_prt[] = { 0XB5, 0X62, 0X06, 0X00, 0X01, 0X00, 0X01 };
const char ubx_ack[] = { 0xB5, 0x62};

extern volatile uint8_t UART3_strReady;
extern char UART3_recBuf[];
extern void UART3_send(char *data);

uint8_t fletcher8(char *crc, uint8_t len) {

	uint8_t CK_A = 0;
	uint8_t CK_B = 0;
	uint8_t i = 0;

	for (i = 2; i < len; i++) {

		CK_A = CK_A + crc[i];
		CK_B = CK_B + CK_A;
	}

	crc[len] = CK_A;
	crc[len + 1] = CK_B;


}

void ubx_send(char* ubx_cmd, uint8_t len) {

	uint32_t timeout = 10000000;
	fletcher8(ubx_cmd, 7);
/*
	printf("Printing UBX-msg IN ubx func\r\n");

	for (uint8_t i = 0; i < len + 2; i++) {
		printf("%02x", (unsigned char) ubx_cmd[i]);
	}

	printf("\r\n");
*/
	UART3_send(ubx_cmd);

	while (1) {

		while ( timeout  > 0 ) {
			timeout--;
		}

		printf("waiting\r\n");

		if (UART3_strReady) {
			//printf("%s\r\n",UART3_recBuf);
			UART3_strReady = 0;

			if (memcmp(UART3_recBuf, ubx_ack, 1000) == 0) {

				for (uint8_t i = 0; i < strlen(UART3_recBuf) + 2; i++) {
					//printf("%02x", (unsigned char) ubx_cmd[i]);
				}

				printf("got ubx\r\n");
				memset(UART3_recBuf, 0, strlen(UART3_recBuf));
				break;

			}

		}

	}

}
