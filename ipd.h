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
#define IPD_MAXPUB 8192


struct ipd
{
  struct ev_io io;
  int fd;
  struct ev_loop *loop;
  void *ud;
  union
  {
    int (*rcb)(void *,const char *,int,char **);
    int (*bcb)(void *,const char *,int);
  } u;
  char *rxbuf;
  int rxblen;
  char app[IPD_MAX_APP_NAME_LEN+1];
};


/*
 *  register application
 *
 *    ipd  - uninitialized ipd struct
 *    app  - application name, must be unique system wide
 *    loop - libev loop
 *    cb   - callback for requests
 *           int cb(void *ud, const char *req, int len, char **reply)
 *             ud       - userdata
 *             req      - request as c string
 *             len      - length of request including terminator
 *             reply    - reply
 *    ud   - userdata, passed to cb
 */
static inline int ipd_reg(struct ipd *ipd, const char *app, struct ev_loop *loop, int (*cb)(void *,const char *,int,char **), void *ud);

/*
 *  unregister application
 *
 *    ipd - ipd struct filled by ipd_reg()
 */
static inline void ipd_unreg(struct ipd *ipd);

/*
 *  publish message on public bus
 *
 *    msg - message as c string
 */
static inline int ipd_pub(const char *msg);

/*
 *  subscribe to public message bus
 *
 *    ipd  - uninitialized ipd struct for the subscription
 *    loop - libev loop
 *    cb   - callback for messages on the bus
 *           int cb(void *ud, char *req, int len, unsigned *unused)
 *             ud       - userdata
 *             req      - request as c string
 *             len      - length of request including terminator
 *    ud   - userdata passed to cb
 */
static inline int ipd_sub(struct ipd *ipd, struct ev_loop *loop, int (*cb)(void *,const char *,int), void *ud);

/*
 *  send an rpc request for an application
 *
 *    app    - name of destination application
 *    req    - request as c string
 *    reply  - buffer for reply or 0 if reply is not important
 *    buflen - length of reply buffer or 0 if reply should be dropped
 */
static inline int ipd_send_request(const char *app, const char *req, char *reply, unsigned buflen);


static inline void ipd_process_command_cb(struct ev_loop* loop, struct ev_io *io, int r)
{
  struct ipd *ipd=(struct ipd *)io;
  struct sockaddr_un ca={0};
  char buf[1];
  int n;
  socklen_t cl=sizeof(struct sockaddr_un);
  char *rpl=0;
  
  ca.sun_family=AF_UNIX;
  if(-1!=(n=recvfrom(ipd->fd,buf,sizeof(buf),MSG_TRUNC|MSG_PEEK,(struct sockaddr *)&ca,&cl))&&0!=ipd->u.rcb)
  {
    if(ipd->rxblen<n) ipd->rxbuf=realloc(ipd->rxbuf,ipd->rxblen=n);
    if(-1!=recvfrom(ipd->fd,ipd->rxbuf,n,0,0,0))
    {
      ipd->u.rcb(ipd->ud,ipd->rxbuf,n,&rpl);
      if(0!=rpl) sendto(ipd->fd,rpl,strlen(rpl)+1,0,(struct sockaddr *)&ca,cl);
    }
  }
}


static inline void ipd_unreg(struct ipd *ipd)
{
  char nam[sizeof(IPD_DIR)+1+IPD_MAX_APP_NAME_LEN+1];

  if(0!=ipd)
  {
    ev_io_stop(ipd->loop,&ipd->io);
    close(ipd->fd);
    if(0!=ipd->rxbuf) free(ipd->rxbuf);
    if(0!=ipd->app[0])
    {
      snprintf(nam,sizeof(nam),"%s/%s",IPD_DIR,ipd->app);
      unlink(nam);
    }
    bzero(ipd,sizeof(struct ipd));
  }
}

