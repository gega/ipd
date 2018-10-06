#include <ev.h>
#include <stdio.h>
#include <ctype.h>
#include "../ipd.h"


int scb(void *ud, char *msg, int len, unsigned *rlen)
{
  printf("h(%d):%s\n",len,msg);
  if(rlen!=0)
  {
    for(int i=0;i<len;i++) msg[i]=toupper(msg[i]);
    *rlen=len;
  }
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
