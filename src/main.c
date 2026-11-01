#include "common.h"
#include "intercept.h"
#include "client.h"

int main(void)
{
    signal(SIGPIPE, SIG_IGN);

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0)
    {
        perror("Eroare la socket");
        return 1;
    }

    int on = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(8080);

    if (bind(srv, (struct sockaddr *)&addr, sizeof addr) < 0)
    {
        perror("Eroare la bind");
        return 1;
    }
    if (listen(srv, 64) < 0)
    {
        perror("Eroare la listen");
        return 1;
    }

    printf("Proxy pornit pe 127.0.0.1:8080. Interceptare PORNITA.\n");
    printf("Comenzi: f, d, set Nume: valoare, del Nume, show, on, off\n");
    fflush(stdout);

    pthread_t ct;
    if (pthread_create(&ct, NULL, console_thread, NULL) == 0)
    {
        pthread_detach(ct);
    }

    for (;;)
    {
        int cli = accept(srv, NULL, NULL);
        if (cli < 0)
        {
            perror("Eroare la accept");
            continue;
        }

        int *arg = malloc(sizeof *arg);
        if (arg == NULL)
        {
            close(cli);
            continue;
        }
        *arg = cli;

        pthread_t t;
        if (pthread_create(&t, NULL, client_thread, arg) != 0)
        {
            perror("Eroare la pthread_create");
            free(arg);
            close(cli);
            continue;
        }
        pthread_detach(t);
    }

    close(srv);
    return 0;
}
