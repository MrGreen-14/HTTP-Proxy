#include "common.h"
#include "build.h"

static int skip_header(const char *name)
{
    static const char *skip[] = {
        "Proxy-Connection", "Connection", "Keep-Alive",
        "Accept-Encoding", "Proxy-Authorization", "Expect"
    };

    for (size_t i = 0; i < sizeof skip / sizeof skip[0]; i++)
    {
        if (strcasecmp(name, skip[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int build_head(const char *first, const Message *m, int filter,
               char *out, size_t cap)
{
    int w = snprintf(out, cap, "%s\r\n", first);

    if (w < 0 || (size_t)w >= cap)
    {
        return -1;
    }
    size_t n = (size_t)w;

    for (int i = 0; i < m->count; i++)
    {
        if (filter && skip_header(m->headers[i].nume))
        {
            continue;
        }

        w = snprintf(out + n, cap - n, "%s: %s\r\n",
                     m->headers[i].nume, m->headers[i].value);
        if (w < 0 || (size_t)w >= cap - n)
        {
            return -1;
        }
        n += (size_t)w;
    }

    if (filter)
    {
        w = snprintf(out + n, cap - n, "Connection: close\r\n");
        if (w < 0 || (size_t)w >= cap - n)
        {
            return -1;
        }
        n += (size_t)w;
    }

    w = snprintf(out + n, cap - n, "\r\n");
    if (w < 0 || (size_t)w >= cap - n)
    {
        return -1;
    }
    n += (size_t)w;

    return (int)n;
}
