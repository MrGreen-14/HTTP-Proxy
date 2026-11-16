#include "common.h"
#include "history.h"

static FILE *fp = NULL;
static pthread_mutex_t hmu = PTHREAD_MUTEX_INITIALIZER;
static unsigned long next_id = 1;
static long run_id = 0;

int history_open(const char *path)
{
    int rc;

    pthread_mutex_lock(&hmu);
    if (fp != NULL)
    {
        fclose(fp);
        fp = NULL;
    }
    fp = fopen(path, "a");
    run_id = (long)time(NULL);
    rc = fp != NULL ? 0 : -1;
    pthread_mutex_unlock(&hmu);
    return rc;
}

void history_close(void)
{
    pthread_mutex_lock(&hmu);
    if (fp != NULL)
    {
        fclose(fp);
        fp = NULL;
    }
    pthread_mutex_unlock(&hmu);
}

unsigned long history_new_id(void)
{
    unsigned long id;

    pthread_mutex_lock(&hmu);
    id = next_id++;
    pthread_mutex_unlock(&hmu);
    return id;
}


static int valid_utf8(const unsigned char *s, size_t n)
{
    size_t i = 0;

    while (i < n)
    {
        unsigned char c = s[i];
        size_t need;

        if (c < 0x80)
        {
            i++;
            continue;
        }
        if (c >= 0xC2 && c <= 0xDF)
        {
            need = 1;
        }
        else if (c >= 0xE0 && c <= 0xEF)
        {
            need = 2;
        }
        else if (c >= 0xF0 && c <= 0xF4)
        {
            need = 3;
        }
        else
        {
            return 0;
        }

        if (i + need >= n)
        {
            return 0;
        }
        for (size_t k = 1; k <= need; k++)
        {
            if ((s[i + k] & 0xC0) != 0x80)
            {
                return 0;
            }
        }
        if ((c == 0xE0 && s[i + 1] < 0xA0) || (c == 0xED && s[i + 1] >= 0xA0) ||
            (c == 0xF0 && s[i + 1] < 0x90) || (c == 0xF4 && s[i + 1] >= 0x90))
        {
            return 0;
        }
        i += need + 1;
    }
    return 1;
}


static void put_json_str(FILE *f, const unsigned char *s, size_t n, int utf8_ok)
{
    fputc('"', f);
    for (size_t i = 0; i < n; i++)
    {
        unsigned char c = s[i];

        switch (c)
        {
            case '"':  fputs("\\\"", f); break;
            case '\\': fputs("\\\\", f); break;
            case '\n': fputs("\\n", f);  break;
            case '\r': fputs("\\r", f);  break;
            case '\t': fputs("\\t", f);  break;
            default:
                if (c < 0x20)
                {
                    fprintf(f, "\\u%04x", c);
                }
                else if (c >= 0x80 && !utf8_ok)
                {
                    fputc('?', f);
                }
                else
                {
                    fputc(c, f);
                }
        }
    }
    fputc('"', f);
}


static void put_text(FILE *f, const char *s)
{
    size_t n = strlen(s);

    put_json_str(f, (const unsigned char *)s, n, valid_utf8((const unsigned char *)s, n));
}


static void put_b64(FILE *f, const unsigned char *s, size_t n)
{
    static const char T[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t i = 0;

    fputc('"', f);
    for (; i + 2 < n; i += 3)
    {
        unsigned v = ((unsigned)s[i] << 16) | ((unsigned)s[i + 1] << 8) | s[i + 2];

        fputc(T[v >> 18], f);
        fputc(T[(v >> 12) & 63], f);
        fputc(T[(v >> 6) & 63], f);
        fputc(T[v & 63], f);
    }
    if (n - i == 1)
    {
        unsigned v = (unsigned)s[i] << 16;

        fputc(T[v >> 18], f);
        fputc(T[(v >> 12) & 63], f);
        fputs("==", f);
    }
    else if (n - i == 2)
    {
        unsigned v = ((unsigned)s[i] << 16) | ((unsigned)s[i + 1] << 8);

        fputc(T[v >> 18], f);
        fputc(T[(v >> 12) & 63], f);
        fputc(T[(v >> 6) & 63], f);
        fputc('=', f);
    }
    fputc('"', f);
}

void history_log(unsigned long id, const char *kind, const char *decision,
                 const Message *m, const char *body, size_t body_have,
                 long long body_len)
{
    const unsigned char *b = (const unsigned char *)body;
    size_t keep;
    int utf8;
    int truncated;
    time_t now;
    struct tm tmv;
    char ts[32];

    pthread_mutex_lock(&hmu);
    if (fp == NULL)
    {
        pthread_mutex_unlock(&hmu);
        return;
    }

    now = time(NULL);
    gmtime_r(&now, &tmv);
    strftime(ts, sizeof ts, "%Y-%m-%dT%H:%M:%SZ", &tmv);

    keep = (b == NULL) ? 0 : body_have;
    if (keep > HISTORY_BODY_MAX)
    {
        keep = HISTORY_BODY_MAX;
    }
    if (b != NULL && keep < body_have)
    {
        while (keep > 0 && (b[keep] & 0xC0) == 0x80)
        {
            keep--;
        }
    }
    utf8 = valid_utf8(b, keep);
    truncated = (long long)keep < body_len;

    fprintf(fp, "{\"run\":%ld,\"id\":%lu,\"ts\":\"%s\",\"kind\":\"%s\",\"decision\":\"%s\",\"first\":",
            run_id, id, ts, kind, decision);
    put_text(fp, m->first);

    fputs(",\"headers\":[", fp);
    for (int i = 0; i < m->count; i++)
    {
        if (i > 0)
        {
            fputc(',', fp);
        }
        fputs("{\"name\":", fp);
        put_text(fp, m->headers[i].nume);
        fputs(",\"value\":", fp);
        put_text(fp, m->headers[i].value);
        fputc('}', fp);
    }

    fprintf(fp, "],\"body_len\":%lld,\"body_truncated\":%s,\"body_encoding\":\"%s\",\"body\":",
            body_len, truncated ? "true" : "false", utf8 ? "utf8" : "base64");
    if (utf8)
    {
        put_json_str(fp, b, keep, 1);
    }
    else
    {
        put_b64(fp, b, keep);
    }
    fputs("}\n", fp);

    fflush(fp);
    pthread_mutex_unlock(&hmu);
}

void history_error(unsigned long id, int code, const char *message)
{
    Message m;

    memset(&m, 0, sizeof m);
    snprintf(m.first, sizeof m.first, "%d %s", code, message);
    m.first[strcspn(m.first, "\r\n")] = '\0';
    history_log(id, "EROARE", "proxy", &m, NULL, 0, 0);
}
