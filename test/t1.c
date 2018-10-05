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
  
  loop=ev_default_loop(EVBACKEND_SELECT);
  ipd_reg(&ipd,"hurka", loop, scb, 0);
  ev_run(loop,0);

  return(0);
}
