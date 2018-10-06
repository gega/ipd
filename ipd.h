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
  int (*cb)(void *,char *,int,unsigned *);
  char app[IPD_MAX_APP_NAME_LEN+1];
};


static inline void ipd_process_command_cb(struct ev_loop* loop, struct ev_io *io, int r)
{
  struct ipd *ipd=(struct ipd *)io;
  struct sockaddr_un ca={0};
  char buf[IPD_BUFSIZ];
  int n;
  socklen_t cl=sizeof(struct sockaddr_un);
  unsigned rs=0;
  
  ca.sun_family=AF_UNIX;
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

static inline int ipd_reg(struct ipd *ipd, const char *app, struct ev_loop *loop, int (*cb)(void *,char *,int, unsigned *), void *ud)
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
    if(-1!=(ipd->fd=socket(AF_UNIX,SOCK_DGRAM,0)))
    {
      unlink(nam);
      sa.sun_family=AF_UNIX;
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

static inline int ipd_pub(const char *msg)
{
  int ret=-1;
  int fd;
  const int bc=1;
  struct sockaddr_in ad={0};
  
  if(NULL!=msg)
  {
    fd=socket(AF_INET,SOCK_DGRAM,0);
    setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&bc,sizeof(bc));
    ad.sin_family=AF_INET;
    ad.sin_port=(in_port_t)htons(IPD_PORT);
    ad.sin_addr.s_addr=inet_addr("127.255.255.255");
    ret=sendto(fd,msg,strlen(msg)+1,0,(struct sockaddr *)&ad,sizeof(ad));
  }
  
  return(ret);
}

static inline void ipd_sub_cb(struct ev_loop* loop, struct ev_io *io, int r)
{
  struct ipd *ipd=(struct ipd *)io;
  char buf[IPD_BUFSIZ];
  int n;
  
  if(-1!=(n=recvfrom(ipd->fd,buf,sizeof(buf),0,0,0))&&0!=ipd->cb)
  {
    ipd->cb(ipd->ud,buf,n,0);
  }
}

static inline int ipd_sub(struct ipd *ipd, struct ev_loop *loop, int (*cb)(void *,char *,int, unsigned *), void *ud)
{
  int ret=-1;
  const int enable=1;
  struct sockaddr_in ad={0};

  if(0!=ipd&&0!=loop&&0!=cb)
  {
    ipd->fd=socket(PF_INET,SOCK_DGRAM,0);
    setsockopt(ipd->fd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(int));
    ad.sin_family=AF_INET;
    ad.sin_port=htons(IPD_PORT);
    ad.sin_addr.s_addr=INADDR_ANY;
    bind(ipd->fd,(struct sockaddr*)&ad,sizeof(ad));
    ipd->cb=cb;
    ipd->ud=ud;
    ev_io_init(&ipd->io,ipd_sub_cb,ipd->fd,EV_READ);
    ev_io_start(loop,&ipd->io);
    ret=0;
  }

  return(ret);
}

#define ipd_send_command(app,cmd) ipd_send_request(app,cmd,NULL,0)

static inline int ipd_send_request(const char *app, const char *req, char *reply, unsigned buflen)
{
  int ret=-1,fd;
  struct sockaddr_un ca={0},sa={0};
  char dir[]=IPD_DIR "/ipd_csr.XXXXXX";
  char buf;
  
  if(0!=app&&0!=req)
  {
    fd=socket(AF_UNIX,SOCK_DGRAM,0);
    ca.sun_family=sa.sun_family=AF_UNIX;
    mkdtemp(dir);
    snprintf(ca.sun_path,sizeof(ca.sun_path),"%s/cs",dir);
    bind(fd,(struct sockaddr *)&ca,sizeof(ca));
    snprintf(sa.sun_path,sizeof(sa.sun_path),"%s/%s",IPD_DIR,app);
    sendto(fd,req,strlen(req)+1,0,(struct sockaddr *)&sa,sizeof(sa));
    ret=recvfrom(fd,(reply==0?&buf:reply),buflen,0,0,0);
    unlink(ca.sun_path);
    rmdir(dir);
  }

  return(ret);
}
