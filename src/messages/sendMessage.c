/****************************************************************************
*                                 CALLBACKS
*****************************************************************************/
int unicast_sent(struct unicast_conn *c, int status, int num_tx){
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d: status %d num_tx %d\n",
    dest->u8[0], dest->u8[1], status, num_tx);
}

int broadcast_sent(struct broadcast_conn *c, int status, int num_tx){

}


/****************************************************************************
*                                BROADCAST FUNCTIONS
*****************************************************************************/
int send_broadcast_message(struct broadcast_conn broadcast_connection,char * message){
	packetbuf_copyfrom(message, strlen(message));
    broadcast_send(&broadcast_connection);
    printf("broadcast message sent\n");
}

int send_unicast_message(){

}

int sendHelloMessage(){}

int sendDiscoveryMessage(){}

int sendNodeInformationMessage(){}

int sendDataMessage(){}

int sendACK(){

}