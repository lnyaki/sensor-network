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

/**
 * \file
 *         Testing the broadcast layer in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "lib/list.h"
#include "random.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"

#include <stdio.h>

/* TAG IDs */
#define TAG_INFO 1
#define TAG_ACK_INFO 2
#define TAG_DISCOVERY 3
#define TAG_VADOR 4
#define TAG_ACK_PARENT 5




/* This are the messages structures */
struct node{
	struct node *next; // we need it for our fathers and sons list
	linkaddr_t addr; // parent address
	uint8_t rank;  //parent rank
};
	
struct simple_tag{
	uint8_t tag;
};

/* Broadcast and unicast structures */
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

/* Nodes variables */
static uint8_t node_rank = 255;
static struct node parent;
static char has_parent = 0; // boolean for "has a parent"
static char has_connection = 0; // boolean for "received ack from parent after ping"
static char has_tried_connecting = 0; //  number of tries to reach parent

LIST(lukes_list); // dynamic list of nodes (all lukes)
LIST(vadors_list); // dynamic list of possible fathers

/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
PROCESS(example_unicast_process, "Example unicast");

AUTOSTART_PROCESSES(&example_broadcast_process,&example_unicast_process);
/*---------------------------------------------------------------------------*/

/* Function called when receiving a brodcast message*/
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from){
	
	struct simple_tag *msg_rcv;
	//struct info_message *msg_2_snd;
	uint8_t msg_2_snd[4];

	/*data received*/
	msg_rcv = packetbuf_dataptr(); // pointer to data
	
	printf("DAD REQUEST from %d.%d: '%d'\n", from->u8[0], from->u8[1], 
msg_rcv->tag);
	/*checks if the broadcast message is a Dis*/
	if (msg_rcv->tag == TAG_DISCOVERY && has_parent){
		/*data to send*/

		msg_2_snd[0] = TAG_VADOR; // sending node's information (rank, addr)
		msg_2_snd[1] = node_rank;
		msg_2_snd[2] = linkaddr_node_addr.u8[0]; // TODO pas besoin info déjà présente
		msg_2_snd[3] = linkaddr_node_addr.u8[1]; // TODO remove
		packetbuf_copyfrom(msg_2_snd,sizeof(msg_2_snd));
		unicast_send(&unicast, from);
		printf("DAD(%d.%d) RESPONSE to possible son %d.%d: '%d'\n",linkaddr_node_addr.u8[0],linkaddr_node_addr.u8[1], from->u8[0], from->u8[1], msg_2_snd[0]);
	}
}

/* Function called when receiving a unicast message */
static void recv_uc(struct unicast_conn *c, const linkaddr_t *from){
		
	uint8_t *ptr_data = packetbuf_dataptr();

	printf("unicast message received from %d.%d with this beautiful tag: %d\n",
	 from->u8[0], from->u8[1],ptr_data[0]);

	struct node *inc_node_info = malloc(sizeof(struct node));
	struct simple_tag pong;

	switch(ptr_data[0]) {

		case TAG_VADOR :
			printf("Adding daddy to the list\n");
			linkaddr_copy(&inc_node_info->addr,from);
			inc_node_info->rank = ptr_data[1];
			list_add(vadors_list,inc_node_info);
			break;
		case TAG_ACK_PARENT:
			printf("Adding son to the list\n");
			linkaddr_copy(&inc_node_info->addr,from);
			inc_node_info->rank = ptr_data[1];
			list_add(lukes_list,inc_node_info);
			break;
		case TAG_INFO:
			/* Ping from luke */
			printf("Ping from the son\n");
			pong.tag = TAG_ACK_INFO;
			packetbuf_copyfrom(&pong,sizeof(struct simple_tag));
			unicast_send(&unicast, from);
			break;
		case TAG_ACK_INFO:
			/* Pong from parent */
			printf("Pong from the parent\n");
			has_tried_connecting = 0;
			//has_connection = 1;
			break;
	}
	
}

