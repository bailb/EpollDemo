#include<stdio.h>
#include"EpollObject.h"

int startup(char* _ip,int _port)  //创建一个套接字，绑定，检测服务器  
{  
  //sock  
  int sock=socket(AF_INET,SOCK_STREAM,0);     
  if(sock<0)  
  {  
      perror("sock");  
      exit(2);  
  }  
    
  int opt = 1;  
  setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));  
  no_block_fd(sock);
  struct sockaddr_in local;         
  local.sin_port=htons(_port);  
  local.sin_family=AF_INET;  
  local.sin_addr.s_addr=inet_addr(_ip);  
  
  if(bind(sock,(struct sockaddr*)&local,sizeof(local))<0)   
  {  
      perror("bind");  
      exit(3);  
  }  
  if(listen(sock,5)<0)  
  {  
      perror("listen");  
      exit(4);  
  }  
  return sock;    //这样的套接字返回  
} 

int main()
{
    int listen_fd = startup("0.0.0.0",5555);

    EpollObject *ep = new EpollObject();
    ep->init();
    struct epoll_event ev;
    ev.data.fd = listen_fd;
    ev.events = EPOLLET | EPOLLIN;
    spEpollConn  spConn = spEpollConn(new tcpListenConn());
    ep->addEv(&ev,spConn);
    ep->start_loop();
}
