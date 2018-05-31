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
	int rank;  //parent rank
};

struct info_message{
	int tag;
	struct node *node_info; // node info
};	
	
struct simple_tag{
	int tag;
};



/* Broadcast and unicast structures */
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

/* Nodes variables */
static uint8_t node_rank = 0;
static struct node *parent = NULL;
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
	if (msg_rcv->tag == TAG_DISCOVERY){
		/*data to send*/

		msg_2_snd[0] = TAG_VADOR; // sending node's information (rank, addr)
		msg_2_snd[1] = node_rank;
		msg_2_snd[2] = linkaddr_node_addr.u8[0];
		msg_2_snd[3] = linkaddr_node_addr.u8[1];
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
			pong.tag = TAG_ACK_INFO;
			packetbuf_copyfrom(&pong,sizeof(struct simple_tag));
			unicast_send(&unicast, from);
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

	/*while(1) {

		
		etimer_set(&et, CLOCK_SECOND * 8 + random_rand() % (CLOCK_SECOND * 4));

		
		
		
		if(list_head(vadors_list) != NULL && parent == NULL){
			struct node *vador_list_ptr = list_head(vadors_list); // father's list pointer
			int best_rank = vador_list_ptr->rank;
			struct node *i;
			for(i = vador_list_ptr; i != NULL; i = list_item_next(i)){
				if(vador_list_ptr->rank < best_rank && vador_list_ptr->rank < node_rank){
					best_rank = vador_list_ptr->rank;
					parent = vador_list_ptr;
				}
			}

			
			node_rank = best_rank+1; // 
		}
		
		if(parent == NULL){ // no parent then rebroadcast
			packetbuf_copyfrom(&dis,sizeof(struct simple_tag));
			broadcast_send(&broadcast);
			printf("Looking for my vader %d\n",dis.tag);
		}

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		
	}*/
	
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
		linkaddr_t addr;
		struct simple_tag dio;
		struct node parent;

		etimer_set(&et, 100*CLOCK_SECOND);

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		/* If I get a serial input transform and send to lukes */
	}

	PROCESS_END();
}