/*---------------------------------------------------------------------------*/

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

/*---------------------------------------------------------------------------*/

/* This broadcast process will be used to find a parent */

PROCESS_THREAD(example_broadcast_process, ev, data){
	static struct etimer et;
	struct simple_tag dis;

	dis.tag = TAG_DISCOVERY; // Looking for parent

	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);

	while(1) {

		/* Delay */
		etimer_set(&et, CLOCK_SECOND * 10 + random_rand() % (CLOCK_SECOND * 4));

		/* Check list of potential parents and make choice */
		
		if(list_head(vadors_list) != NULL && !has_parent){
			printf("My current rank: %d\n before chosing parent", node_rank);
			//struct node *vador_list_ptr = list_head(vadors_list); // father's list pointer
			uint8_t best_rank = -1;	
			//printf("DEBUG: parent's address %x %d.%d\n", &vador_list_ptr->addr, vador_list_ptr->addr.u8[0],vador_list_ptr->addr.u8[1]);
			struct node *i;
			for(i = list_head(vadors_list); i != NULL; i = list_item_next(i)){
				printf("current proposition rank: %d\n",i->rank);
				if(i->rank < best_rank && i->rank < node_rank){
					best_rank = i->rank;
					linkaddr_copy(&parent.addr,&i->addr);
					//parent.addr = i->addr;
					//printf("DEBUG: parent's address %d.%d\n", parent.addr.u8[0],parent.addr.u8[1]);
					parent.rank = best_rank;
				}
			}

			// TODO Comment free la liste ?

			
			/* Define its own rank regarding the father */
			node_rank = best_rank+1;
			
			printf("My rank after daddy check: %d\n", node_rank);

			has_parent = 1;
			//has_connection = 1;
			

			/* Send ACK to father */
			uint8_t msg_2_snd[4];

			msg_2_snd[0] = TAG_ACK_PARENT;
			msg_2_snd[1] = node_rank;
			msg_2_snd[2] = linkaddr_node_addr.u8[0];
			msg_2_snd[3] = linkaddr_node_addr.u8[1];
			packetbuf_copyfrom(msg_2_snd,sizeof(msg_2_snd));
			unicast_send(&unicast, &parent.addr);
			printf("Sending ACK to parent %d.%d\n",parent.addr.u8[0],parent.addr.u8[1]); //  problem with addresses
		}

		
		
		if(has_parent == 0){// || has_connection == 0){ // no parent or not connected anymore then rebroadcast
			printf("No parent or no connection\n");
			packetbuf_copyfrom(&dis,sizeof(struct simple_tag));
			broadcast_send(&broadcast);
			printf("Looking for a father %d\n",dis.tag);
		}

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		
	}
	
	PROCESS_END();
}

/*---------------------------------------------------------------------------*/

static const struct unicast_callbacks unicast_callbacks = {recv_uc};

/*---------------------------------------------------------------------------*/

/*This unicast process will be used to send the data to the parent*/

PROCESS_THREAD(example_unicast_process, ev, data){
	PROCESS_EXITHANDLER(unicast_close(&unicast);)

	PROCESS_BEGIN();

	unicast_open(&unicast, 146, &unicast_callbacks);

	while(1) {
		static struct etimer et;
		struct simple_tag ping;

		etimer_set(&et, 100*CLOCK_SECOND);

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		
		if(has_parent && has_tried_connecting < 3){
			printf("TRYING TO PING DADDY\n");
			ping.tag = TAG_INFO;
			packetbuf_copyfrom(&ping,sizeof(struct simple_tag));
			unicast_send(&unicast, &parent.addr);
			has_tried_connecting++;
		}else if(has_tried_connecting >= 3){
			/* New node situation */
			has_tried_connecting = 0; 
			//has_connection = 0; 
			has_parent = 0;
			node_rank = 255;
		}		

	}
	PROCESS_END();
}