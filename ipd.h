#include <stdio.h>
#include <stdlib.h>
#include <ev.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <stdarg.h>
#include <sys/un.h>


#define IPD_DIR "/tmp/ipd"
#define IPD_APP_DIR IPD_DIR "/app"
#define IPD_PORT_DIR IPD_DIR "/port"
#define IPD_TMP_DIR IPD_DIR "/tmp"
#define IPD_MAX_APP_NAME_LEN 31
#define IPD_PORT_MIN 8700
#define IPD_PORT_MAX 8800
#define IPD_PORT_LEN 6
#define IPD_PORT_ALLOC_RETRY 40


typedef struct
{
  struct ev_io io;
  int fd;
  int port;
  struct ev_loop *loop;
  void *ud;
  int (*cb)(void *,const char *);
  char app[IPD_MAX_APP_NAME_LEN+1];
} ipd_t;


static inline void raccept_cb(struct ev_loop* loop, struct ev_io *io, int r)
{
  ipd_t *ipd=(ipd_t *)io;
  char *buf;
  struct sockaddr_in client_address;

  socklen_t client_address_len = sizeof(client_address);
  int client_fd = accept(ipd->fd, (struct sockaddr*)(&client_address), &client_address_len);

  buf=malloc(1000); // FIXME: realloc to dynamic buffer
  if(-1!=read(client_fd,buf,1000))
  {
    ipd->cb(ipd->ud,buf);
    free(buf);
  }
  close(client_fd);
}

static inline void ipd_unreg(ipd_t *ipd)
{
  char nam[sizeof(IPD_APP_DIR)+1+IPD_MAX_APP_NAME_LEN+1];
  char pnm[sizeof(IPD_PORT_DIR)+1+IPD_PORT_LEN+1];

  if(NULL!=ipd)
  {
    ev_io_stop(ipd->loop,&ipd->io);
    shutdown(ipd->fd,SHUT_RDWR);
    close(ipd->fd);
    snprintf(nam,sizeof(nam),"%s/%s",IPD_APP_DIR,ipd->app);
    snprintf(pnm,sizeof(pnm),"%s/%d",IPD_PORT_DIR,ipd->port);
    unlink(nam);
    unlink(pnm);
  }
}

static inline int ipd_reg(ipd_t *ipd, const char *app, struct ev_loop *loop, int (*cb)(void *,const char *), void *ud)
{
  int ret=-1;
  char nam[sizeof(IPD_APP_DIR)+1+IPD_MAX_APP_NAME_LEN+1];
  char tmp[sizeof(IPD_TMP_DIR)+1+IPD_MAX_APP_NAME_LEN+1];
  char pnm[sizeof(IPD_PORT_DIR)+1+IPD_PORT_LEN+1];
  FILE *f;
  
  if(0==mkdir(IPD_DIR,0777))
  {
    mkdir(IPD_APP_DIR,0777);
    mkdir(IPD_PORT_DIR,0777);
    mkdir(IPD_TMP_DIR,0777);
  }
  if(NULL!=ipd&&NULL!=app&&NULL!=loop)
  {
    ipd->port=0;
    ipd->fd=-1;
    ipd->loop=loop;
    ipd->cb=cb;
    ipd->ud=ud;
    strncpy(ipd->app,app,sizeof(ipd->app));
    snprintf(nam,sizeof(nam),"%s/%s",IPD_APP_DIR,app);
    snprintf(tmp,sizeof(tmp),"%s/%s",IPD_TMP_DIR,app);
    if(NULL!=(f=fopen(tmp,"w+b")))
    {
      int p,cnt=IPD_PORT_ALLOC_RETRY;
      struct sockaddr_un ad;
      srand(getpid());
      do
      {
        p=(rand()%(IPD_PORT_MAX-IPD_PORT_MIN))+IPD_PORT_MIN;
        snprintf(pnm,sizeof(pnm),"%s/%d",IPD_PORT_DIR,p);
        ad.sun_family=AF_UNIX;
        strcpy(ad.sun_path,pnm);
        if(-1!=(ipd->fd=socket(AF_UNIX,SOCK_STREAM,0)))
        {
          if(0==bind(ipd->fd,(struct sockaddr*)(&ad),sizeof(ad)))
          {
            listen(ipd->fd,10);
            ret=ipd->port=p;
          }
          else close(ipd->fd);
        }
      } while(ipd->port==0&&--cnt>0);
      fprintf(f,"%d\n",ipd->port);
      fclose(f);
      if(ipd->port==0||rename(tmp,app)!=0)
      {
        unlink(tmp);
        unlink(pnm);
      }
      else
      {
        ev_io_init(&ipd->io,raccept_cb,ipd->fd,EV_READ);
        ev_io_start(ipd->loop,&ipd->io);
      }
    }
  }
  
  return(ret);
}

static inline int ipd_pub(const char *msg)
{
  // send 'msg' as udp broadcast to 127.255.255.255:PORT
  int ret=-1;
  
  if(NULL!=msg)
  {
  }
  
  return(ret);
}

static inline int ipd_sub(struct ev_loop *loop, int (*cb)(void *,const char *), void *ud)
{
  // udp server on PORT => cb
  return(0);
}

static inline void ipd_send_command(const char *app, const char *cmd)
{
  // connect to unix socket IPD_DIR/'app' and write 'cmd', close socket
  if(NULL!=app&&NULL!=cmd)
  {
  }
}

static inline char *ipd_send_request(const char *app, const char *req)
{
  // connect to unix socket IPD_DIR/'app' and write 'cmd', return reply
  char *ret=NULL;
  
  if(NULL!=app&&NULL!=req)
  {
  }

  return(ret);
}
