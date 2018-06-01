#include "contiki.h"
#include "net/rime/rime.h"
#include "lib/list.h"
#include "random.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "lib/sensors.h"
#include "dev/i2cmaster.h"
#include "dev/tmp102.h"
#include "dev/adxl345.h"
#include "dev/battery-sensor.h"
#include "dev/serial-line.h"

#include <stdio.h>

/* TAG IDs */
#define TAG_INFO 1
#define TAG_ACK_INFO 2
#define TAG_DISCOVERY 3
#define TAG_VADOR 4
#define TAG_ACK_PARENT 5
#define TAG_ROOT 6

// #define TAG_ACK_ROOT 7 TODO Pas sur si nécessaire

#define UPDATE_MODE 8
#define PERIODIC_MODE 9
#define START_SEND 10
#define STOP_SEND 11



#define TAG_TEMP 'T'
#define TAG_BATTERY 'B'
#define TAG_ACCEL 'Z'
#define READ_INTERVAL (CLOCK_SECOND)

struct node{
    struct node *next; // we need it for our fathers and sons list
    linkaddr_t addr; // parent address
    uint8_t rank;  //parent rank
};

/* Broadcast and unicast structures */
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

/* Nodes variables */
static uint8_t node_rank = 255;
static struct node parent;
static uint8_t simple_tag; // byte used to send simple tags (pings, configs, ...)
static char has_parent = 0; // boolean for "has a parent"
static char has_connection = 0; // boolean for "received ack from parent after ping"
static char has_tried_connecting = 0; //  number of tries to reach parent
static uint8_t buffer[8]; // Possible buffer for data_message not ackd
static char periodic_mode = 1;
static char has_subscribers = 0;
static struct etimer et_temp;
static struct etimer et_accel;
static struct etimer et_battery;
static process_event_t new_subscriber;

LIST(lukes_list); // dynamic list of nodes (all lukes)
LIST(vadors_list); // dynamic list of possible fathers

/*---------------------------------------------------------------------------*/
PROCESS(broadcast_process, "Broadcast");
PROCESS(unicast_process, "Example");
PROCESS(temp_process, "TMP102 Temperature sensor process");
PROCESS(accel_process, "ADXL345 Accelerometer sensor process");
PROCESS(battery_process, "Battery sensor process");

AUTOSTART_PROCESSES(&unicast_process,&broadcast_process,&temp_process,&accel_process,&battery_process);
/*---------------------------------------------------------------------------*/

/* Function called when receiving a brodcast message*/
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from){

    //struct simple_tag *msg_rcv;
    //struct info_message *msg_2_snd;
    uint8_t msg_2_snd[6];

    /*data received*/
    simple_tag = *(uint8_t*) packetbuf_dataptr(); // pointer to data


    printf("DAD REQUEST from %d.%d: '%d'\n", from->u8[0], from->u8[1], simple_tag);
    /*checks if the broadcast message is a Dis*/
    if (simple_tag == TAG_DISCOVERY && has_parent){

        msg_2_snd[0] = TAG_VADOR; // sending node's information (rank, addr)
        msg_2_snd[1] = node_rank;
        msg_2_snd[2] = linkaddr_node_addr.u8[0]; 
        msg_2_snd[3] = linkaddr_node_addr.u8[1]; 
        msg_2_snd[4] = periodic_mode; // Config State
        msg_2_snd[5] = has_subscribers; // Config State
        packetbuf_copyfrom(msg_2_snd,sizeof(msg_2_snd));
        unicast_send(&unicast, from);
        printf("DAD(%d.%d) RESPONSE to possible son %d.%d: '%d'\n",linkaddr_node_addr.u8[0],linkaddr_node_addr.u8[1], from->u8[0], from->u8[1], msg_2_snd[0]);
    }
}

static void propagate(uint8_t config_tag){
    if(list_head(lukes_list) != NULL){
        struct node *i;
        for(i = list_head(lukes_list); i != NULL; i = list_item_next(i)){
            packetbuf_copyfrom(&config_tag,sizeof(uint8_t));
            unicast_send(&unicast, &i->addr);
        }
    }
}

