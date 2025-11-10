/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_termcap.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/11 00:50:02 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/11 00:50:03 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define C_OK "\033[32m"
#define C_FAIL "\033[31m"
#define C_WARN "\033[33m"
#define C_INFO "\033[36m"
#define C_RST "\033[0m"

static int g_use_color = 1;

static void putc_wrap(char c) { write(1, &c, 1); }
static int out_int(int c)
{
    putc_wrap((char)c);
    return c;
}

static void print_bytes(const char *s)
{
    const unsigned char *p;
    if (!s)
    {
        printf("(null)");
        return;
    }
    p = (const unsigned char *)s;
    while (*p)
    {
        if (*p >= 32 && *p < 127)
            putchar(*p);
        else
            printf("\\x%02X", *p);
        p++;
    }
}

static void show_header(const char *title)
{
    printf("\n%s== %s ==%s\n", g_use_color ? C_INFO : "", title, g_use_color ? C_RST : "");
}

static void show_term_entry(void)
{
    t_tglob *g;
    size_t len;
    char *e;

    g = access_glob();
    e = g->term_entry;
    show_header("Raw termcap entry");
    if (!e)
    {
        printf("%s(no term_entry set)%s\n", g_use_color ? C_FAIL : "", g_use_color ? C_RST : "");
        return;
    }
    len = strlen(e);
    printf("Length: %zu bytes\n", len);
    printf("Raw: ");
    print_bytes(e);
    putchar('\n');
    if (len < 20)
        printf("%s[WARN]%s entry is very short -> database likely missing (/etc/termcap?)\n",
               g_use_color ? C_WARN : "", g_use_color ? C_RST : "");
}

static void parse_and_list(void)
{
    t_tglob *g;
    char *e;
    char *cur;
    int n_bool = 0;
    int n_num = 0;
    int n_str = 0;

    g = access_glob();
    e = g->term_entry;
    show_header("Parsed capabilities");
    if (!e)
    {
        printf("none\n");
        return;
    }
    cur = e;
    while (*cur)
    {
        if (*cur == ':')
        {
            cur++;
            continue;
        }
        /* collect token */
        {
            char token[256];
            int i = 0;
            while (*cur && *cur != ':' && *cur != '\n' && i < 255)
                token[i++] = *cur++;
            token[i] = '\0';
            if (i == 0)
                continue;
            /* classify */
            if (strchr(token, '#'))
            {
                n_num++;
                printf("NUM  %-12s value(raw)=", token);
                print_bytes(strchr(token, '#') + 1);
                putchar('\n');
            }
            else if (strchr(token, '=') || strchr(token, '~'))
            {
                char *v = strchr(token, '=');
                if (!v)
                    v = strchr(token, '~');
                *v = '\0';
                n_str++;
                printf("STR  %-12s =", token);
                print_bytes(v + 1);
                putchar('\n');
            }
            else
            {
                n_bool++;
                printf("BOOL %-12s = true\n", token);
            }
        }
    }
    printf("Totals: bool=%d num=%d str=%d\n", n_bool, n_num, n_str);
}

void test_cap(const char *cap)
{
    int num;
    int flag;
    char *str;
    char buf[512];
    char *area = buf;

    num = tgetnum(cap);
    flag = tgetflag(cap);
    str = tgetstr(cap, &area);
    printf("%-4s | num=", cap);
    if (num >= 0)
        printf("%s%d%s", g_use_color ? C_OK : "", num, g_use_color ? C_RST : "");
    else
        printf("%s-1%s", g_use_color ? C_WARN : "", g_use_color ? C_RST : "");
    printf(" flag=%s%s%s", g_use_color ? (flag ? C_OK : C_WARN) : "",
           flag ? "1" : "0", g_use_color ? C_RST : "");
    printf(" str=");
    if (str)
    {
        print_bytes(str);
    }
    else
        printf("%sNULL%s", g_use_color ? C_FAIL : "", g_use_color ? C_RST : "");
    putchar('\n');
}

