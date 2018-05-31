#include "contiki.h"
#include "net/rime/rime.h"
#include <stdio.h>

//Our own includes
//#include "sensor.h"

/*---------------------------------------------------------------------------*/
/*---------------------        Start Process    -----------------------------*/
/*-----------------------------------------------------------------*----------*/
PROCESS(sensor_code, "Sensor Communication Process");
AUTOSTART_PROCESSES(&sensor_code);
/*---------------------------------------------------------------------------*/
/************************************************************************************
///////////////                                                       ///////////////
///////////////                  Messages Received                    ///////////////
///////////////                                                       ///////////////
************************************************************************************/
/****************************************************************************
*                                 CALLBACKS
*****************************************************************************/

static void unicast_received(struct unicast_conn *c, const linkaddr_t *sender)
{
  printf("unicast message received from %d.%d\n",
	 sender->u8[0], sender->u8[1]);
}

static void broadcast_received(struct broadcast_conn *c, const linkaddr_t *sender)
{
  printf("broadcast message received from %d.%d: '%s'\n",
         sender->u8[0], sender->u8[1], (char *)packetbuf_dataptr());
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
void send_broadcast_message(struct broadcast_conn broadcast_connection,char[50] message){
	packetbuf_copyfrom(message, strlen(message));
    broadcast_send(&broadcast_connection);
    printf("broadcast message sent\n");
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
static const struct broadcast_callbacks broadcast_call = {broadcast_received};
static struct broadcast_conn broadcast_connection;
static const struct unicast_callbacks unicast_callback = {unicast_received};
static struct unicast_conn unicast_connection;

//Waiting time of the process, in seconds
const int PROCESS_WAIT_TIME = 5;
const int BROADCAST_CHANNEL = 100;
const int UNICAST_CHANNEL 	= 101;

/*-------------------------------------------------------------------------*/
void open_connections(struct broadcast_conn broadcast, struct unicast_conn unicast){

	broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);
	//unicast_open(&unicast, UNICAST_CHANNEL, &unicast_callback);
}
//Close the broadcast and unicast connections
void close_connections(struct broadcast_conn broadcast, struct unicast_conn unicast){
	
	broadcast_close(&broadcast);
	unicast_close(&unicast);
}

//The function that is executed when an event happens
void process_event(int ev, char* data){
	
	//if we received a message
	if(ev == PROCESS_EVENT_MSG){

    	//This is a mock example (funciton in messageSent.h)
    	//send_unicast_message(&unicast_connection, data);
	}

}

//The function that is executed when the timer runs out
void periodic_processing(struct broadcast_conn broadcast,char data){
	
	//Defined in messageSent.h.
    //send_broadcast_message(&broadcast, data);
    

}
/*-------------------------------------------------------------------------*/
/****************************************************************************
*                               PROCESS THREAD
****************************************************************************/
PROCESS_THREAD(sensor_code, ev, data){
	PROCESS_EXITHANDLER(close_connections(broadcast_connection, unicast_connection);)
	PROCESS_BEGIN();

	open_connections(broadcast_connection, unicast_connection);

	//Data to broadcast
	char data = 't';

	while(1){
		static struct etimer et;
    	//linkaddr_t addr = null;
    
    	etimer_set(&et, CLOCK_SECOND * PROCESS_WAIT_TIME);
    
    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    	//Check if timer expired
    	if(ev == PROCESS_EVENT_TIMER){
    		periodic_processing(broadcast_connection, data);
    	}

    	//Otherwise, another event event has happened
    	else{
    		process_event(ev, NULL);
    	}
	}

	PROCESS_END();
}
