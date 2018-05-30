#include "contiki.h"
#include "net/rime.h"
#include <stdio.h>
/****************************************************************************
*                                 CALLBACKS
*****************************************************************************/
extern int unicast_sent(struct unicast_conn *connection, int status, int num_tx);

extern int broadcast_sent(struct broadcast_conn *connection, int status, int num_tx);


/****************************************************************************
*                                BROADCAST FUNCTIONS
*****************************************************************************/
extern int send_broadcast_message(struct broadcast_conn *broadcast_connection, char message);

extern int send_unicast_message();