static inline int ipd_reg(struct ipd *ipd, const char *app, struct ev_loop *loop, int (*cb)(void *,const char *,int,char **), void *ud)
{
  int ret=-1;
  char nam[sizeof(IPD_DIR)+1+IPD_MAX_APP_NAME_LEN+1];
  struct sockaddr_un sa={0};
  
  mkdir(IPD_DIR,0777);
  if(0!=ipd&&0!=app&&0!=loop)
  {
    ipd->fd=-1;
    ipd->loop=loop;
    ipd->u.rcb=cb;
    ipd->ud=ud;
    ipd->rxbuf=0;
    ipd->rxblen=0;
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
  int ret=-1,l;
  int fd;
  const int bc=1;
  struct sockaddr_in ad={0};
  
  if(0!=msg&&(l=strlen(msg))<IPD_MAXPUB)
  {
    if(-1!=(fd=socket(AF_INET,SOCK_DGRAM,0)))
    {
      if(-1!=setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&bc,sizeof(bc)))
      {
        ad.sin_family=AF_INET;
        ad.sin_port=(in_port_t)htons(IPD_PORT);
        ad.sin_addr.s_addr=inet_addr("127.255.255.255");
        ret=sendto(fd,msg,l+1,0,(struct sockaddr *)&ad,sizeof(ad));
      }
    }
  }
  
  return(ret);
}

static inline void ipd_sub_cb(struct ev_loop* loop, struct ev_io *io, int r)
{
  struct ipd *ipd=(struct ipd *)io;
  char buf[IPD_MAXPUB];
  int n;
  
  if(-1!=(n=recvfrom(ipd->fd,buf,sizeof(buf),0,0,0))&&0!=ipd->u.bcb) ipd->u.bcb(ipd->ud,buf,n);
}

#define ipd_unsub(ipd) ipd_unreg(ipd)

static inline int ipd_sub(struct ipd *ipd, struct ev_loop *loop, int (*cb)(void *,const char *,int), void *ud)
{
  int ret=-1;
  const int enable=1;
  struct sockaddr_in ad={0};

  if(0!=ipd&&0!=loop&&0!=cb)
  {
    ipd->loop=loop;
    ipd->rxbuf=0;
    ipd->rxblen=0;
    ipd->app[0]=0;
    if(-1!=(ipd->fd=socket(PF_INET,SOCK_DGRAM,0)))
    {
      setsockopt(ipd->fd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(int));
      setsockopt(ipd->fd,SOL_SOCKET,SO_REUSEPORT,&enable,sizeof(int));
      ad.sin_family=AF_INET;
      ad.sin_port=htons(IPD_PORT);
      ad.sin_addr.s_addr=INADDR_ANY;
      if(-1!=bind(ipd->fd,(struct sockaddr*)&ad,sizeof(ad)))
      {
        ipd->u.bcb=cb;
        ipd->ud=ud;
        ev_io_init(&ipd->io,ipd_sub_cb,ipd->fd,EV_READ);
        ev_io_start(loop,&ipd->io);
        ret=0;
      }
    }
  }

  return(ret);
}

#define ipd_send_command(app,cmd) ipd_send_request(app,cmd,0,0)

static inline int ipd_send_request(const char *app, const char *req, char *reply, unsigned buflen)
{
  int ret=-1,fd;
  struct sockaddr_un ca={0},sa={0};
  char dir[]=IPD_DIR "/ipd_csr.XXXXXX";
  char buf;
  
  if(0!=app&&0!=req)
  {
    if(-1!=(fd=socket(AF_UNIX,SOCK_DGRAM,0))&&0!=mkdtemp(dir))
    {
      ca.sun_family=sa.sun_family=AF_UNIX;
      snprintf(ca.sun_path,sizeof(ca.sun_path),"%s/cs",dir);
      if(-1!=bind(fd,(struct sockaddr *)&ca,sizeof(ca)))
      {
        snprintf(sa.sun_path,sizeof(sa.sun_path),"%s/%s",IPD_DIR,app);
        if(0<sendto(fd,req,strlen(req)+1,0,(struct sockaddr *)&sa,sizeof(sa))) ret=recvfrom(fd,(reply?reply:&buf),buflen,0,0,0);
        else if(reply) *reply=0;
        unlink(ca.sun_path);
      }
      rmdir(dir);
    }
  }

  return(ret);
}
