#ifndef COMMON_H
#define COMMON_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<sys/types.h>
#include<errno.h>
#include<pthread.h>
#include<signal.h>
#include<stdio.h>
#include<string.h>
#include<time.h>
#include<unistd.h>

#define MAX_HEADERS 64
#define MAX_LINE 4096
#define MAX_HEAD 65536
#define OUT_CAP 90000
#define MAX_BODY (10LL * 1024*1024)
#define MAX_RESP (10LL*1024*1024)

#ifndef TIMEOUT_SEC
#define TIMEOUT_SEC 60
#endif

typedef struct Header
{
    char nume[64];
    char value[1024];
}Header;

typedef struct Message
{
    char first[2304];
    Header headers[MAX_HEADERS];
    int count;
}Message;

typedef struct Request
{
    char method[16];
    char url[2048];
    char version[16];
}Request;

#endif
