#ifndef HISTORY_H
#define HISTORY_H

#include "common.h"

#define HISTORY_BODY_MAX 65536

int history_open(const char *path);

void history_close(void);

unsigned long history_new_id(void);

/*
 * Scrie O LINIE JSON despre un mesaj.
 *   id        - de la history_new_id(), acelasi pentru cerere si raspuns
 *   kind      - "CERERE" sau "RASPUNS"
 *   decision  - "forward", "drop", "timeout" sau "passthrough"
 *   m         - mesajul (prima linie + headerele), in forma finala, dupa editari
 *   body      - octetii corpului pe care ii avem in memorie (poate fi NULL)
 *   body_have - cati octeti sunt la adresa body
 *   body_len  - marimea REALA a corpului (poate fi mai mare decat body_have)
 */
void history_log(unsigned long id, const char *kind, const char *decision,
                 const Message *m, const char *body, size_t body_have,
                 long long body_len);

#endif