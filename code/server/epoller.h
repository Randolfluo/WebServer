


#ifndef EPOLLER_H
#define EPOLLER_H

#include <cstdint>  //无符号整数
#include <sys/epoll.h>  //epoll
#include <fcntl.h>      //一系列与文件描述符相关的函数，如 fcntl、open、close 等。
#include <unistd.h>     //read、write、close、dup2 
#include <assert.h>
#include <vector>
#include <errno.h>

class Epoller{
public:
    explicit Epoller(int MaxEvent = 1024);

    ~Epoller();

    bool AddFd(int fd, uint32_t events);

    bool ModFd(int fd, uint32_t events);

    bool DelFd(int fd);

    int Wait(int timeoutMs = -1);

    int GetEventFd(size_t i)    const;

    uint32_t GetEvents(size_t i)    const;

private:
    int epollFd_;

    std::vector<struct epoll_event> events_;    
    
};




#endif
