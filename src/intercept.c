#include "common.h"
#include "parser.h"
#include "headers.h"
#include "intercept.h"

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;

static int intercept_on = 1;
static Message *pending = NULL;
static const char *pending_kind = "";
static long long pending_body = 0;
static int decision = 0;

static void print_message(const char *kind, const Message *m, long long body_len)
{
    printf("\n---- %s oprita ----\n", kind);
    printf("%s\n", m->first);

    for (int i = 0; i < m->count; i++)
    {
        printf("%s: %s\n", m->headers[i].nume, m->headers[i].value);
    }

    printf("[body: %lld octeti]\n", body_len);
}

int intercept(const char *kind, Message *m, long long body_len)
{
    pthread_mutex_lock(&gate);
    pthread_mutex_lock(&mu);

    if (!intercept_on)
    {
        pthread_mutex_unlock(&mu);
        pthread_mutex_unlock(&gate);
        return 1;
    }

    pending = m;
    pending_kind = kind;
    pending_body = body_len;
    decision = 0;

    print_message(kind, m, body_len);
    printf("f = forward, d = drop, set Nume: valoare, del Nume, show, off\n> ");
    fflush(stdout);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += TIMEOUT_SEC;

    while (decision == 0)
    {
        int rc = pthread_cond_timedwait(&cv, &mu, &ts);

        if (rc == ETIMEDOUT && decision == 0)
        {
            decision = 1;
            printf("\n(%d s fara decizie: mesajul este trimis automat)\n", TIMEOUT_SEC);
        }
    }

    int d = decision;
    pending = NULL;

    pthread_mutex_unlock(&mu);
    pthread_mutex_unlock(&gate);

    return d == 1;
}

void *console_thread(void *unused)
{
    (void)unused;
    char line[1024];

    while (fgets(line, sizeof line, stdin) != NULL)
    {
        line[strcspn(line, "\r\n")] = '\0';

        pthread_mutex_lock(&mu);

        if (strcmp(line, "on") == 0)
        {
            intercept_on = 1;
            printf("Interceptare PORNITA\n");
        }
        else if (strcmp(line, "off") == 0)
        {
            intercept_on = 0;
            printf("Interceptare OPRITA\n");
            if (pending != NULL)
            {
                decision = 1;
                pthread_cond_broadcast(&cv);
            }
        }
        else if (pending == NULL)
        {
            if (line[0] != '\0')
            {
                printf("Niciun mesaj nu asteapta o decizie acum.\n");
            }
        }
        else if (strcmp(line, "f") == 0 || strcmp(line, "forward") == 0)
        {
            decision = 1;
            pthread_cond_broadcast(&cv);
        }
        else if (strcmp(line, "d") == 0 || strcmp(line, "drop") == 0)
        {
            decision = 2;
            pthread_cond_broadcast(&cv);
        }
        else if (strcmp(line, "show") == 0)
        {
            print_message(pending_kind, pending, pending_body);
        }
        else if (strncmp(line, "set ", 4) == 0)
        {
            Header h;

            if (parse_header_line(line + 4, &h) == 0 &&
                set_header(pending, h.nume, h.value) == 0)
            {
                print_message(pending_kind, pending, pending_body);
            }
            else
            {
                printf("Format: set Nume: valoare\n");
            }
        }
        else if (strncmp(line, "del ", 4) == 0)
        {
            char *nume = trim(line + 4);

            if (remove_header(pending, nume) > 0)
            {
                print_message(pending_kind, pending, pending_body);
            }
            else
            {
                printf("Header inexistent: %s\n", nume);
            }
        }
        else
        {
            printf("Comanda necunoscuta.\n");
        }

        fflush(stdout);
        pthread_mutex_unlock(&mu);
    }

    pthread_mutex_lock(&mu);
    intercept_on = 0;
    if (pending != NULL)
    {
        decision = 1;
        pthread_cond_broadcast(&cv);
    }
    printf("Intrarea standard s-a inchis: interceptarea este oprita\n");
    pthread_mutex_unlock(&mu);

    return NULL;
}