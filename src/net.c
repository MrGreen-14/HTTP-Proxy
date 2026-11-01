#include "common.h"
#include "parser.h"
#include "headers.h"
#include "net.h"

ssize_t read_head(int fd, char *data, size_t cap)
{
    size_t len = 0;

    while (len < cap)
    {
        ssize_t n = recv(fd, data + len, cap - len, 0);

        if (n < 0)
        {
            return -1;
        }
        if (n == 0)
        {
            return 0;
        }

        len += (size_t)n;
        data[len] = '\0';

        if (strstr(data, "\r\n\r\n") != NULL)
        {
            return (ssize_t)len;
        }
    }

    return -2;
}

int send_all(int fd, const char *buf, size_t len)
{
    size_t sent = 0;

    while (sent < len)
    {
        ssize_t n = send(fd, buf + sent, len - sent, MSG_NOSIGNAL);

        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }

        sent += (size_t)n;
    }

    return 0;
}

static const char *reason(int code)
{
    switch (code)
    {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 413: return "Payload Too Large";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        default:  return "Unknown";
    }
}

void reply(int fd, int code, const char *body)
{
    char resp[2048];
    int n = snprintf(resp, sizeof resp,
                     "HTTP/1.1 %d %s\r\n"
                     "Content-Type: text/plain; charset=utf-8\r\n"
                     "Content-Length: %zu\r\n"
                     "Connection: close\r\n"
                     "\r\n"
                     "%s",
                     code, reason(code), strlen(body), body);

    if (n > 0 && (size_t)n < sizeof resp)
    {
        send_all(fd, resp, (size_t)n);
    }
}

int parse_url(const char *url, char *host, size_t hcap,int *port, char *path, size_t pcap)
{
    if (strncasecmp(url, "https://", 8) == 0)
    {
        return -2;
    }
    if (strncasecmp(url, "http://", 7) != 0)
    {
        return -1;
    }

    const char *p = url + 7;
    const char *slash = strchr(p, '/');
    size_t alen = slash ? (size_t)(slash - p) : strlen(p);
    char auth[300];

    if (alen == 0 || alen >= sizeof auth)
    {
        return -1;
    }

    memcpy(auth, p, alen);
    auth[alen] = '\0';
    *port = 80;

    char *colon = strrchr(auth, ':');
    if (colon != NULL)
    {
        char *endp;
        long v = strtol(colon + 1, &endp, 10);

        if (colon[1] == '\0' || *endp != '\0' || v < 1 || v > 65535)
        {
            return -1;
        }
        *port = (int)v;
        *colon = '\0';
    }

    if (auth[0] == '\0' || strlen(auth) >= hcap)
    {
        return -1;
    }
    snprintf(host, hcap, "%s", auth);

    if (slash != NULL)
    {
        if (strlen(slash) >= pcap)
        {
            return -1;
        }
        snprintf(path, pcap, "%s", slash);
    }
    else
    {
        snprintf(path, pcap, "/");
    }

    return 0;
}

int connect_to(const char *host, int port)
{
    struct addrinfo hints, *res, *rp;
    char ps[16];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(ps, sizeof ps, "%d", port);

    if (getaddrinfo(host, ps, &hints, &res) != 0)
    {
        return -1;
    }

    int fd = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0)
        {
            continue;
        }
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd >= 0)
    {
        struct timeval tv = {15, 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    }

    return fd;
}

int read_response(int fd, int is_head, char **out_buf,size_t *out_len, size_t *out_hend)
{
    size_t cap = 65536;
    size_t len = 0;
    size_t hend = 0;
    int have_head = 0;
    int nobody = 0;
    long long clen = -1;
    int chunked = 0;
    char *buf = malloc(cap + 1);

    if (buf == NULL)
    {
        return -1;
    }

    for (;;)
    {
        if (len == cap)
        {
            if (cap >= (size_t)MAX_RESP)
            {
                free(buf);
                return -2;
            }

            size_t ncap = cap * 2;
            if (ncap > (size_t)MAX_RESP)
            {
                ncap = (size_t)MAX_RESP;
            }

            char *nb = realloc(buf, ncap + 1);
            if (nb == NULL)
            {
                free(buf);
                return -1;
            }
            buf = nb;
            cap = ncap;
        }

        ssize_t n = recv(fd, buf + len, cap - len, 0);
        if (n <= 0)
        {
            break;
        }

        len += (size_t)n;
        buf[len] = '\0';

        if (!have_head)
        {
            char *e = strstr(buf, "\r\n\r\n");

            if (e != NULL)
            {
                Message tmp;
                int code = 0;

                have_head = 1;
                hend = (size_t)(e - buf) + 4;

                if (parse_message_head(buf, &tmp) < 0)
                {
                    free(buf);
                    return -1;
                }

                sscanf(tmp.first, "%*s %d", &code);
                if (is_head || code == 204 || code == 304 || (code >= 100 && code < 200))
                {
                    nobody = 1;
                }

                const char *cl = find_header(&tmp, "Content-Length");
                if (cl != NULL)
                {
                    clen = strtoll(cl, NULL, 10);
                }

                const char *te = find_header(&tmp, "Transfer-Encoding");
                if (te != NULL && strcasestr(te, "chunked") != NULL)
                {
                    chunked = 1;
                }
            }
        }

        if (have_head)
        {
            if (nobody)
            {
                break;
            }
            if (clen >= 0 && (long long)(len - hend) >= clen)
            {
                break;
            }
            if (chunked && len - hend >= 5 &&
                memcmp(buf + len - 5, "0\r\n\r\n", 5) == 0)
            {
                break;
            }
        }
    }

    if (!have_head)
    {
        free(buf);
        return -1;
    }

    *out_buf = buf;
    *out_len = len;
    *out_hend = hend;
    return 0;
}
