#include "contiki.h"
#include "net/rime.h"
#include <stdio.h>

/****************************************************************************
*                                 UNICAST
*****************************************************************************/
extern void unicast_received(struct unicast_conn *c, const rimeaddr_t *sender);

/****************************************************************************
*                                 BROADCAST
*****************************************************************************/
extern void broadcast_received(struct broadcast_conn *c, const rimeaddr_t *sender);