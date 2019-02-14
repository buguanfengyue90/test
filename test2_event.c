#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <event2/event.h>

void cb_func(evutil_socket_t fd, short what, void *arg)
{
    const char *data = arg;
    printf("Got an event on socket %d:%s%s%s%s [%s]\n",
            (int) fd,
            (what&EV_TIMEOUT) ? " timeout" : "",
            (what&EV_READ) ? " read" : "",
            (what&EV_WRITE) ? " write" : "",
            (what&EV_SIGNAL) ? " signal" : "",
            data);

}

void main_loop(evutil_socket_t fd1, evutil_socket_t fd2)
{
    struct event *ev1, *ev2;
    struct timeval five_seconds = {5,0};
    struct event_base *base = event_base_new();
    /* The caller has already set up fd1, fd2 somehow, and m
     * ake them
     * nonblocking. */

    //监听 fd1 是否可读, 设置超时/持续监听
    ev1 = event_new(base, fd1, EV_TIMEOUT|EV_READ|EV_PERSIST
            , cb_func,
            (char*)"Reading event");

    //监听 fd2 是否可写,并且自动将非未决转换成未决
    //ev2 = event_new(base, fd2, EV_WRITE|EV_PERSIST, cb_func,  事件处于 初始化状态
    ev2 = event_new(base, fd2, EV_WRITE, cb_func,
            (char*)"Writing event");

    //调用结束的时候,事件处于 未决状态
    event_add(ev1, &five_seconds);
    event_add(ev2, NULL);

    //监听 安插ev1 和 ev2 的base. 当对应事件满足时 直接调用回调函数处理
    event_base_dispatch(base);

    event_base_free(base);
}

int main(int argc, char *argv[])
{
    const char *fifo = "event.fifo";

    int fd1, fd2;

    unlink(fifo);           //确保fifo不存在

    if (mkfifo(fifo, 0644)) {
        perror("mkfifo error");
        exit(1);
    }

    fd1 = open(fifo, O_RDONLY|O_NONBLOCK, 0644);
    if (fd1 < 0) {
        perror("open fifo error");
        exit(1);
    }

    fd2 = open(fifo, O_WRONLY|O_NONBLOCK, 0644);
    if (fd2 < 0) {
        perror("open fifo error");
        exit(1);
    }

    main_loop(fd1, fd2);

    close(fd1);
    close(fd2);

    return 0;
}
