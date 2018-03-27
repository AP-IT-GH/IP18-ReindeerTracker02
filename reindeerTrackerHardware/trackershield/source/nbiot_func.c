/*
 * nbiot_func.c
 *
 *  Created on: Mar 16, 2018
 *      Author: teemu
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nbiot_func.h"

const char postHeader[] =
		"POST /Reindeertracker/API/data/index.php HTTP/1.1\r\n"
				"Host: 168.235.64.81\r\n"
				"Content-Length: %d\r\n"
				"Content-Type: application/json\r\n"
				"Cache-Control: no-cache\r\n\r\n";

const char client_id[] = "reindeertracker";
const char topic[] = "reindeer";
const char username[] = "reindeer";
const char passwd[] = "reindeer1234";

void assembleMqtt(reindeerData_t *reindeerData, char *udpMessage)
{

	uint8_t clientid_lt = strlen(client_id);
	uint8_t pass_lt = strlen(passwd);
	uint8_t user_lt = strlen(username);
	//M		Q		T    T   prLe  flag  kea   kea
	uint8_t connect_command[] =
	{ 0x10, clientid_lt + pass_lt + user_lt + 10 + 6, 0x00, 0x04, 0x4d, 0x51,
			0x54, 0x54, 0x04, 0xc2, 0x00, 0x0a, 0x00, clientid_lt };

	uint8_t packet_ptr = 0;

	memmove(udpMessage, connect_command, sizeof(connect_command)); //move connect command header to the beginning of udpMessage
	packet_ptr += sizeof(connect_command); //increment packet pointer by the added data length so we know where to write to the packet next

	memmove(udpMessage + packet_ptr, client_id, sizeof(client_id)); //add client id next
	packet_ptr += clientid_lt;

	uint8_t w_buf[5] =
	{ 0x00, user_lt };

	memmove(udpMessage + packet_ptr, w_buf, 2); //put user length..
	packet_ptr += 2;

	memmove(udpMessage + packet_ptr, username, user_lt); 	//put user
	packet_ptr += user_lt;

	w_buf[1] = pass_lt;

	memmove(udpMessage + packet_ptr, w_buf, 2); //put passwd length..
	packet_ptr += 2;

	memmove(udpMessage + packet_ptr, passwd, pass_lt); 	//put passwd
	packet_ptr += pass_lt;

	uint8_t topic_lt = strlen(topic);

	int length = 0;

	static char jsonMessage[150];

	length += sprintf(jsonMessage, "{\r\n\r\n    \"serialnumber\":\"%s\",\r\n",
			reindeerData->serialNum);
	length += sprintf(jsonMessage + length, "    \"lat\":\"%s\",\r\n",
			reindeerData->latitude);
	length += sprintf(jsonMessage + length, "    \"long\":\"%s\",\r\n",
			reindeerData->longitude);
	length += sprintf(jsonMessage + length, "    \"status\":\"%s\",\r\n",
			reindeerData->dead);
	length += sprintf(jsonMessage + length, "    \"battery\":\"%d\"\r\n\r\n}",
			reindeerData->batteryLevel);

	printf(jsonMessage);

	w_buf[0] = 0x30;
	w_buf[1] = 0x00;
	w_buf[2] = 0x00;
	w_buf[3] = 0x00;
	w_buf[4] = topic_lt; //put in publish command

	w_buf[1] = topic_lt + length + 2; //replace 0 with length of publish message

	uint8_t x = w_buf[1];
	uint8_t encodedByte = 0;

	encodedByte = x % 128;

	x = x / 128;

	encodedByte = encodedByte | 128;

	w_buf[1] = encodedByte;

	w_buf[2] = x;

	memmove(udpMessage + packet_ptr, w_buf, 5);
	packet_ptr += 5;

	memmove(udpMessage + packet_ptr, topic, topic_lt);
	packet_ptr += topic_lt;

	memmove(udpMessage + packet_ptr, jsonMessage, length);
	packet_ptr += length;

	uint8_t packet_len = packet_ptr;

	for (packet_ptr = 0; packet_ptr < packet_len; packet_ptr++) //print the packet as hex dump for debugging
	{

		printf("%02x", (uint8_t)udpMessage[packet_ptr]);

		//if ((packet_ptr + 1) > 0 && ((packet_ptr + 1) % 15 == 0))
			//printf("\n"); //this just changes line after 16 bytes printed

	}

	printf("packet length %d\r\n",packet_len);
}

void assemblePacket(reindeerData_t *reindeerData, char *udpMessage)
{

	int length = 0;

	static char jsonMessage[150];

	length += sprintf(jsonMessage, "{\r\n\r\n    \"serialnumber\":\"%s\",\r\n",
			reindeerData->serialNum);
	length += sprintf(jsonMessage + length, "    \"lat\":\"%s\",\r\n",
			reindeerData->latitude);
	length += sprintf(jsonMessage + length, "    \"long\":\"%s\",\r\n",
			reindeerData->longitude);
	length += sprintf(jsonMessage + length, "    \"status\":\"%s\",\r\n",
			reindeerData->dead);
	length += sprintf(jsonMessage + length, "    \"battery\":\"%d\"\r\n\r\n}",
			reindeerData->batteryLevel);

	printf(jsonMessage);

	//strcpy(udpMessage,postHeader);

	int udpLength = sprintf(udpMessage, postHeader, length);

	strcpy(udpMessage + udpLength, jsonMessage);

	printf("%s\r\n", udpMessage);

	udpLength = strlen(udpMessage);

	printf("length of udp msg %d \r\nPrinting UDP message in hex\r\n",
			udpLength);

	for (uint16_t p = 0; p < udpLength; p++)
	{

		printf("%02x", udpMessage[p]);
	}

	printf("\r\n");

}

/*
 void NB_setPin(char* pinCode) {

 char cmd_buf[100];

 sprintf(cmd_buf, "%s2,\"%s\"\r\n", AT_NPIN, pinCode);

 printf(cmd_buf);

 }
 */