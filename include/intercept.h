#ifndef INTERCEPT_H
#define INTERCEPT_H

#include "common.h"


/* Ce intoarce intercept(): 0 = DROP, orice alta valoare = FORWARD (cu motivul ei). */
enum
{
    DEC_DROP = 0,        /* ai dat d                                        */
    DEC_FORWARD = 1,     /* ai dat f (sau off cat timp astepta)             */
    DEC_TIMEOUT = 2,     /* nicio decizie in TIMEOUT_SEC: trimis automat    */
    DEC_PASSTHROUGH = 3  /* interceptarea era oprita (off): nu s-a oprit nimic */
};

int intercept(const char*kind,Message*m,long long body_len);

/* Numele deciziei ("drop", "forward", "timeout", "passthrough"), pentru jurnal. */
const char *intercept_decision_name(int dec);

void *console_thread(void*unused);

#endif
