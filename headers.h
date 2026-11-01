#ifndef HEADERS_H
#define HEADERS_H

#include "common.h"

const char* find_header(const Message*m,const char*name);
int set_header(Message*m,const char*name,const char*value);
int remove_header(Message*m,const char*name);

#endif