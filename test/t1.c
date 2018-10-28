#include <ev.h>
#include <stdio.h>
#include <ctype.h>
#include "../ipd.h"


int scb(void *ud, const char *msg, int len, char **rply)
{
  char rbuf[100];

  printf("h(%d):%s\n",len,msg);
  if(rply!=0)
  {
    if(len<sizeof(rbuf)) for(int i=0;i<len;i++) rbuf[i]=toupper(msg[i]);
    else snprintf(rbuf,sizeof(rbuf),"*ERROR: message too long");
    *rply=rbuf;
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
