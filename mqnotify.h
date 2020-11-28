#ifndef MQ_NOTIFY_H
#define MQ_NOTIFY_H
#include <regex.h>

#define MQ_MAX_MSG_LEN sizeof(MessageData)
#define MQ_NAME "/NOTIFICATION_SERVER"

#define DATA "DATA"
#define APP_NAME "APP_NAME"
#define BODY "BODY"
#define TIMEOUT "TIMEOUT"
#define URGENCY "URGENCY"

typedef enum {
    ANY=0, LOW, NORMAL, CRITICAL
} Urgency;

typedef struct {
    // pid of calling process
    int pid;
    // message id to replace an existing notification
    char mid[64];
    Urgency urgency;
    int timeout;
    // if non-zero send signal if cmd of matching rule exits with status 0
    char signalSuccess;
    // if non-zero send signal if cmd of matching rule exits with non-zero status
    char signalFail;
    // user specified classification
    char type[64];
    // application name
    char appName[64];
    // message header
    char header[64];
    // message body
    char body[255];

    char data[64];
}MessageData;

enum {
    CONTINUE_ON_MATCH = 1
};

typedef struct {
    const char* const type;
    const char* const appName;
    const char* const header;
    const char* const body;
    const char* const cmd;
    const char* const successCmd;
    const char* const failCmd;
    const int flags;
    regex_t regexes[4];
} Rule;
#endif
