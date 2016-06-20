#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define MAX_EVENTS 32
#define IN_FIFO_NAME "/home/box/in.fifo"
#define OUT_FIFO_NAME "/home/box/out.fifo"

int set_nonblock(int fd)
{
  int flags;
#if defined(O_NONBLOCK)
  if(-1 == (flags = fcntl(fd, F_GETFL, 0)))
    flags = 0;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
  flags = 1;
  return ioctl(fd, FIOBIO, &flags);
#endif
}

int main(int argc, char **argv)
{
  unlink(IN_FIFO_NAME);
  unlink(OUT_FIFO_NAME);
  if((mkfifo(IN_FIFO_NAME, O_RDWR)) == -1)
  {
    std::cerr << "Невозможно создать fifo\n";
    return -1;
  }
  if((mkfifo(OUT_FIFO_NAME, O_RDWR)) == -1)
  {
    std::cerr << "Невозможно создать fifo\n";
    return -1;
  }

  int ReadFifo = 0;
  if (( ReadFifo = open(IN_FIFO_NAME, O_RDONLY | O_NONBLOCK))<0)
  {
    std::cerr << "read fifo open\n";
    return -1;
  }
  int WriteFifo = 0;
  if((WriteFifo = open(OUT_FIFO_NAME, O_WRONLY))<0)
  {
    std::cerr << "write fifo open\n";
    return -1;
  }

  set_nonblock(ReadFifo);
  set_nonblock(WriteFifo);

  int EPoll = epoll_create1(0);
  struct epoll_event Event;
  Event.data.fd = ReadFifo;
  Event.events = EPOLLIN;
  epoll_ctl(EPoll, EPOLL_CTL_ADD, ReadFifo, &Event);

  while(true)
  {
    struct epoll_event Events[MAX_EVENTS];
    int N = epoll_wait(EPoll, Events, MAX_EVENTS, -1);
    for(int i = 0; i < N; i++)
    {
      if(Events[i].data.fd == ReadFifo)
      {
        static char Buffer[1024];
        int RecvResult = read(Events[i].data.fd, Buffer, 1024);
        if(RecvResult == 0)
        {
          close(Events[i].data.fd);
        }
        else if(RecvResult > 0)
        {
          write(WriteFifo, Buffer, 1024);
        }
      }
    }
  }

  return 0;
}
