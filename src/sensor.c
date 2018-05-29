#include "contiki.h"
#include "net/rime/rime.h"
#include <stdio.h>

//Our own includes
#include "messages/receiveMessage.h"
#include "messages/sendMessage.h"
#include "sensor.h"

/*---------------------------------------------------------------------------*/
/*---------------------        Start Process    -----------------------------*/
/*---------------------------------------------------------------------------*/
PROCESS(sensor_code, "Sensor Communication Process");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/





/****************************************************************************
*                                 CALLBACKS
*****************************************************************************/
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct unicast_callbacks unicast_callbacks = {unicast_received, unicast_sent};
static struct unicast_conn unicast_connection;


/*-------------------------------------------------------------------------*/
int open_connections(){
	broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);
	unicast_open(&unicast_connection, UNICAST_CHANNEL, &unicast_callbacks);
}
//Close the broadcast and unicast connections
int close_connections(&broadcast, &unicast){
	broadcast_close(&broadcast)
	unicast_close(&unicast)
}

//The function that is executed when an event happens
int process_event(int ev){
	//if we received a message
	if(ev == PROCESS_EVENT_MSG){

    	//This is a mock example (funciton in messageSent.h)
    	send_unicast_message(&unicast, data)
	}
}

//The function that is executed when the timer runs out
int periodic_processing(){
	//Defined in messageSent.h.
    send_broadcast_message(&broadcast, data)
}

/****************************************************************************
*                               PROCESS THREAD
ffffffff***********************************************************f*************/
PROCESS_THREAD(example_broadcast_process, ev, data){
	PROCESS_EXITHANDLER(close_connections(&broadcast, &unicast_connection);)
	PROCESS_BEGIN();

	open_connections()

	//Data to broadcast
	char * data;

	while(1){
		static struct etimer et;
    	linkaddr_t addr;
    
    	etimer_set(&et, CLOCK_SECOND * PROCESS_WAIT_TIME);
    
    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    	//Check if timer expired
    	if(ev == PROCESS_EVENT_TIMER){
    		periodic_processing();
    	}

    	//Otherwise, another event event has happened
    	else{
    		process_event(ev);
    	}
	}

	PROCESS_END();
}