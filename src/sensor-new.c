#include "contiki.h"
#include "net/rime.h"
#include <stdio.h>

//Our own includes
#include "messages/receiveMessage.h"
#include "messages/sendMessage.h"
//#include "sensor.h"

/*---------------------------------------------------------------------------*/
/*---------------------        Start Process    -----------------------------*/
/*-----------------------------------------------------------------*----------*/
PROCESS(sensor_code, "Sensor Communication Process");
AUTOSTART_PROCESSES(&sensor_code);
/*---------------------------------------------------------------------------*/





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
int open_connections(struct broadcast_conn *broadcast, struct unicast_conn *unicast){

	broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);
	//unicast_open(&unicast, UNICAST_CHANNEL, &unicast_callback);
	
	return 0;
}
//Close the broadcast and unicast connections
int close_connections(struct broadcast_conn broadcast, struct unicast_conn unicast){
	
	broadcast_close(&broadcast);
	unicast_close(&unicast);

		return 0;

}

//The function that is executed when an event happens
int process_event(int ev, char* data){
	
	//if we received a message
	if(ev == PROCESS_EVENT_MSG){

    	//This is a mock example (funciton in messageSent.h)
    	send_unicast_message(&unicast_connection, data);
	}

	return 0;
}

//The function that is executed when the timer runs out
int periodic_processing(struct broadcast_conn broadcast,char* data){
	
	//Defined in messageSent.h.
    send_broadcast_message(&broadcast, data);
    
    return 0;

}
/*-------------------------------------------------------------------------*/
/****************************************************************************
*                               PROCESS THREAD
****************************************************************************/
PROCESS_THREAD(sensor_code, ev, data){
	PROCESS_EXITHANDLER(close_connections(broadcast_connection, unicast_connection);)
	PROCESS_BEGIN();

	open_connections(&broadcast_connection, &unicast_connection);

	//Data to broadcast
	char * data = "toto";

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
