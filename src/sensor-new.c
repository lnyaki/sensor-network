/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */
#include "contiki.h"
#include "net/rime/rime.h"
#include "lib/list.h"
#include "random.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"

#include <stdio.h>

/* Buffer size */
#define BUFFER_SIZE 100

/*TAG IDs*/
#define TAG_INFO 1
#define TAG_DISCOVERY 2
#define TAG_ACK_PARENT 3

/* This are the messages structures*/
struct info_message{
	int tag;
	int rank; // own rank
};	
	
struct simple_tag{
	int tag;
};

struct node{
	linkaddr_t addr; // parent address
	int rank;  //parent rank
};

/*Broadcast and unicast structures*/
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

/*nodes variables*/
//static node *parent;
LIST(sons_list); // dynamic list of nodes (all sons)

/*---------------------------------------------------------------------------*/
/*------------------      Process Declaration   -----------------------------*/
/*---------------------------------------------------------------------------*/
PROCESS(broadcast_process, "Broadcast Process");
PROCESS(unicast_process, "Unicast Process");
AUTOSTART_PROCESSES(&broadcast_process, &unicast_process);
/*---------------------------------------------------------------------------*/
/************************************************************************************
///////////////                                                       ///////////////
///////////////                  Messages Received                    ///////////////
///////////////                                                       ///////////////
************************************************************************************/
/****************************************************************************
*                                 CALLBACKS
*****************************************************************************/

static void unicast_received(struct unicast_conn *c, const linkaddr_t *from){
   printf("unicast message received from %d.%d\n",
	 from->u8[0], from->u8[1]);


	/*
	Unicast messages can have 2 reasons:
	New parent from a DIS
		=> ACK parent chosen
	Message from son
		=> pass to parent
		=> ACK son
	*/
	//packetbuf_copyfrom(&dis,sizeof(dis));
	//unicast_send(&unicast, from);
}

static void broadcast_received(struct broadcast_conn *c, const linkaddr_t *from){
	struct simple_tag *msg_rcv;
	struct info_message *msg_2_snd;

	/*data received*/
	msg_rcv = packetbuf_dataptr(); // pointer to data

	/*data to send*/
	msg_2_snd->tag = TAG_INFO;
	
	printf("broadcast message received from %d.%d: '%d'\n", from->u8[0], from->u8[1], msg_rcv->tag);

	/*checks if the broadcast message is a Dis*/
	if (msg_rcv->tag == TAG_DISCOVERY){
		packetbuf_copyfrom(&msg_2_snd,sizeof(msg_2_snd));
		unicast_send(&unicast, from);
		printf("Unicast message sent to son: %d\n", msg_2_snd->tag);
	}
}

/************************************************************************************
///////////////                                                       ///////////////
///////////////                  Messages to send                     ///////////////
///////////////                                                       ///////////////
************************************************************************************/
/****************************************************************************
*                                 CALLBACKS
*****************************************************************************/
void unicast_sent(struct unicast_conn *c, int status, int num_tx){
  const linkaddr_t *destination = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(destination, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d: status %d num_tx %d\n",
    destination->u8[0], destination->u8[1], status, num_tx);
}

void broadcast_sent(struct broadcast_conn *c, int status, int num_tx){

}


/****************************************************************************
*                                BROADCAST FUNCTIONS
*****************************************************************************/
void send_broadcast_message(struct broadcast_conn broadcast_connection,struct simple_tag message){
	packetbuf_copyfrom(&message, sizeof(struct simple_tag));
    broadcast_send(&broadcast_connection);
    printf("broadcast message sent %d\n",message.tag);
}

void send_unicast_message(){}

void sendHelloMessage(){}

void sendDiscoveryMessage(){}

void sendNodeInformationMessage(){}

void sendDataMessage(){}

void sendACK(){}


/************************************************************************************
///////////////                                                       ///////////////
///////////////                  sensor.c                             ///////////////
///////////////                                                       ///////////////
************************************************************************************/


/****************************************************************************
*                                 CALLBACKS
*****************************************************************************/
static const struct broadcast_callbacks broadcast_callback = {broadcast_received};
static const struct unicast_callbacks unicast_callback = {unicast_received};

/****************************************************************************
*                                 CONNECTIONS
*****************************************************************************/
static struct broadcast_conn broadcast_connection;
static struct unicast_conn unicast_connection;

/****************************************************************************
*                                 CONSTANTS
*****************************************************************************/
const int PROCESS_WAIT_TIME = 5;
const int BROADCAST_CHANNEL = 100;
const int UNICAST_CHANNEL 	= 101;

/*-------------------------------------------------------------------------*/

//The function that is executed when an event happens
void process_event(int ev, char* data){
	
	//if we received a message
	if(ev == PROCESS_EVENT_MSG){

    	//This is a mock example (funciton in messageSent.h)
    	//send_unicast_message(&unicast_connection, data);
	}

}


/*-------------------------------------------------------------------------*/
/****************************************************************************
*                               PROCESS THREAD
****************************************************************************/
/*------------------------    BROADCAST    --------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data){

	PROCESS_EXITHANDLER(broadcast_close(&broadcast_connection);)
	PROCESS_BEGIN();

	broadcast_open(&broadcast_connection, BROADCAST_CHANNEL,&broadcast_callback);

	/* Variables initialization */
	static struct etimer et;
	struct simple_tag dis;


	dis.tag = TAG_DISCOVERY; // Looking for parent

	while(1){

    
    	etimer_set(&et, CLOCK_SECOND * PROCESS_WAIT_TIME);
    

		/* Delay 2-4 seconds */
		etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    	send_broadcast_message(broadcast_connection, dis);
	}

	PROCESS_END();
}

/*------------------------    UNICAST    --------------------------------*/
/*This unicast process will be used to send the data to the parent*/
PROCESS_THREAD(unicast_process, ev, data){
	PROCESS_EXITHANDLER(unicast_close(&unicast_connection);)

	PROCESS_BEGIN();

	unicast_open(&unicast_connection, UNICAST_CHANNEL, &unicast_callback);

	while(1) {
		static struct etimer et;
		/*
		linkaddr_t addr;
		struct simple_tag dio;
		struct node parent;
		*/
		etimer_set(&et, 100*CLOCK_SECOND);

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		//packetbuf_copyfrom(&dio,sizeof(struct simple_tag)); // TODO pk &dis, normal pour une struct ?
		
		//addr.u8[0] = 1;
		//addr.u8[1] = 0;
		//if(!linkaddr_cmp(&papa->addr, &linkaddr_node_addr)) {
		//	unicast_send(&unicastc, &parent->addr);
		//}

	}

	PROCESS_END();
}
