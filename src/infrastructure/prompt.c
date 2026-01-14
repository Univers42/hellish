/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:22 by marvin            #+#    #+#             */
/*   Updated: 2026/01/12 00:45:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <stdio.h>
#include <readline/readline.h>
#include "parser.h"
#include <locale.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

static void vec_push_ansi(t_string *v, const char *seq)
{
    char rl_begin = '\001';
    char rl_end = '\002';
    vec_push(v, &rl_begin);
    vec_push_str(v, seq);
    vec_push(v, &rl_end);
}

static int get_cols(void)
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

static int measure_width(const char *str)
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

static char *shorten_path(const char *path, int maxlen)
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

static void get_git_info(char **branch, int *dirty)
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

static char *get_venv_name(void)
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

static void get_timebuf(char *buf, size_t buflen)
{
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(buf, buflen, "%H:%M:%S", &tm);
}

t_string prompt_more_input(t_parser *parser)
{
    t_string ret;
    t_tt curr;
    size_t i = 0;

    vec_init(&ret);
    ret.elem_size = 1;
    
    while (i < parser->parse_stack.len)
    {
        curr = *(int *)vec_idx(&parser->parse_stack, i++);
        if (curr == TT_BRACE_LEFT)
            vec_push_str(&ret, "subsh");
        else if (curr == TT_PIPE)
            vec_push_str(&ret, "pipe");
        else if (curr == TT_AND)
            vec_push_str(&ret, "cmdand");
        else if (curr == TT_OR)
            vec_push_str(&ret, "cmdor");
        else
            continue;
        vec_push_str(&ret, " ");
    }
    if (ret.len > 0)
        ((char *)ret.ctx)[ret.len - 1] = '>';
    vec_push_str(&ret, " ");
    return ret;
}

t_string prompt_normal(t_shell *state)
{
    t_string ret;
    char *user;
    char cwd[PATH_MAX + 1];
    char *short_cwd;
    int cols;
    char *branch;
    int branch_dirty;
    char *venv;
    char time_buf[32];
    int left_width;
    int time_width;
    int pad;

    (void)state;

    ensure_locale();
    vec_init(&ret);
    ret.elem_size = 1;
    
    user = getenv("USER");
    if (!user)
        user = "user";
    
    if (!getcwd(cwd, sizeof(cwd)))
        ft_strcpy(cwd, "~");
    
    cols = get_cols();
    short_cwd = shorten_path(cwd, ft_max(20, cols - 50));
    
    branch = NULL;
    branch_dirty = 0;
    get_git_info(&branch, &branch_dirty);
    
    venv = get_venv_name();
    get_timebuf(time_buf, sizeof(time_buf));
    
    /* Calculate visible width */
    left_width = 0;
    left_width += measure_width("╭─");
    left_width += 1;  /* space */
    left_width += ft_strlen(user);
    left_width += 1;  /* space */
    left_width += measure_width("─");
    left_width += 1;  /* space */
    left_width += ft_strlen(short_cwd);
    
    if (branch)
    {
        left_width += 1;  /* space */
        left_width += measure_width("─");
        left_width += 1;  /* space */
        left_width += ft_strlen(branch);
    }
    
    if (venv)
    {
        left_width += 2;  /* " (" */
        left_width += ft_strlen(venv);
        left_width += 1;  /* ")" */
    }
    
    /* Build line 1 */
    vec_push_ansi(&ret, "\033[90m");
    vec_push_str(&ret, "╭─");
    vec_push_ansi(&ret, "\033[0m");
    vec_push_str(&ret, " ");
    
    vec_push_ansi(&ret, "\033[38;5;208m");
    vec_push_str(&ret, user);
    vec_push_ansi(&ret, "\033[0m");
    
    vec_push_str(&ret, " ");
    vec_push_ansi(&ret, "\033[90m");
    vec_push_str(&ret, "─");
    vec_push_ansi(&ret, "\033[0m");
    vec_push_str(&ret, " ");
    
    vec_push_ansi(&ret, "\033[36m");
    vec_push_str(&ret, short_cwd);
    vec_push_ansi(&ret, "\033[0m");
    
    if (branch)
    {
        vec_push_str(&ret, " ");
        vec_push_ansi(&ret, "\033[90m");
        vec_push_str(&ret, "─");
        vec_push_ansi(&ret, "\033[0m");
        vec_push_str(&ret, " ");
        
        if (branch_dirty)
            vec_push_ansi(&ret, "\033[35m");
        else
            vec_push_ansi(&ret, "\033[37m");
        vec_push_str(&ret, branch);
        vec_push_ansi(&ret, "\033[0m");
    }
    
    if (venv)
    {
        vec_push_str(&ret, " ");
        vec_push_ansi(&ret, "\033[34m");
        vec_push_str(&ret, "(");
        vec_push_str(&ret, venv);
        vec_push_str(&ret, ")");
        vec_push_ansi(&ret, "\033[0m");
    }
    
    /* Right-align time */
    time_width = measure_width("⏳") + ft_strlen(time_buf);
    pad = cols - left_width - time_width;
    
    if (pad < 1)
        pad = 1;
    
    while (pad-- > 0)
    {
        char sp = ' ';
        vec_push(&ret, &sp);
    }
    
    vec_push_ansi(&ret, "\033[2m");
    vec_push_str(&ret, "⏳");
    vec_push_str(&ret, time_buf);
    vec_push_ansi(&ret, "\033[0m");
    
    /* Line 2 - THE CRITICAL LINE FOR CURSOR POSITIONING
     * 
     * Readline calculates cursor position based on the LAST line of the prompt only.
     * The previous line doesn't matter for cursor calculation.
     * 
     * We MUST ensure that what we tell readline matches what's actually displayed.
     * 
     * DIAGNOSIS: If "f     d" appears (with spaces), it means readline thinks
     * the prompt is SHORTER than it actually is, so it's compensating by adding spaces.
     */
    
    vec_push_str(&ret, "\n");
    /* Use a plain ASCII prompt symbol without control markers so it prints
       correctly even in environments where readline markers are not handled. */
    vec_push_str(&ret, "$ ");

    /* Cleanup */
    free(short_cwd);
    if (branch)
        free(branch);
    if (venv)
        free(venv);
    
    return ret;
}