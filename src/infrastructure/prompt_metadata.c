/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_metadata.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:35:57 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 16:36:30 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

void get_git_info(char **branch, int *dirty)
{
    FILE *f;
    char buf[512];

    *branch = NULL;
    *dirty = 0;
    f = popen("git rev-parse --abbrev-ref HEAD 2>/dev/null", "r");
    if (f)
    {
        if (fgets(buf, sizeof(buf), f))
        {
            size_t l = ft_strlen(buf);
            while (l && (buf[l-1] == '\n' || buf[l-1] == '\r'))
                buf[--l] = '\0';
            if (l > 0)
                *branch = ft_strdup(buf);
        }
        pclose(f);
    }
    f = popen("git status --porcelain 2>/dev/null", "r");
    if (f)
    {
        if (fgets(buf, sizeof(buf), f))
            *dirty = 1;
        pclose(f);
    }
}

char *get_venv_name(void)
{
    char *venv = getenv("VIRTUAL_ENV");
    char *conda = getenv("CONDA_DEFAULT_ENV");
    const char *p = NULL;
    
    if (conda && *conda)
        return ft_strdup(conda);
    if (venv && *venv)
    {
        p = ft_strrchr(venv, '/');
        if (p)
            return ft_strdup(p + 1);
        return ft_strdup(venv);
    }
    return NULL;
}

void get_timebuf(char *buf, size_t buflen)
{
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(buf, buflen, "%H:%M:%S", &tm);
}