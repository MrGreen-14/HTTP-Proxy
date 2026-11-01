#ifndef INTERCEPT_H
#define INTERCEPT_H

#include "common.h"


int intercept(const char*kind,Message*m,long long body_len);

void *console_thread(void*unused);

#endif