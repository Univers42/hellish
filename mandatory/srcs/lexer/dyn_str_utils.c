#include "lexer.h"

void    d_str_reset(t_dynamic_str *d_str)
{
    if (!d_str || !d_str->str)
        return ;
    d_str->len = 0;
    d_str->str[0] = '\0';
    return ;
}