/* Function called when receiving a unicast message */
static void recv_uc(struct unicast_conn *c, const linkaddr_t *from){

    uint8_t *ptr_data = packetbuf_dataptr();

    printf("unicast message received from %d.%d with this beautiful tag: %d\n",
            from->u8[0], from->u8[1],ptr_data[0]);

    struct node *inc_node_info = malloc(sizeof(struct node));

    switch(ptr_data[0]) {

        case TAG_VADOR :
            printf("Adding daddy to the list\n");
            linkaddr_copy(&inc_node_info->addr,from);
            inc_node_info->rank = ptr_data[1];
            list_add(vadors_list,inc_node_info);
            periodic_mode = ptr_data[4];
            has_subscribers = ptr_data[5];
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
            simple_tag = TAG_ACK_INFO;
            packetbuf_copyfrom(&simple_tag,sizeof(uint8_t));	
            unicast_send(&unicast, from);
            break;
        case TAG_ACK_INFO:
            /* Pong from parent */
            printf("Pong from the parent\n");
            has_tried_connecting = 0;
            break;
        case TAG_ROOT:
            /* Data to send to root */
            printf("Forwarding data to root\n");
            if(has_parent){
                packetbuf_copyfrom(ptr_data,packetbuf_datalen());
                unicast_send(&unicast, &parent.addr);
        case UPDATE_MODE:
            periodic_mode = 0;
            propagate(UPDATE_MODE);
            break;
        case PERIODIC_MODE:
            periodic_mode = 1;
            propagate(PERIODIC_MODE);
            break;
        case START_SEND:
            has_subscribers = 1;
            process_post(&battery_process, new_subscriber, NULL);
            process_post(&accel_process, new_subscriber, NULL);
            process_post(&temp_process, new_subscriber, NULL);
            propagate(START_SEND);
            break;
        case STOP_SEND:
            has_subscribers = 0;
            propagate(STOP_SEND);
            break;
    }

}

/*---------------------------------------------------------------------------*/

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

/*---------------------------------------------------------------------------*/

/* This broadcast process will be used to find a parent */

PROCESS_THREAD(broadcast_process, ev, data){
    static struct etimer et_broadcast;

    simple_tag = TAG_DISCOVERY; // Looking for parent

    PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

        PROCESS_BEGIN();

    broadcast_open(&broadcast, 129, &broadcast_call);

    while(1) {

        /* Delay */
        etimer_set(&et_broadcast, CLOCK_SECOND);

        /* Check list of potential parents and make choice */

        if(list_head(vadors_list) != NULL && !has_parent){
            printf("My current rank: %d\n before chosing parent", node_rank);
            uint8_t best_rank = 255;	
            struct node *i;
            for(i = list_head(vadors_list); i != NULL; i = list_item_next(i)){
                printf("current proposition rank: %d\n",i->rank);
                if(i->rank < best_rank && i->rank < node_rank){
                    best_rank = i->rank;
                    linkaddr_copy(&parent.addr,&i->addr);
                    parent.rank = best_rank;
                }
            }

            /* Free the memory and the list */	
            struct node *j;
            for(j = list_head(vadors_list); j != NULL; j = list_item_next(i)){
                free(list_pop(vadors_list));	
            }


            /* Define its own rank regarding the father */
            node_rank = best_rank+1;

            printf("My rank after daddy check: %d\n", node_rank);

            has_parent = 1;


            /* Send ACK to father */
            uint8_t msg_2_snd[4];

            msg_2_snd[0] = TAG_ACK_PARENT;
            msg_2_snd[1] = node_rank;
            msg_2_snd[2] = linkaddr_node_addr.u8[0];
            msg_2_snd[3] = linkaddr_node_addr.u8[1];
            packetbuf_copyfrom(msg_2_snd,sizeof(msg_2_snd));
            unicast_send(&unicast, &parent.addr);
            printf("Sending ACK to parent %d.%d\n",parent.addr.u8[0],parent.addr.u8[1]); 
        }



        if(has_parent == 0){
            printf("No parent or no connection\n");
            packetbuf_copyfrom(&simple_tag,sizeof(uint8_t));
            broadcast_send(&broadcast);
            printf("Looking for a father %d\n",simple_tag);
        }

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_broadcast));


    }

    PROCESS_END();
}

/*---------------------------------------------------------------------------*/

static const struct unicast_callbacks unicast_callbacks = {recv_uc};

/*---------------------------------------------------------------------------*/

/*This unicast process will be used to send the data to the parent*/

