#include"EpollObject.h"

EpollObject::EpollObject():m_epoll_fd(-1)
,m_monitor_size(256)
,m_epoll_timeout(-1)
,m_is_done(false)
{


}
int EpollObject::init()
{
    m_epoll_fd = epoll_create(m_monitor_size);    //可处理的最大句柄数256个
    if (m_epoll_fd <= 0)
    {
        perror("epoll_create error");
        return 0;
    } 
}
EpollObject::~EpollObject()
{

}
int no_block_fd(int fd)
{
    int ret = -1;
    do {
        int flag;
        if (flag = fcntl(fd, F_GETFL, 0) <0) 
            perror("get flag");
        flag |= O_NONBLOCK;
        if (fcntl(fd, F_SETFL, flag) < 0)
            perror("set flag");
    } while(0);
    return ret;
}

int EpollObject::addEv(struct epoll_event *ev,spEpollConn conn)
{
    int care_fd = ev->data.fd;
    {
        std::lock_guard<std::recursive_mutex> lock(m_conn_map_mutex);
        auto iter = m_conn_map.find(care_fd);
        if (iter!= m_conn_map.end())
        {
            printf("has been exist \n");
            return -1;
        }
        m_conn_map[care_fd] = conn;
    }
    int ret = epoll_ctl(m_epoll_fd,EPOLL_CTL_ADD,care_fd,ev);
    if  (ret != 0)
    {
        perror("epoll_ctl error");
    }
    return ret;
}

int EpollObject::delEv(struct epoll_event *ev)
{
    int care_fd = ev->data.fd;
    {
        std::lock_guard<std::recursive_mutex> lock(m_conn_map_mutex);
        auto iter = m_conn_map.find(care_fd);
        if (iter != m_conn_map.end())
        {
            printf("has been exist \n");
            m_conn_map.erase(care_fd);
            return -1;
        }
    }
    int ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, care_fd, ev); 
    if  (ret != 0)
    {
        perror("epoll_ctl error");
    }
    return ret;
}

int EpollObject::start_loop()
{
    int ready_num = 0;
    struct epoll_event revs[64];    
    while(!m_is_done)
    {
        switch((ready_num = epoll_wait(m_epoll_fd,revs,64,m_epoll_timeout)))  //返回需要处理的事件数目  64表
        {
            case 0:
                printf("timeout\n");  
                break;  
            case -1:
                perror("epoll_wait error");
                break;
            default:
            {
                std::lock_guard<std::recursive_mutex> lock(m_conn_map_mutex);
                for(int i=0;i < ready_num;i++)
                {
                    auto iter = m_conn_map.find(revs[i].data.fd);
                    if (iter != m_conn_map.end())
                    {
                        iter->second->doAction(this,&revs[i]);
                    }
                    else
                    {
                        //don't exist remove
                    }
                }
                break;
            }

        }

    }
}

void tcpConn::doAction(void*arg, struct epoll_event *ev)
{
    if (arg == nullptr || ev == nullptr)
    {
        printf("doAction error");
        return;
    }

    EpollObject *ser = (EpollObject*)arg;
    if (ev->events & EPOLLIN)
    {
        char buf[1024]= {0};
        while(1)
        {
            int ret = recv(ev->data.fd,buf,1023,0);
            if (ret < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                else
                {
                    ser->delEv(ev);
                    close(ev->data.fd);
                    break;
                }
            }
            else if (ret == 0)
            {
                //remote peer close the connection
                break;
                ser->delEv(ev);
                close(ev->data.fd);
            }
            else
            {
                printf("recv[%d] [%s]\n",ret,buf);
            }
        }

    }

}

void tcpListenConn::doAction(void*arg, struct epoll_event *ev)
{
    if (arg == nullptr || ev == nullptr)
    {
        printf("doAction error");
        return;
    }
    EpollObject *ser = (EpollObject*)arg;
    if (ev->events & EPOLLIN)
    {
        struct epoll_event conn_ev;
        struct sockaddr_in peer;
        socklen_t len;

        int new_fd = accept(ev->data.fd,(struct sockaddr*)&peer,&len);    
        if(new_fd > 0)  
        {  
            printf("get a new client:%s:%d\n",inet_ntoa(peer.sin_addr),ntohs(peer.sin_port));
            no_block_fd(new_fd);  
            conn_ev.events = EPOLLIN | EPOLLET;  
            conn_ev.data.fd = new_fd;
            spEpollConn spCon = spEpollConn(new tcpConn());
            ser->addEv(&conn_ev,spCon);
        }
        else
        {
            //error;
            printf("accept error [%d] [%s]\n",errno,strerror(errno));
        }       
    }

}
