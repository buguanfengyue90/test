#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <event2/event.h>

#define N 4096

int main(int argc, char *argv[])
{
    struct event_base *base;

    //创建一个 base对象
    base = event_base_new();

    //向base中安插 监听 事件

    //获取当前系统所支持的多路IO机制
    int i;
    const char **methods = event_get_supported_methods();
    printf("Starting Libevent %s. Available methods are:\n",
            event_get_version());
    for (i=0; methods[i] != NULL; ++i) {
        printf(" %s\n", methods[i]);
    }

    //监听base上绑定的事件,满足后回调相应的函数
    event_base_dispatch(base);          //自带 循环机制 while (1) {

    //释放base
    event_base_free(base);

	return 0;
}
