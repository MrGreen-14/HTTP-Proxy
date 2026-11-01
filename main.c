#include "common.h"
#include "parser.h"
#include "headers.h"
#include "intercept.h"

int main(void)
{
    const char *text =
        "GET http://example.com/ HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: test\r\n"
        "Accept: */*\r\n"
        "\r\n";

    pthread_t ct;
    pthread_create(&ct, NULL, console_thread, NULL);
    pthread_detach(ct);

    printf("Test P2 pornit. Comenzi: f, d, set Nume: valoare, del Nume, show, on, off\n");

    for (int runda = 1; runda <= 3; runda++)
    {
        Message m;

        if (parse_message_head(text, &m) < 0)
        {
            printf("parse esuat\n");
            return 1;
        }

        printf("\n=== Runda %d ===\n", runda);

        int rez = intercept("CERERE", &m, 0);

        printf("Decizie: %s\n", rez ? "FORWARD" : "DROP");
        printf("Mesaj final: %s\n", m.first);

        for (int i = 0; i < m.count; i++)
        {
            printf("  %s: %s\n", m.headers[i].nume, m.headers[i].value);
        }
    }

    return 0;
}