#ifndef NET_H
#define NET_H

#include "common.h"

ssize_t read_head(int fd, char *data, size_t cap);
int send_all(int fd, const char *buf, size_t len);
void reply(int fd, int code, const char *body);
int parse_url(const char *url, char *host, size_t hcap,
              int *port, char *path, size_t pcap);
int connect_to(const char *host, int port);
int read_response(int fd, int is_head, char **out_buf,
                  size_t *out_len, size_t *out_hend);

#endif
