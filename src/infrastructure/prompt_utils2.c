/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 16:35:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"


void vec_push_ansi(t_string *v, const char *seq)
{
    char rl_begin = '\001';
    char rl_end = '\002';
    vec_push(v, &rl_begin);
    vec_push_str(v, seq);
    vec_push(v, &rl_end);
}

int get_cols(void)
{
    char *env = getenv("COLUMNS");
    if (env)
    {
        int c = atoi(env);
        if (c > 0)
            return c;
    }
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return (int)ws.ws_col;
    return 80;
}

int measure_width(const char *str)
{
    mbstate_t state;
    wchar_t wc;
    size_t len;
    int total = 0;
    const char *p = str;
    
    memset(&state, 0, sizeof(state));
    while (*p)
    {
        len = mbrtowc(&wc, p, MB_CUR_MAX, &state);
        if (len == (size_t)-1 || len == (size_t)-2)
        {
            total += 1;
            p++;
            memset(&state, 0, sizeof(state));
            continue;
        }
        if (len == 0)
            break;
        int w = wcwidth(wc);
        if (w < 0)
            w = 0;
        total += w;
        p += len;
    }
    return total;
}

char *shorten_path(const char *path, int maxlen)
{
    size_t plen = ft_strlen(path);
    if ((int)plen <= maxlen)
        return ft_strdup(path);
    
    const char *p = path + plen;
    int kept = 0;
    
    while (p > path && kept < maxlen - 4)
    {
        p--;
        if (*p == '/')
            kept = (int)(path + plen - p - 1);
    }
    
    if (p <= path || kept == 0)
        return ft_strdup(path + plen - (maxlen - 4));
    
    char *out = ft_strjoin(".../" , p + 1);
    return out;
}
