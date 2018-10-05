#include <ev.h>
#include <stdio.h>
#include "ipd.h"


int scb(void *ud, const unsigned char *msg, int len, unsigned *rlen)
{
  printf("m(%d):%s\n",len,(const char *)msg);
  if(rlen!=0) *rlen=0;
  return(0);
}


int main(int argc, char **argv)
{
  struct ipd ipd;
  struct ev_loop *loop;
  unsigned char buf[100];
  unsigned char msg[]="uzenet!";
  
  loop=ev_default_loop(EVBACKEND_SELECT);

  ipd_send_request("hurka",(unsigned char *)msg,sizeof(msg),(unsigned char *)&buf,sizeof(buf));
  
  return(0);
}
