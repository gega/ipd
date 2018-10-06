#include <ev.h>
#include <stdio.h>
#include "../ipd.h"


int main(int argc, char **argv)
{
  char buf[100]={0};
  char msg[]="uzenet!";
  
  ipd_send_request("hurka",msg,buf,sizeof(buf));

  printf("r=%s\n",buf);
  
  return(0);
}