PROCESS_THREAD(unicast_process, ev, data){
    PROCESS_EXITHANDLER(unicast_close(&unicast);)

        static struct etimer et_unicast;
        PROCESS_BEGIN();

    unicast_open(&unicast, 146, &unicast_callbacks);

    while(1) {


        etimer_set(&et_unicast, 100*CLOCK_SECOND);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_unicast));

        if(has_parent && has_tried_connecting < 3){
            printf("TRYING TO PING DADDY\n");
            simple_tag = TAG_INFO;
            packetbuf_copyfrom(&simple_tag,sizeof(uint8_t));
            unicast_send(&unicast, &parent.addr);
            has_tried_connecting++;
        }else if(has_tried_connecting >= 3){
            /* New node situation */
            printf("RESET, PARENT LOST\n");
            has_tried_connecting = 0; 
            has_parent = 0;
            node_rank = 255;
        }		

    }
    PROCESS_END();
}

/* This process will read battery tension value and send it to the parent */

PROCESS_THREAD(battery_process, ev, data) {
    PROCESS_BEGIN();

    uint16_t battery;
    static uint16_t battery_last = 0;
    SENSORS_ACTIVATE(battery_sensor);

    while(1) {
        if(!has_subscribers)
            PROCESS_WAIT_EVENT_UNTIL(ev == new_subscriber);
        etimer_set(&et_battery, READ_INTERVAL);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_battery));

        battery = battery_sensor.value(0);

        if(!periodic_mode && battery == battery_last)
            continue;
        battery_last = battery;

        uint8_t msg[4];
        msg[0] = TAG_ROOT;
        msg[1] = TAG_BATTERY;
        msg[2] = (battery >> 8);
        msg[3] = (battery & 0xff);

        printf("%c %d\n", TAG_BATTERY, battery);
        packetbuf_copyfrom(msg,sizeof(msg));
        unicast_send(&unicast, &parent.addr);
        printf("Sending battery data to parent %d.%d\n",parent.addr.u8[0],parent.addr.u8[1]);
    }
    PROCESS_END();
}

/* This process will read the accelerometer value and send it to the parent */

PROCESS_THREAD(accel_process, ev, data) {
    PROCESS_BEGIN();

    int16_t x, y, z;
    static int16_t x_last = 0, y_last = 0, z_last = 0;
    accm_init();


    while(1) {
        uint8_t msg[8];
        if(!has_subscribers)
            PROCESS_WAIT_EVENT_UNTIL(ev == new_subscriber);
        etimer_set(&et_accel, READ_INTERVAL);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_accel));

        x = accm_read_axis(X_AXIS);
        y = accm_read_axis(Y_AXIS);
        z = accm_read_axis(Z_AXIS);

        if(!periodic_mode && x == x_last && y == y_last && z == z_last)
            continue;
        x_last = x;
        y_last = y;
        z_last = z;

        msg[0] = TAG_ROOT;
        msg[1] = TAG_ACCEL;
        msg[2] = x >> 8;
        msg[3] = x & 0xff;
        msg[4] = y >> 8;
        msg[5] = y & 0xff;
        msg[6] = z >> 8;
        msg[7] = z & 0xff;

        printf("%c %d %d %d\n", TAG_ACCEL, x, y, z);
        packetbuf_copyfrom(msg,sizeof(msg));
        unicast_send(&unicast, &parent.addr);
        printf("Sending accelerometer data to parent %d.%d\n",parent.addr.u8[0],parent.addr.u8[1]);
    }
    PROCESS_END();
}

/* This process will read the temperature sensor and send it to the parent */

PROCESS_THREAD(temp_process, ev, data) {
    PROCESS_BEGIN();

    int16_t temp;
    static int16_t temp_last = 0;

    tmp102_init();

    while(1) {
        uint8_t msg[4];
        if(!has_subscribers)
            PROCESS_WAIT_EVENT_UNTIL(ev == new_subscriber);
        etimer_set(&et_temp, READ_INTERVAL);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_temp));

        temp = tmp102_read_temp_raw();

        if(!periodic_mode && temp == temp_last)
            continue;
        temp_last = temp;

        msg[0] = TAG_ROOT;
        msg[1] = TAG_TEMP;
        msg[2] = temp >> 8;
        msg[3] = temp & 0xff;

        printf("%c %d\n", TAG_TEMP, temp);
        packetbuf_copyfrom(msg,sizeof(msg));
        unicast_send(&unicast, &parent.addr);
        printf("Sending temperature data to parent %d.%d\n",parent.addr.u8[0],parent.addr.u8[1]);
    }
    PROCESS_END();
}
