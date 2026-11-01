#include "common.h"
#include "headers.h"

const char*find_header(const Message*m,const char*name)
{
    for(int i=0;i<m->count;i++)
    {
        if(strcasecmp(m->headers[i].nume,name)==0)
        {
            return m->headers[i].value;
        }
    }
    return NULL;
}


int set_header(Message*m,const char*name,const char*value)
{
    if(strlen(name)>=sizeof m->headers[0].nume||strlen(value)>=sizeof m->headers[0].value)
    {
        return -1;
    }

    for(int i=0;i<m->count;i++)
    {
        if(strcasecmp(m->headers[i].nume,name)==0)
        {
        snprintf(m->headers[i].value,sizeof m->headers[i].value,"%s",value);
        return 0;
        }
    }
    if(m->count>=MAX_HEADERS)
    {
        return -1;
    }
    snprintf(m->headers[m->count].nume,sizeof m->headers[m->count].nume,"%s",name);
    snprintf(m->headers[m->count].value,sizeof m->headers[m->count].value,"%s",value);
    m->count++;
    return 0;
}


int remove_header(Message*m,const char*name)
{
    int removed=0;
    int j=0;
    for(int i=0;i<m->count;i++)
    {
        if(strcasecmp(m->headers[i].nume,name)==0)
        {
            removed++;
            continue;
        }
        if(i!=j)
        {
            m->headers[j]=m->headers[i];

        }
        j++;
    }
    m->count=j;
    return removed;

}