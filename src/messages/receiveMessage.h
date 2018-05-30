#include "contiki.h"
#include "net/rime/rime.h"
#include <stdio.h>

/****************************************************************************
*                                 UNICAST
*****************************************************************************/
extern void unicast_received(struct unicast_conn *c, const linkaddr_t *sender);

/****************************************************************************
*                                 BROADCAST
*****************************************************************************/
extern void broadcast_received(struct broadcast_conn *c, const linkaddr_t *sender);