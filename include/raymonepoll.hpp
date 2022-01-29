
#ifndef __RAYMON_SHAN_EPOLL_H
#define __RAYMON_SHAN_EPOLL_H

#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>

#include "raymonepollbuffer.hpp"
#include "raymonparser.hpp"
#include "raymontest.hpp"  

#define EPOLL_MAX_EVENTS  100

#define __CLOSE_AND_FREE  do { tbuffer->Put(info); events[i].data.ptr = 0; close(readfd); } while (0)

// response are generate by other thread/process and write to fd.
typedef class WriteSeparateEpoll {
private:
  int servsock;
  pInterfaceBuffer tbuffer;
  pInterfaceParser tparser;
  long idlewait;
  int epollfd;  
  
public:
  WriteSeparateEpoll(pInterfaceBuffer tb, pInterfaceParser tp, long idlewait) {
    tbuffer = tb;
    tparser = tp;
    this->idlewait = idlewait;
  }

  unsigned long BeginListen(short listenport) {
    struct sockaddr_in servaddr;
    struct epoll_event ev;
    pEpollBuffer info;
    //    socklen_t addr_len;

    servsock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    servaddr.sin_port = htons(listenport);

    int reuseaddr = 1;    // begin listen when time_wait
    setsockopt(servsock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int));
    bind(servsock, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (listen(servsock, SOMAXCONN) != 0) {
      _LOG_ATTE("CRITICAL ERROR LISTEN ON PORT %d, errno:%d, %s", listenport, errno, strerror(errno));
      exit(1);
    }

    epollfd = epoll_create(EPOLL_MAX_EVENTS);
    if (epollfd == -1) {
      _LOG_ATTE("CRITICAL ERROR CREATE EPOLL, errno:%d, %s", errno, strerror(errno));
      exit(1);
    }
    ev.data.fd = servsock;
    info = tbuffer->Get();
    if (info == 0) {
      _LOG_ATTE("GET FIRST BUFFER ERROR, CANNOT CONTINUE");
      exit(1);
    }
    info->socketFd = servsock;
    ev.data.ptr = info;
    ev.events = EPOLLET | EPOLLIN | EPOLLHUP | EPOLLRDHUP;    
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, servsock, &ev)) {
      _LOG_ATTE("CANNOT ADD ACCEPT SOCKET TO EPOLL, CANNOT CONTINUE");
      exit(1);
    }

    timespec now;
    timespec lastidle;
    clock_gettime(CLOCK_MONOTONIC, &lastidle);
    
    struct epoll_event events[EPOLL_MAX_EVENTS];
    int nfds, readfd;
    char *readbuf;
    int nread;

    unsigned long fnreturn;
    struct sockaddr_in  clntaddr;
    socklen_t clntaddrsize = sizeof(clntaddr);
    int clntsock;

    _LOG_ATTE("listen on port %d, socket %d OK !!!", listenport, servsock);    
    while (1) {
      nfds = epoll_wait(epollfd, events, EPOLL_MAX_EVENTS, idlewait);
      clock_gettime(CLOCK_MONOTONIC, &now);
      if ((now.tv_sec - lastidle.tv_sec) * 1000 + (now.tv_nsec - lastidle.tv_nsec) / 1000000 > idlewait) {
	//	_LOG_TRAC("into idle schedule");
	lastidle = now;
	tparser->Idle();
      }
      
      if (nfds > 0)
	_LOG_DBUG("before epoll loop, get %d events", nfds);
      for (int i = 0; i < nfds; i++) {
	info = (pEpollBuffer)events[i].data.ptr;
	if (!info) {
	  _LOG_EROR("UNKNOW REASON, THREAD GET EMPTY EPOLL dataptr");
	  continue;
	}
	readfd = info->socketFd;
	readbuf = info->bufferPointer;
	_LOG_DBUG("thread for eqoll socket: %d, event: %d, buffer:%p", readfd, events[i].events, info);
	if (readfd == 0 || readbuf == 0) {
	  _LOG_EROR("UNKNOW REASON, fd or buf IS ZERO");
	  continue;
	}
	
	if (readfd == servsock) {       // listen socket, do accept
	  if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP) ) {
	    _LOG_CRIT("THREAD FOR ACCEPT SOCKET: %d,  ERROR!!!   SYSTEM MAYBE HALT!!!", servsock);
	  }
	  if (events[i].events & EPOLLIN) {
	    do {
	      clntsock = accept4(servsock, (struct sockaddr*)&clntaddr, &clntaddrsize, SOCK_NONBLOCK);
	      if (clntsock <= 0) {
		if (errno != EAGAIN) {
		  _LOG_EROR("ERROR IN ACCEPT errno:%d %s", errno, strerror(errno));
		}
		break;
	      }

	      ev.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLRDHUP;
	      info = tbuffer->Get();
	      if (info == 0) {
		_LOG_EROR("OUT OF BUFFER, connect limit, no connect will be accept");
		close(clntsock);
		break;
	      }
	      info->socketFd = clntsock;
	      ev.data.ptr = info;
	      _LOG_INFO("get connection %s:%d, socket %d, alloc buffer %p", inet_ntoa(clntaddr.sin_addr), ntohs(clntaddr.sin_port), clntsock, info );
	      if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clntsock, &ev)) {
		_LOG_EROR("CANNOT ADD SOCKET TO EPOLL, SOCKET:%d, CLOSE AND FREE BUFFER", clntsock);
		__CLOSE_AND_FREE;
	      }
	    } while (clntsock  > 0);
	    if (errno != EAGAIN) {
	      _LOG_CRIT("THREAD FOR ACCEPT SOCKET %d ERROR!!!   SYSTEM MAYBE HALT!!!", servsock);
	    }
	  }
	  continue;
	}

	// following for connected socket read
	if (events[i].events & EPOLLERR) {
	  _LOG_WARN("thread for read socket %d, closed by system, free buffer %p", readfd, info);
	  tbuffer->Put(info);
	  events[i].data.ptr = 0;
	  continue;
	}
	if (events[i].events & EPOLLHUP) {
	  _LOG_INFO("thread for read socket %d, closed by local, free buffer %p", readfd, info);
	  tbuffer->Put(info);
	  events[i].data.ptr = 0;
	  continue;	  
	}
	if (events[i].events & EPOLLRDHUP) {
	  _LOG_INFO("thread for read socket %d, closed by remote, free buffer %p", readfd, info);
	  __CLOSE_AND_FREE;
	  continue;	  
	}
	if (events[i].events & EPOLLIN) {
	  info->bufferEnd = info->bufferBegin;
	  do {
	    nread = read(readfd, readbuf + info->bufferEnd, info->bufferSize - info->bufferEnd - 1);
	    if (nread < 0 && errno != EAGAIN) {
	      _LOG_EROR("THREAD FOR READ SOCKET %d ERROR!!!, CLOSE AND FREE BUFFER %p", readfd, info);
	      __CLOSE_AND_FREE;
	      break;
	    }
	    if (nread < 0 && errno == EAGAIN) break;
	    info->bufferEnd += nread;   // here nread MUST >= 0
	  } while (1);
	  _LOG_INFO("thread for read socket, socket %d, readed %ld bytes", readfd, info->bufferEnd - info->bufferBegin);
	  readbuf[info->bufferEnd] = 0;
	  // LOG_DBUG << "data:" << readbuf;

	  if (info->bufferBegin != info->bufferEnd) {
	    fnreturn = tparser->Request(info);
	    if (info->bufferBegin >= info->bufferSize - 1) {
	      _LOG_WARN("thread for read socket %d this buffer full, close and free buffer %p", readfd, info);
	      __CLOSE_AND_FREE;
	    } else if (fnreturn & REQUEST_CLOSE) {
	      _LOG_INFO("thread for read socket %d application close and free buffer %p", readfd, info);
	      __CLOSE_AND_FREE;
	    } else if (fnreturn & REQUEST_CONTINUE) {
	      info->bufferBegin = info->bufferEnd;
	      // printf("in contiune\n");
	    } else { // IS REQUEST_COMPLETE
	      info->bufferBegin = info->bufferEnd = 0;
	      // printf("in complete\n");
	    }
	  } else {
	    _LOG_EROR("ERROR BUFFER IS FULL, will close socket %d and free buffer %p, begin: %ld, end:%ld, size %ld",
		      readfd, info, info->bufferBegin, info->bufferEnd, info->bufferSize);
	    tbuffer->Put(info);
	    events[i].data.ptr = 0;
	    close(readfd); 
	  }
	}
	if (events[i].events & EPOLLOUT) {
	  // SHOULD DO LATER
	// if (oldval & SHOULD_CLOSE) closeFd(ab, &events[i]);
	}
      }
    }
    return 0;
  }
} WriteSeparateEpoll, *pWriteSeparateEpoll;




#endif  // __RAYMON_SHAN_EPOLL_H
      

