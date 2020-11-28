#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mqnotify.h"
#include "config.h"

static MessageData data;
#define LEN(A) sizeof(A)/sizeof(A[0])

int matches(const regex_t regex, const char* str) {
    printf("Trying to match: '%s'", str);
    regmatch_t match;
    int ret = regexec(&regex, str, 1, &match, 0);
    return ret != REG_NOMATCH && match.rm_so == 0 && match.rm_eo == strlen(str);
}

int matchesRule(const MessageData*data, const Rule*r) {
    for(int n=0;n<4;n++) {
        const char* data_str = data->type+ sizeof(data->type)*n;
        const char* rule_str = (&r->type)[n];
        printf("\t'%s' '%s'\n", data_str, rule_str);
        if(!data_str[0] && rule_str||data_str[0] && rule_str && !matches(r->regexes[n], data_str))
            return 0;
    }
    return 1;
}

void initRules() {
    for(int i=0; i < LEN(rules); i++)
        for(int n=0;n<LEN(rules[0].regexes);n++)
            if((&rules[i].type)[n])
                if(regcomp(&rules[i].regexes[n], (&rules[i].type)[n], REG_EXTENDED) < 0)
                    perror("Failed to compile regex");
}

int spawn(const char* cmd) {
    fflush(NULL);
    int pid = fork();
    if(!pid) {
        const char* const args[] = {"/bin/sh", "-c", cmd, NULL};
        execv(args[0], (char* const*)args);
    }
    return pid;
}

void setEnv(const MessageData*data) {
    setenv(DATA, data->data, 1);
    setenv(APP_NAME, data->appName, 1);
    setenv(BODY, data->body, 1);
    char buffer[16];
    sprintf(buffer, "%d", data->timeout);
    setenv(TIMEOUT, buffer, 1);
    sprintf(buffer, "%d", data->urgency);
    setenv(URGENCY, buffer, 1);
}

void triggerRule(const MessageData*data, const Rule*rule) {
    fflush(NULL);
    if(!fork()) {
        setEnv(data);
        int pid = spawn(rule->cmd);
        int status = 0;
        waitpid(pid, &status, 0);
        int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : -1;
        if(exitCode == 0) {
            if(rule->successCmd)
                spawn(rule->successCmd);
            if(data->signalSuccess)
                kill(data->pid, data->signalSuccess);
        } else {
            if(rule->failCmd)
                spawn(rule->failCmd);
            if(data->signalSuccess)
                kill(data->pid, data->signalSuccess);
        }
    }
}


struct mq_attr attr = {
    .mq_maxmsg = MAX_MESSAGES,
    .mq_msgsize = MQ_MAX_MSG_LEN
};
int main() {
    mqd_t mqd = mq_open(MQ_NAME, O_RDONLY|O_CLOEXEC|O_CREAT, 0722,  &attr);
    if(mqd == -1) {
        perror("Failed to create ");
        exit(1);
    }
    initRules();
    unsigned int msgPriority;
    struct mq_attr attr2;
    if (mq_getattr(mqd, &attr2) == -1)
        exit(1);
    printf("%ld %ld %ld\n", attr.mq_msgsize , attr2.mq_msgsize ,MQ_MAX_MSG_LEN);
    while(1) {
        int ret = mq_receive(mqd,(char*) &data, MQ_MAX_MSG_LEN, &msgPriority);
        printf("Recieved msg %s %s\n", data.header, data.body);
        if(ret == -1) {
            perror("Failed to read message");
            if(errno == EBADF)
                exit(1);
            exit(2);
            continue;
        }
        for(int i=0; i < LEN(rules); i++){
                printf("Checkign rules %d\n",i);
            if(matchesRule(&data, rules+i)) {
                printf("Triggering rule %s\n",rules[i].type);
                triggerRule(&data, rules+i);
                if(!(rules[i].flags & CONTINUE_ON_MATCH))
                    break;
            }
        }
    }
}
