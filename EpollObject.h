#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<errno.h>  
#include<sys/types.h>  
#include<sys/epoll.h>  
#include<sys/socket.h>  
#include<arpa/inet.h>  
#include<netinet/in.h>  
#include<assert.h>  
#include<fcntl.h>  
#include<unistd.h>
#include<memory>
#include<mutex>
#include<map>
int no_block_fd(int fd);
class iEpollConn {
public:
    iEpollConn(){};
    virtual ~iEpollConn(){};
public:
    virtual void doAction(void*arg, struct epoll_event *ev) = 0;
};
typedef std::shared_ptr<iEpollConn> spEpollConn;

class EpollObject {
public:
    EpollObject();
    ~EpollObject();

    int init();
    int addEv(struct epoll_event *ev,spEpollConn);
    int delEv(struct epoll_event *ev);
    int start_loop();
private:
    int m_epoll_fd;
    int m_monitor_size;
    int m_epoll_timeout;

    bool m_is_done;

    std::map<int,spEpollConn> m_conn_map;
    std::recursive_mutex m_conn_map_mutex;
};


class tcpConn : public iEpollConn  {
public:
    virtual ~tcpConn(){};
public:
    virtual void doAction(void*arg, struct epoll_event *ev);
};

class tcpListenConn : public iEpollConn {
public:
    virtual ~tcpListenConn(){};
public:
    virtual void doAction(void*arg, struct epoll_event *ev);
};

class udpConn : public iEpollConn {
public:
    virtual ~udpConn(){};
public:
    virtual void doAction(void*arg, struct epoll_event *ev);

};