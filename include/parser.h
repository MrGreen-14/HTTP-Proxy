#ifndef PARSER_H
#define PARSER_H

#include "common.h"

char*trim(char*s);
int parse_request_line(char*line,Request *r);
int parse_header_line(char*line,Header*h);
int parse_message_head(const char*text,Message*m);

#endif