#include <fcntl.h>           /* For O_* constants */
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "mqnotify.h"

#define LEN(A) (sizeof(A)/sizeof(A[0]))

#define COPY(F) strncpy(data.F, optarg, LEN(data.F)-1);
#define TOINT(F) data.F=atoi(optarg);
int main(int argc, char*argv[]) {
    int opt;
    int priority = 1;
    MessageData data = { .pid=getppid()};
    while((opt = getopt(argc, argv, "a:h:s:f:p:t:m:")) != -1)  {
        switch(opt)  {
            case 'a':
                COPY(appName);
                break;
            case 't':
                TOINT(timeout);
                break;
            case 'm':
            case 'h':
                COPY(mid);
                break;
            case 'p':
                TOINT(urgency);
                break;
            case 'c':
                COPY(type);
                break;
            case 's':
                TOINT(signalSuccess);
                break;
            case 'f':
                TOINT(signalFail);
                break;
        }
    }
    if( optind < argc)
        strncpy(data.header, argv[optind++], LEN(data.header)-1);
    if( optind < argc)
        strcat(data.body, argv[optind++]);
    else
        read(0, data.body, sizeof(data.body));
    printf("'%s' '%s'\n", data.header, data.body);
    mqd_t mqd = mq_open(MQ_NAME, O_WRONLY|O_CLOEXEC);
    return mq_send(mqd, (char*)&data, MQ_MAX_MSG_LEN, priority);
}
