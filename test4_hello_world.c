/*
  This exmple program provides a trivial server program that listens for TCP
  connections on port 9995.  When they arrive, it writes a short message to
  each client connection, and closes each connection once it is flushed.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).
*/


#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

static const char MESSAGE[] = "Hello, World!\n";

static const int PORT = 9995;

static void listener_cb(struct evconnlistener *, evutil_socket_t,
    struct sockaddr *, int socklen, void *);
static void conn_writecb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);
static void signal_cb(evutil_socket_t, short, void *);

int
main(int argc, char **argv)
{
	struct event_base *base;
	struct evconnlistener *listener;
	struct event *signal_event;

	struct sockaddr_in sin;

	base = event_base_new();
	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

    //针对 listen_fd 绑定一个监听事件 同时设置对应的回调函数 listener_cb 
	listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
	    LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
	    (struct sockaddr*)&sin,
	    sizeof(sin));

	if (!listener) {
		fprintf(stderr, "Could not create a listener!\n");
		return 1;
	}

    //将SIGINT信号 绑定一个事件，同时给这个信号注册一个回调函数
	signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);

	if (!signal_event || event_add(signal_event, NULL)<0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}


    //开启事件监听循环
	event_base_dispatch(base);

	evconnlistener_free(listener);
	event_free(signal_event);
	event_base_free(base);

	printf("done\n");
	return 0;
}

//listenner 表示当前listenner本身， fd--->已经连接过来的新的客户端
static void
listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
    struct sockaddr *sa, int socklen, void *user_data)
{
	struct event_base *base = user_data;

    //创建一个bufferevent
	struct bufferevent *bev;

    //根据fd 得到cfd, 并将客户端cfd绑定一个事件 加入到base里, 并设置cfd被close, bev则自动free
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!bev) {
		fprintf(stderr, "Error constructing bufferevent!");
		event_base_loopbreak(base);
		return;
	}

    //给当前这个bufferevent 设置回调函数
	bufferevent_setcb(bev, NULL, conn_writecb, conn_eventcb, NULL);

    //启动bufferevent的写事件监控
	bufferevent_enable(bev, EV_WRITE);

    //禁止bufferevent的读事件监控
	bufferevent_disable(bev, EV_READ);

    //将hello world 写入bufferevent中的写入缓冲区中。
    //使用bufferevent 如果想向客户端发送数据的话，只需要将数据
    //写到 写入缓冲区就可以了， 底层会默认刷新这个缓冲区，通过send发送给当前
    //bufferevent所绑定的cfd

    //如果当前bufferevent的低水位是0,默认的， 当写入缓冲区中的数据为空的时候
    //也就是数据已经全被发给了客户端，才会触发当前bufferevent的写回调函数
	bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
}

static void
conn_writecb(struct bufferevent *bev, void *user_data)
{
    //此时数据已经发送给客户端  数据已经全部发完了，才会触发这个回调函数
    //output此时指向的地址就是当前bev中的写入缓冲区的首地址
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_get_length(output) == 0) {
		printf("flushed answer\n");
        printf("给用户发送的数据已经全部写完，目前输出缓冲区是空\n");
		bufferevent_free(bev);//由于设置 了 BEV_OPT_CLOSE_ON_FREE 当free bev 会默认直接
                            //close bev 对应的cfd
	}
}

static void
conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
	if (events & BEV_EVENT_EOF) {
		printf("Connection closed.\n");
	} else if (events & BEV_EVENT_ERROR) {
		printf("Got an error on the connection: %s\n",
		    strerror(errno));/*XXX win32*/
	}
	/* None of the other events can happen here, since we haven't enabled
	 * timeouts */
	bufferevent_free(bev);
}

static void
signal_cb(evutil_socket_t sig, short events, void *user_data)
{
	struct event_base *base = user_data;
	struct timeval delay = { 2, 0 };

	printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

	event_base_loopexit(base, &delay);
}
