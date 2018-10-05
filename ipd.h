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
#define IPD_MAX_APP_NAME_LEN 31
#define IPD_PORT 12193
#define IPD_BUFSIZ 2048


struct ipd
{
  struct ev_io io;
  int fd;
  struct ev_loop *loop;
  void *ud;
  int (*cb)(void *,const unsigned char *,int,unsigned *);
  char app[IPD_MAX_APP_NAME_LEN+1];
};


static inline void ipd_process_command_cb(struct ev_loop* loop, struct ev_io *io, int r)
{
  struct ipd *ipd=(struct ipd *)io;
  struct sockaddr_un ca={0};
  unsigned char buf[IPD_BUFSIZ];
  int n;
  socklen_t cl=sizeof(struct sockaddr_un);
  unsigned rs=0;
  
  ca.sun_family=PF_UNIX;
  if(-1!=(n=recvfrom(ipd->fd,buf,sizeof(buf),0,(struct sockaddr *)&ca,&cl))&&0!=ipd->cb)
  {
    ipd->cb(ipd->ud,buf,n,&rs);
    if(rs==0) buf[rs++]=0;
    sendto(ipd->fd,buf,rs,0,(struct sockaddr *)&ca,cl);
  }
}


static inline void ipd_unreg(struct ipd *ipd)
{
  char nam[sizeof(IPD_DIR)+1+IPD_MAX_APP_NAME_LEN+1];

  if(NULL!=ipd)
  {
    ev_io_stop(ipd->loop,&ipd->io);
    close(ipd->fd);
    snprintf(nam,sizeof(nam),"%s/%s",IPD_DIR,ipd->app);
    unlink(nam);
  }
}

static inline int ipd_reg(struct ipd *ipd, const char *app, struct ev_loop *loop, int (*cb)(void *,const unsigned char *,int, unsigned *), void *ud)
{
  int ret=-1;
  char nam[sizeof(IPD_DIR)+1+IPD_MAX_APP_NAME_LEN+1];
  struct sockaddr_un sa={0};
  
  mkdir(IPD_DIR,0777);
  if(NULL!=ipd&&NULL!=app&&NULL!=loop)
  {
    ipd->fd=-1;
    ipd->loop=loop;
    ipd->cb=cb;
    ipd->ud=ud;
    strncpy(ipd->app,app,sizeof(ipd->app));
    snprintf(nam,sizeof(nam),"%s/%s",IPD_DIR,app);
    if(-1!=(ipd->fd=socket(PF_UNIX,SOCK_DGRAM,0)))
    {
      unlink(nam);
      sa.sun_family=PF_UNIX;
      strncpy(sa.sun_path,nam,sizeof(sa.sun_path));
      if(-1!=(bind(ipd->fd,(struct sockaddr *)&sa,sizeof(sa))))
      {
        ev_io_init(&ipd->io,ipd_process_command_cb,ipd->fd,EV_READ);
        ev_io_start(ipd->loop,&ipd->io);
        ret=0;
      }
    }
  }
    
  return(ret);
}

static inline int ipd_pub(const unsigned char *msg, unsigned len)
{
  // send 'msg' as udp broadcast to 127.255.255.255:IPD_PORT
  int ret=-1;
  
  if(NULL!=msg&&len>0)
  {
  }
  
  return(ret);
}

static inline int ipd_sub(struct ev_loop *loop, int (*cb)(void *,const unsigned char *, unsigned *), void *ud)
{
  // normal udp server on IPD_PORT => cb(ud,buf,buflen), cb returns -1 => close
  return(0);
}

static inline void ipd_send_command(const char *app, const unsigned char *cmd, unsigned len)
{
  // connect to unix socket IPD_DIR/'app' and write 'cmd', close socket
  struct sockaddr_un sa={0},ca={0};
  unsigned char buf[IPD_BUFSIZ];
  int fd;

  if(0!=app&&0!=cmd)
  {
    fd=socket(AF_UNIX,SOCK_DGRAM,0);
    ca.sun_family=PF_UNIX;
    char *dir=mkdtemp("ipd_cs.XXXXXX");
    snprintf(ca.sun_path,sizeof(ca.sun_path),"%s/cs",dir);
    bind(fd,(struct sockaddr *)&ca,sizeof(ca));
    snprintf(sa.sun_path,sizeof(sa.sun_path),"%s/%s",IPD_DIR,app);
    sendto(fd,cmd,len,0,(struct sockaddr *)&sa,sizeof(sa));
    recvfrom(fd,buf,sizeof(buf),0,0,0);
    unlink(ca.sun_path);
    rmdir(dir);
  }
}

static inline int ipd_send_request(const char *app, const unsigned char *req, unsigned len,unsigned char *reply, unsigned buflen)
{
  // connect to unix socket IPD_DIR/'app' and write 'cmd', return reply
  int ret=-1,fd;
  struct sockaddr_un ca={0},sa={0};
  
  if(NULL!=app&&NULL!=req&&len>0&&reply!=0&&buflen>0)
  {
    fd=socket(AF_UNIX,SOCK_DGRAM,0);
    ca.sun_family=PF_UNIX;
    char *dir=mkdtemp("ipd_cs.XXXXXX");
    snprintf(ca.sun_path,sizeof(ca.sun_path),"%s/cs",dir);
    bind(fd,(struct sockaddr *)&ca,sizeof(ca));
    snprintf(sa.sun_path,sizeof(sa.sun_path),"%s/%s",IPD_DIR,app);
    sendto(fd,req,len,0,(struct sockaddr *)&sa,sizeof(sa));
    ret=recvfrom(fd,reply,buflen,0,0,0);
    unlink(ca.sun_path);
    rmdir(dir);
  }

  return(ret);
}
