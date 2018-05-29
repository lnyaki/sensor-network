

/****************************************************************************
*                                 UNICAST
*****************************************************************************/
/*---------------------------------------------------------------------------*/
static void unicast_sent(struct unicast_conn *c, int status, int num_tx){
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d: status %d num_tx %d\n",
    dest->u8[0], dest->u8[1], status, num_tx);
}

/****************************************************************************
*                                 BROADCAST
*****************************************************************************/
static void broadcast_sent(struct broadcast_conn *c, int status, int num_tx){

}