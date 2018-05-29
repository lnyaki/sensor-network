#include "contiki.h"
#include "net/rime/rime.h"
#include <stdio.h>


/*---------------------------------------------------------------------------*/
/*---------------------        Start Process    -----------------------------*/
/*---------------------------------------------------------------------------*/
PROCESS(sensor_code, "Sensor Communication Process");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/

/****************************************************************************
*                                 UNICAST
*****************************************************************************/
static void
unicast_received(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("unicast message received from %d.%d\n",
	 from->u8[0], from->u8[1]);
}
/*---------------------------------------------------------------------------*/
static void
unicast_sent(struct unicast_conn *c, int status, int num_tx)
{
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d: status %d num_tx %d\n",
    dest->u8[0], dest->u8[1], status, num_tx);
}
/*---------------------------------------------------------------------------*/

/****************************************************************************
*                                 BROADCAST
*****************************************************************************/
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}

/*---------------------------------------------------------------------------*/
/****************************************************************************
*                                 CALLBACKS
*****************************************************************************/
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct unicast_callbacks unicast_callbacks = {unicast_received, unicast_sent};
static struct unicast_conn uc;

static int BROADCAST_CHANNEL 	= 100;
static int UNICAST_CHANNEL 		= 101;
/****************************************************************************
*                               PROCESS THREAD
*****************************************************************************/
PROCESS_THREAD(example_broadcast_process, ev, data){
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
	PROCESS_BEGIN();

	//(<connection>, <channel>, <callbacks>)
	broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);
	unicast_open(&uc, UNICAST_CHANNEL, &unicast_callbacks);

	while(1){

	}

	PROCESS_END();
}