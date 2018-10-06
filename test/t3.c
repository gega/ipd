#include <stdio.h>
#include "../ipd.h"


int main(int argc, char **argv)
{
  char msg[]="t3-uzenet!";
  
  int r=ipd_pub(msg);

  printf("r=%d\n",r);
  
  return(0);
}
