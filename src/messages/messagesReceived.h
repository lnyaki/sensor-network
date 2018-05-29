

/****************************************************************************
*                                 UNICAST
*****************************************************************************/
static void
unicast_received(struct unicast_conn *c, const rimeaddr_t *sender)
{
  printf("unicast message received from %d.%d\n",
	 from->u8[0], from->u8[1]);
}

/****************************************************************************
*                                 BROADCAST
*****************************************************************************/
static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *sender)
{
  printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}