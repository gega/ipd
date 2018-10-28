#include <ev.h>
#include <stdio.h>
#include <ctype.h>
#include "../ipd.h"


int scb(void *ud, const char *msg, int len)
{
  printf("h(%d):%s\n",len,msg);
  return(0);
}


int main(int argc, char **argv)
{
  struct ipd ipd;
  struct ev_loop *loop;
  
  loop=ev_default_loop(EVBACKEND_SELECT);
  ipd_sub(&ipd, loop, scb, 0);
  ev_run(loop,0);

  return(0);
}
