#include <stdio.h>
#include "contiki.h"
#include "lib/sensors.h"
#include "dev/serial-line.h"

#include "net/rime/rime.h"
#include "lib/list.h"
#include "random.h"

#define READ_INTERVAL (CLOCK_SECOND/10)

PROCESS(test_serial, "Serial receive process");
PROCESS(ask_config, "Startup config ask");

AUTOSTART_PROCESSES(&ask_config, &test_serial);

static struct etimer et_temp;
static struct etimer et_accel;
static struct etimer et_battery;

#define TAG_TEMP 'T'
#define TAG_BATTERY 'B'
#define TAG_ACCEL 'Z'

#define ASK_CONFIG 'A'
#define TEMP 'T'
#define ACCEL 'Z'
#define BATTERY 'B'
#define UPDATE_MODE 'U'
#define PERIODIC_MODE 'P'
#define STOP_SEND 'X'
#define START_SEND 'S'

/* TAG IDs */
#define TAG_INFO 1
#define TAG_ACK_INFO 2
#define TAG_DISCOVERY 3
#define TAG_VADOR 4
#define TAG_ACK_PARENT 5
#define TAG_ROOT 6

#define TAG_UPDATE_MODE 8
#define TAG_PERIODIC_MODE 9
#define TAG_START_SEND 10
#define TAG_STOP_SEND 11

static char periodic_mode = 1;
static char has_subscribers = 0;

static process_event_t new_subscriber;

/* This are the messages structures */
struct node{
    struct node *next; // needed for fathers and sons list
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

/*---------------------------------------------------------------------------*/

float floor(float x) {
    if(x >= 0.0f) {
        return (float)((int)x);
    } else {
        return (float)((int)x - 1);
    }
}

/*---------------------------------------------------------------------------*/

/* Function called when receiving a brodcast message*/
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from){

    struct simple_tag *msg_rcv;
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

/*---------------------------------------------------------------------------*/


/* Function called when receiving a unicast message */
static void recv_uc(struct unicast_conn *c, const linkaddr_t *from){

    uint8_t *ptr_data = packetbuf_dataptr();

    printf("unicast message received from %d.%d with this beautiful tag: %d\n",
            from->u8[0], from->u8[1],ptr_data[0]);

    struct node *inc_node_info = malloc(sizeof(struct node));
    struct simple_tag pong;

    uint16_t battery, temp, x, y, z;
    float mv;

    switch(ptr_data[0]) {
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
        case TAG_ROOT:
            switch(ptr_data[1]){
                case TAG_BATTERY:
                    battery = (ptr_data[2] << 8) | ptr_data[3];
                    mv = (battery * 2.500 * 2) / 4096;
                    printf("%c %ld.%03d", TAG_BATTERY, (long) mv,
                            (unsigned) ((mv - floor(mv)) * 1000));
                    break;
                case TAG_TEMP:
                    temp = (ptr_data[2] << 8) | ptr_data[3];
                    printf("%c %d", TAG_TEMP, temp);
                    break;
                case TAG_ACCEL:
                    x = (ptr_data[2] << 8) | ptr_data[3];
                    y = (ptr_data[4] << 8) | ptr_data[5];
                    z = (ptr_data[6] << 8) | ptr_data[7];
                    printf("%c %d %d %d\n", TAG_ACCEL, x, y, z);
                    break;
            }
    }
}


PROCESS_THREAD(ask_config, ev, data){
    PROCESS_BEGIN();
    printf("%c\n", ASK_CONFIG);
    PROCESS_END();
}

/*---------------------------------------------------------------------------*/

static void propagate(uint8_t config_tag){ 
    if(list_head(lukes_list) != NULL){
        struct node *i;
        for(i = list_head(lukes_list); i != NULL; i = list_item_next(i)){
            packetbuf_copyfrom(&config_tag,sizeof(uint8_t));
            unicast_send(&unicast, &i->addr);
        }
    }
}

/*---------------------------------------------------------------------------*/

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static const struct unicast_callbacks unicast_callbacks = {recv_uc};

PROCESS_THREAD(test_serial, ev, data)
{

    PROCESS_EXITHANDLER(unicast_close(&unicast);)
        PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

        PROCESS_BEGIN();

    unicast_open(&unicast, 146, &unicast_callbacks);
    broadcast_open(&broadcast, 129, &broadcast_call);

    for(;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
        if(ev == serial_line_event_message) {
            switch(((char *)data)[0]){
                case UPDATE_MODE:
                    periodic_mode = 0;
                    propagate(TAG_UPDATE_MODE);
                    break;
                case PERIODIC_MODE:
                    periodic_mode = 1;
                    propagate(TAG_PERIODIC_MODE);
                    break;
                case START_SEND:
                    has_subscribers = 1;
                    propagate(TAG_START_SEND);
                    break;
                case STOP_SEND:
                    has_subscribers = 0;
                    propagate(TAG_STOP_SEND);
                    break;
            }
        }
    }
    PROCESS_END();
}
