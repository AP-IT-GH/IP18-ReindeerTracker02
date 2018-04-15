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
#include "at_func.h"
#include "timing.h"

extern volatile uint8_t UART3_strReady;
extern uint8_t PCprint(char *data);

extern char NB_recBuf[];
extern char NB_bufPtr;

uint8_t res;

const char postHeader[] =
		"POST /Reindeertracker/API/data/index.php HTTP/1.1\r\n"
				"Host: 168.235.64.81\r\n"
				"Content-Length: %d\r\n"
				"Content-Type: application/json\r\n"
				"Cache-Control: no-cache\r\n\r\n";

const char* client_id = "reindeertracker";
const char* topic = "reindeer";
const char* username = "reindeer";
const char* passwd = "reindeer1234";

uint8_t assembleMqtt(reindeerData_t *reindeerData, char *mqttMessage)
{

	uint8_t clientid_lt = strlen(client_id);
	uint8_t pass_lt = strlen(passwd);
	uint8_t user_lt = strlen(username);

	uint8_t connect_command[] =
	{ 0x10, clientid_lt + pass_lt + user_lt + 10 + 6, 0x00, 0x04, 0x4d, 0x51,
			0x54, 0x54, 0x04, 0xc2, 0x00, 0x0a, 0x00, clientid_lt };

	uint8_t packet_ptr = 0;

	char udpMessage[350];

	memmove(udpMessage, connect_command, sizeof(connect_command)); //move connect command header to the beginning of udpMessage
	packet_ptr += sizeof(connect_command); //increment packet pointer by the added data length so we know where to write to the packet next

	memmove(udpMessage + packet_ptr, client_id, clientid_lt); //add client id next
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

	PCprint(jsonMessage);

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

	char *messagePtr = mqttMessage;

	for (packet_ptr = 0; packet_ptr < packet_len; packet_ptr++) //print the packet as hex dump for debugging
	{

		messagePtr += sprintf(messagePtr, "%02x",
				(uint8_t) udpMessage[packet_ptr]);

		//if ((packet_ptr + 1) > 0 && ((packet_ptr + 1) % 15 == 0))
		//PCprint("\n"); //this just changes line after 16 bytes printed

	}

	//PCprint("packet length %d\r\n",packet_len);

	return packet_len;
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

	PCprint(jsonMessage);

	//strcpy(udpMessage,postHeader);

	int udpLength = sprintf(udpMessage, postHeader, length);

	strcpy(udpMessage + udpLength, jsonMessage);

	//PCprint("%s\r\n", udpMessage);

	udpLength = strlen(udpMessage);

	//PCprint("length of udp msg %d \r\nPrinting UDP message in hex\r\n",
	//		udpLength);

	for (uint16_t p = 0; p < udpLength; p++)
	{

		//PCprint("%02x", udpMessage[p]);
	}

	PCprint("\r\n");

}

void NB_received_data()
{

	uint32_t time_limit = 1000000;

	NB_bufPtr = 0;
	memset(NB_recBuf, 0, strlen(NB_recBuf));

	while (time_limit--)
	{

		if (UART3_strReady)
		{

			if (strstr(NB_recBuf, "NSONMI") != NULL) //if received buffer contains expected answer
			{
				PCprint(NB_recBuf);
				PCprint("received\r\n");
				break;
			}
			else if (strstr(NB_recBuf, "ERROR") != NULL)
			{

				break;
			}
			UART3_strReady = 0;
		}
	}
	NB_bufPtr = 0;
	memset(NB_recBuf, 0, strlen(NB_recBuf));
}

void NB_create_pdp_send(char *mqttMessage, uint8_t msgLen)
{

	uint8_t reSend_msg = 0;

	while(NB_setPin() == 1) //if setPin returns error then reboot and try again
	{
		NB_reboot();
	}

	delay_ms(250);  //viivettä pitää olla
	NB_define_pdp();

	do
	{
		NB_active_pdp();
		NB_network_status();
		/*if(reSend_msg == 1){
		 NB_network_status();
		 //delay_ms(1000);
		 }*/

		NB_show_ip();
		NB_create_socket();
		reSend_msg = NB_send_msg(mqttMessage, msgLen);

	} while (reSend_msg == 1);

}

