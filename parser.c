#include "common.h"
#include "parser.h"

char *trim(char*s)
{
    while(*s==' '|| *s=='\t')
    {
        s++;
    }
    size_t n=strlen(s);
    while(n>0&&(s[n-1]==' '||s[n-1]=='\t'))
    {
n--;
    }
    s[n]='\0';
    return s;
}

int parse_request_line(char*line,Request*r)
{
    char*sp1=strchr(line,' ');
    char*sp2=strrchr(line,' ');

    if(sp1==NULL||sp1==sp2)
    {
        return -1;
    }
    *sp1='\0';
    *sp2='\0';
    char *method=line;
    char*url=trim(sp1+1);
    char*version=trim(sp2+1);

    if(method[0]=='\0'||url[0]=='\0')
    {
        return -1;
    }

    if(strlen(method)>=sizeof r->method || strlen(url)>=sizeof r->url||strlen(version)>= sizeof r->version)
    {
        return -1;
    }

    snprintf(r->method,sizeof r->method,"%s",method);
    snprintf(r->url,sizeof r->url,"%s",url);
    snprintf(r->version,sizeof r->version,"%s",version);
    return 0;
}


int parse_header_line(char*line,Header*h)
{
    char*colon=strchr(line,':');
    if(colon==NULL||colon==line)
    {
        return -1;
    }

    *colon='\0';
    char*name=trim(line);
    char*value=trim(colon+1);
    if(name[0]=='\0')
    {
        return -1;
    }
    if(strlen(name)>=sizeof h->nume||strlen(value)>=sizeof h->value)
    {
        return -1;
    }
    snprintf(h->nume,sizeof h->nume,"%s",name);
    snprintf(h->value,sizeof h->value,"%s",value);
    return 0;
}


int parse_message_head(const char*text,Message*m)
{
    char line[MAX_LINE];
    const char*p=text;
    const char*end;
    int first=1;
    m->count=0;
    m->first[0]='\0';

    while((end=strstr(p,"\r\n"))!=NULL)
    {
        size_t n=(size_t)(end-p);
        if(n>=sizeof line)
        {
            return -1;
        }
        memcpy(line,p,n);
        line[n]='\0';
        p=end+2;
        if(first)
        {
            if(n==0||n>=sizeof m->first)
            {
                return -1;
            }
            memcpy(m->first,line,n+1);
            first=0;
        }
        else if(n==0)
        {
            return 0;
        }
        else{
            if(m->count>=MAX_HEADERS)
            {
                return -1;
            }
            if(parse_header_line(line,&m->headers[m->count])<0)
            {
                return -1;
            }
            m->count++;
        }
    }
    return -1;
}