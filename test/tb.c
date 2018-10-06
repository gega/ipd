#include <ev.h>
#include <stdio.h>
#include <ctype.h>
#include <libgen.h>
#include "../ipd.h"


static char *app;
static ev_timer tim;
static struct ev_loop *loop;


int scb(void *ud, char *msg, int len, unsigned *rlen)
{
  printf("%s(%d):%s\n",app,len,msg);
  if(rlen!=0)
  {
    for(int i=0;i<len;i++) msg[i]=toupper(msg[i]);
    *rlen=len;
  }
  return(0);
}

int pcb(void *ud, char *msg, int len, unsigned *unused)
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
  if((cnt%11)==0) 
  {
    ipd_send_request("ta.c","tb-hurka",rpl,sizeof(rpl));
    printf("[%s] ta>rpl=%s\n",app,rpl);
  }
  if((cnt%17)==0) 
  {
    ipd_send_request("tc.c","tb-hurka",rpl,sizeof(rpl));
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
  tim.repeat=0.56d;
  ipd_reg(&ipd,app,loop,scb,0);
  ipd_sub(&sub,loop,pcb,0);
  printf("%s running...\n",app);
  ev_timer_again(loop,&tim);
  ev_run(loop,0);
  free(app);

  return(0);
}