void NB_reboot()
{

	res = AT_send(AT_NRB, "", "+UFOTAS");
	if (res == 0)
	{
		PCprint("rebooted\r\n");
	}
	else if (res == 1)
	{
		PCprint("error\r\n");
	}
	else if (res == 2)
	{
		PCprint("timeout error\r\n");
	}

}
uint8_t NB_setPin()
{


	res = AT_send(AT_NPIN, "", "+NPIN: \"OK\"");
	if (res == 0)
	{
		PCprint("ack\r\n");
		return 0;
	}
	else if (res == 1)
	{
		PCprint("error\r\n");
		return 1;
	}

	return 1;

}
void NB_cops_register()
{

	res = AT_send(AT_COPS, "=0", "OK");   //0 Register to network
	if (res == 0)
	{
		PCprint("Registered\r\n");
	}
	else if (res == 1)
	{
		PCprint("error\r\n");
	}
}
void NB_cops_deRegister()
{

	res = AT_send(AT_COPS, "=2", "OK");   //2 De-Register from network
	if (res == 0)
	{
		PCprint("De-registered\r\n");
	}
	else if (res == 1)
	{
		PCprint("error\r\n");
	}
}
void NB_cops_readRegister()
{

	res = AT_send(AT_COPS, "?", "OK");   //2 De-Register from network
	if (res == 0)
	{
		PCprint("readCOPS\r\n");
	}
	else if (res == 1)
	{
		PCprint("error\r\n");
	}
}
void NB_network_status()
{
	res = 2;
	while (res != 0)
	{
		res = AT_send(AT_CEREG, "", "+CEREG: 0,1");
		if (res == 0)
		{
			PCprint("CEREG_OK\r\n");
		}
		else if (res == 1)
		{
			PCprint("error\r\n");
		}
	}
}
void NB_define_pdp()
{
	res = AT_send(AT_CGDCONT, "", "OK");
	delay_ms(100);
	if (res == 0)
	{
		PCprint("PDP context 1 defined\r\n");
	}
	else if (res == 1)
	{
		PCprint("error\r\n");
	}
}
void NB_active_pdp()
{
	res = AT_send(AT_CGACT, "=1,1", "OK");     //Active PDP context 1

	if (res == 0)
	{
		PCprint("PDP 1 activated\r\n");
	}
	else if (res == 1)
	{
		PCprint("error\r\n");
	}
}
void NB_show_ip()
{
	res = AT_send(AT_CGPADDR, "", "OK");     //Show ip address

	if (res == 0)
	{
		PCprint("ip  found\r\n");
	}
	else if (res == 1)
	{
		PCprint("error\r\n");
	}
}
void NB_create_socket()
{
	res = AT_send(AT_NSOCR, "", "OK");     //Create UDP socket
	if (res == 0)
	{
		PCprint("Socket ready\r\n");
	}
	else if (res == 1)
	{
		PCprint("error\r\n");
	}
}
uint8_t NB_send_msg(char *mqttMessage, uint8_t msgLen)
{
	char nsost_command[500];
	sprintf(nsost_command, "0,\"167.99.207.133\",1884,%d,\"%s\"", msgLen,
			mqttMessage);

	//PCprint("%s\r\n",nsost_command);

	res = AT_send(AT_NSOST, nsost_command, "OK");     //Send message to server

	if (res == 0)
	{
		PCprint("sent");
	}
	else if (res == 1)
	{
		PCprint("error\r\n");
		return 1;
	}
	return 0;
}
void NB_read_msg()
{
	res = AT_send(AT_NSORF, "", "OK");     //read echo data
	//delay_ms(1000);
	if (res == 0)
	{
		PCprint("echo\r\n");
	}
	else if (res == 1)
	{
		PCprint("error\r\n");
	}
}


