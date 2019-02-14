#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <event2/event.h>

//typedef void (*event_callback_fn)(evutil_socket_t, short, void *)
void callback_func(evutil_socket_t fd, short event, void *arg)
{
    char buf[256] = {0};
    int len = 0;
    struct event_base *base = (struct event_base *) arg;

    printf("fd = %d, event = %d", fd, event);

    len = read(fd, buf, sizeof(buf));

    if (len == -1) {
        perror("read");
        return;
    } else if (len == 0) {
        perror("remote close fd");
        return;
    } else {
        buf[len] = '\0';
        printf("read buf=[%s]\n", buf);
        
        FILE *fp = fopen("event_base_stat.txt", "a");
        if (fp == NULL) {
            perror("fopen err");
            exit(1);
        }
        event_base_dump_events(base, fp);
        fclose(fp);
    }

    return; 
}

int main(int argc, char *argv[])
{
    struct event_base * base = NULL;
    struct event *evfifo = NULL;

    const char *fifo = "event.fifo";

    int fd;

    unlink(fifo);

    if (mkfifo(fifo, 0644) == -1) {
        perror("mkfifo");
        exit(1);
    }

    fd = open(fifo, O_RDONLY);
    if (fd == -1) {
        perror("open socket1 error");
        exit(1);
    }

    //创建一个event_base 
    base = event_base_new(); 

    //将fd 绑定到一个事件中  同时绑定一个读的回调函数
    //此时evfifo事件是一个初始化状态的事件
    //evfifo = event_new(base, fd, EV_READ|EV_PERSIST, callback_func, NULL); 
    evfifo = event_new(base, fd, EV_READ|EV_PERSIST, callback_func, base); 

    //将此事件从 初始化--->未决
    event_add(evfifo, NULL);

    //循环等待事件监控
    event_base_dispatch(base);

    event_free(evfifo);
    event_base_free(base);

	return 0;
}
