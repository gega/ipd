#include <ev.h>
#include <stdio.h>
#include <ctype.h>
#include <libgen.h>
#include "../ipd.h"


static char *app;
static ev_timer tim;
static struct ev_loop *loop;


int scb(void *ud, char *msg, int len, char **rpl)
{
  static char r[100];

  printf("%s(%d):%s\n",app,len,msg);
  if(rpl!=0)
  {
    for(int i=0;i<len;i++) r[i]=toupper(msg[i]);
    *rpl=r;
  }
  return(0);
}

int pcb(void *ud, char *msg, int len)
{
  printf("%s bus: (%d) '%s'\n",app,len,msg);
  return(0);
}

static void tim_cb(EV_P_ ev_timer *w, int revents)
{
  static int cnt=0;
  char msg[200];
  char rpl[100]={0};
  
  snprintf(msg,sizeof(msg),"[%s]:%d.",app,++cnt);
  ipd_pub(msg);
  if((cnt%5)==0) 
  {
    ipd_send_request("tb.c","ta-hurka",rpl,sizeof(rpl));
    printf("[%s] tb>rpl=%s\n",app,rpl);
  }
  if((cnt%7)==0) 
  {
    ipd_send_request("tc.c","ta-hurka",rpl,sizeof(rpl));
    printf("[%s] tc>rpl=%s\n",app,rpl);
  }
  ev_timer_again(loop,w);
}

int main(int argc, char **argv)
{
  struct ipd ipd;
  struct ipd sub;
  app=strdup(basename(__FILE__));
  
  loop=ev_default_loop(EVBACKEND_SELECT);
  ev_init(&tim,tim_cb);
  tim.repeat=0.51d;
  ipd_reg(&ipd,app,loop,scb,0);
  ipd_sub(&sub,loop,pcb,0);
  printf("%s running...\n",app);
  ev_timer_again(loop,&tim);
  ev_run(loop,0);
  free(app);

  return(0);
}
