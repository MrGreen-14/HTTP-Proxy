#include "common.h"
#include "parser.h"
#include "headers.h"
#include "intercept.h"
#include "net.h"
#include "build.h"
#include "client.h"

void *client_thread(void *arg)
{
    int cli = *(int *)arg;
    free(arg);

    char data[MAX_HEAD + 1];
    char out[OUT_CAP];
    char first_copy[2304];
    char upfirst[2304];
    char host[256];
    char hostport[300];
    char path[2048];
    Message req;
    Message rsp;
    Request r;
    int port = 80;
    int srv = -1;
    char *resp = NULL;
    size_t resp_len = 0;
    size_t hend_resp = 0;
    long long clen = 0;
    long long remaining = 0;
    const char *cl;
    const char *te;
    int pu;
    int outlen;
    int rr;

    ssize_t len = read_head(cli, data, MAX_HEAD);
    if (len == -2)
    {
        reply(cli, 400, "Cerere prea mare\n");
        goto done;
    }
    if (len <= 0)
    {
        goto done;
    }

    if (parse_message_head(data, &req) < 0)
    {
        reply(cli, 400, "Cerere invalida\n");
        goto done;
    }

    snprintf(first_copy, sizeof first_copy, "%s", req.first);
    if (parse_request_line(first_copy, &r) < 0)
    {
        reply(cli, 400, "Cerere invalida\n");
        goto done;
    }

    if (strcasecmp(r.method, "CONNECT") == 0)
    {
        reply(cli, 501, "HTTPS nu este suportat\n");
        goto done;
    }

    pu = parse_url(r.url, host, sizeof host, &port, path, sizeof path);
    if (pu == -2)
    {
        reply(cli, 501, "HTTPS nu este suportat\n");
        goto done;
    }
    if (pu < 0)
    {
        reply(cli, 400, "Foloseste serverul ca proxy HTTP (URL complet)\n");
        goto done;
    }

    cl = find_header(&req, "Content-Length");
    clen = cl != NULL ? strtoll(cl, NULL, 10) : 0;
    if (!intercept("CERERE", &req, clen))
    {
        reply(cli, 403, "Cererea a fost aruncata (Drop)\n");
        goto done;
    }

    te = find_header(&req, "Transfer-Encoding");
    if (te != NULL && strcasestr(te, "chunked") != NULL)
    {
        reply(cli, 501, "Body chunked in cerere nu este suportat\n");
        goto done;
    }

    cl = find_header(&req, "Content-Length");
    clen = cl != NULL ? strtoll(cl, NULL, 10) : 0;
    if (clen < 0)
    {
        reply(cli, 400, "Content-Length invalid\n");
        goto done;
    }
    if (clen > MAX_BODY)
    {
        reply(cli, 413, "Body prea mare (maxim 10 MB)\n");
        goto done;
    }

    snprintf(hostport, sizeof hostport, "%s:%d", host, port);
    if (find_header(&req, "Host") == NULL)
    {
        set_header(&req, "Host", hostport);
    }

    snprintf(upfirst, sizeof upfirst, "%s %s HTTP/1.1", r.method, path);
    outlen = build_head(upfirst, &req, 1, out, sizeof out);
    if (outlen < 0)
    {
        reply(cli, 400, "Cerere prea mare\n");
        goto done;
    }

    srv = connect_to(host, port);
    if (srv < 0)
    {
        reply(cli, 502, "Serverul nu raspunde\n");
        goto done;
    }

    if (send_all(srv, out, (size_t)outlen) < 0)
    {
        reply(cli, 502, "Serverul nu raspunde\n");
        goto done;
    }

    {
        const char *hend = strstr(data, "\r\n\r\n") + 4;
        size_t extra = (size_t)(data + len - hend);

        remaining = clen;
        if (extra > 0 && remaining > 0)
        {
            size_t take = (long long)extra > remaining ? (size_t)remaining : extra;

            send_all(srv, hend, take);
            remaining -= (long long)take;
        }
        while (remaining > 0)
        {
            char buf[8192];
            size_t want = remaining < (long long)sizeof buf ? (size_t)remaining : sizeof buf;
            ssize_t n = recv(cli, buf, want, 0);

            if (n <= 0 || send_all(srv, buf, (size_t)n) < 0)
            {
                break;
            }
            remaining -= n;
        }
    }

    rr = read_response(srv, strcasecmp(r.method, "HEAD") == 0,
                       &resp, &resp_len, &hend_resp);
    if (rr == -2)
    {
        reply(cli, 502, "Raspuns prea mare (maxim 10 MB)\n");
        goto done;
    }
    if (rr < 0)
    {
        reply(cli, 502, "Serverul nu a raspuns\n");
        goto done;
    }

    if (parse_message_head(resp, &rsp) < 0)
    {
        reply(cli, 502, "Raspuns invalid de la server\n");
        goto done;
    }

    if (!intercept("RASPUNS", &rsp, (long long)(resp_len - hend_resp)))
    {
        reply(cli, 403, "Raspunsul a fost aruncat (Drop)\n");
        goto done;
    }

    set_header(&rsp, "Connection", "close");
    remove_header(&rsp, "Keep-Alive");

    outlen = build_head(rsp.first, &rsp, 0, out, sizeof out);
    if (outlen < 0)
    {
        reply(cli, 502, "Raspuns prea mare\n");
        goto done;
    }

    send_all(cli, out, (size_t)outlen);
    send_all(cli, resp + hend_resp, resp_len - hend_resp);

    printf("%s %s -> %s\n", r.method, r.url, rsp.first);
    fflush(stdout);

done:
    free(resp);
    if (srv >= 0)
    {
        close(srv);
    }
    close(cli);
    return NULL;
}