static void report_cap(const char *cap)
{
    t_tglob *g;
    char *p;

    g = access_glob();
    if (!g->term_entry)
    {
        printf("%s: missing (no term entry)\n", cap);
        return;
    }
    p = find_capability(g->term_entry, (char *)cap);
    if (!p)
    {
        printf("%s: missing\n", cap);
        return;
    }
    if (p[-1] == '#')
    {
        int v = tc_atoi(p);
        printf("%s: num=%d\n", cap, v);
    }
    else if (p[-1] == ':')
    {
        printf("%s: flag=true\n", cap);
    }
    else if (p[-1] == '=' || p[-1] == '~')
    {
        char *s;

        s = tgetstr(cap, NULL);
        printf("%s: str=", cap);
        print_bytes(s);
        putchar('\n');
        free(s);
    }
    else
    {
        printf("%s: unknown type\n", cap);
    }
}

static void test_common_caps(void)
{
    const char *caps[] = {
        "co", "li", "sg",
        "am", "xn", "bs",
        "cl", "ce", "cd", "cm", "up", "nd", "le", "ho",
        NULL};
    int i = 0;

    show_header("Capability checks");
    while (caps[i])
        report_cap(caps[i++]);
}

static void test_tputs_demo(void)
{
    char *area;
    char buf[512];
    char *cl;
    char *md;
    char *me;

    area = buf;
    show_header("tputs demo");
    cl = tgetstr("cl", &area);
    md = tgetstr("md", &area);
    me = tgetstr("me", &area);
    if (cl)
    {
        printf("Clearing screen... (cl)\n");
        tputs(cl, 1, out_int);
    }
    else
        printf("cl missing\n");
    if (md && me)
    {
        tputs(md, 1, out_int);
        printf("Bold text via termcap\n");
        tputs(me, 1, out_int);
        putchar('\n');
    }
    else
        printf("md/me missing\n");
}

static void test_cursor_move(void)
{
    char *area;
    char buf[256];
    char *cm;
    char *seq;
    int row;
    int col;

    area = buf;
    row = 5;
    col = 10;
    show_header("Cursor movement");
    cm = tgetstr("cm", &area);
    if (!cm)
    {
        printf("cm missing\n");
        return;
    }
    printf("Raw cm: ");
    print_bytes(cm);
    putchar('\n');
    /* API: tgoto(cm, hpos(col), vpos(row)) */
    seq = tgoto(cm, col, row);
    if (seq)
    {
        printf("tgoto(hpos=%d, vpos=%d): ", col, row);
        print_bytes(seq);
        putchar('\n');
        printf("Expected ESC[%02d;%02dH (row+1=%02d col+1=%02d)\n",
               row + 1, col + 1, row + 1, col + 1);
    }
    else
        printf("tgoto failed\n");
}

static void explain_missing(void)
{
    t_tglob *g = access_glob();
    if (!g->term_entry || strlen(g->term_entry) < 20)
    {
        show_header("Diagnosis");
        printf("%sTermcap entry extremely short. Likely causes:%s\n",
               g_use_color ? C_WARN : "", g_use_color ? C_RST : "");
        printf("- /etc/termcap not found or empty\n");
        printf("- Using terminfo system only (modern Linux distros)\n");
        printf("- TERM variable (%s) has no termcap definition\n",
               getenv("TERM") ? getenv("TERM") : "(unset)");
        printf("Suggestions:\n");
        printf("1) Install ncurses-term or termcap package\n");
        printf("2) Provide a fallback hardcoded entry\n");
    }
}

static void test_tgetent(const char *term)
{
    char buf[4096];
    int r;

    show_header("tgetent");
    printf("TERM='%s'\n", term);
    r = tgetent(buf, term);
    printf("tgetent return=%d\n", r);
    if (r != 1)
    {
        printf("%sFailure loading termcap entry%s\n",
               g_use_color ? C_FAIL : "", g_use_color ? C_RST : "");
    }
}

static void show_fallback_note(void)
{
    if (termcap_used_fallback())
        printf("%s[FALLBACK]%s builtin termcap entry used (no /etc/termcap)\n",
               g_use_color ? C_WARN : "", g_use_color ? C_RST : "");
}

int main(int argc, char **argv)
{
    const char *term = getenv("TERM");

    if (argc > 1)
        term = argv[1];
    if (!term)
    {
        fprintf(stderr, "Usage: %s [TERM]\n", argv[0]);
        return (1);
    }
    if (getenv("NO_COLOR"))
        g_use_color = 0;
    test_tgetent(term);
    show_fallback_note();
    show_term_entry();
    parse_and_list();
    test_common_caps();
    test_tputs_demo();
    test_cursor_move();
    explain_missing();
    printf("\nDone.\n");
    return (0);
}
