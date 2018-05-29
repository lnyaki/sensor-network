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
//Close the broadcast and unicast connections
int close_connection(&broadcast, &unicast){
	broadcast_close(&broadcast)
	unicast_close(&unicast)
}

//The function that is executed when an event happens
int process_event(int ev){

}

//The function that is executed when the timer runs out
int periodic_processing(){
	
}
/****************************************************************************
*                               PROCESS THREAD
*****************************************************************************/
PROCESS_THREAD(example_broadcast_process, ev, data){
	PROCESS_EXITHANDLER(close_connection(&broadcast, &unicast_connection);)
	PROCESS_BEGIN();

	//(<connection>, <channel>, <callbacks>)
	broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);
	unicast_open(&unicast_connection, UNICAST_CHANNEL, &unicast_callbacks);

	//Data to broadcast
	char * data;

	while(1){
		static struct etimer et;
    	linkaddr_t addr;
    
    	etimer_set(&et, CLOCK_SECOND * PROCESS_WAIT_TIME);
    
    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    	//Check if timer expired
    	if(etimer_expired(&et)){
    		periodic_processing();
    	}

    	//Otherwise, an event has happened
    	else{
    		process_event(ev);
    	}
    		

    	//Defined in messageSent.h.
    	send_broadcast_message(&broadcast, data)

    	//Defined in messageSent.h
    	send_unicast_message(&unicast, data)
	}

	PROCESS_END();
}