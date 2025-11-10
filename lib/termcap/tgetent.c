/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tgetent.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 23:36:31 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/11 00:40:57 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"

/* Static helpers (definitions restored) */
static char *handle_env_direct(char *name, char *bp, t_tgetent_ctx *ctx)
{
    char *termcap_name;
    char *term_env;

    termcap_name = getenv("TERMCAP");
    if (termcap_name && *termcap_name == '\0')
        termcap_name = NULL;
    if (!termcap_name || *termcap_name == '/')
        return (termcap_name);
    term_env = getenv("TERM");
    if (!term_env || strcmp(name, term_env) != 0)
        return (termcap_name);
    ctx->indirect = tgetst1(find_capability(termcap_name, "tc"), NULL);
    if (!ctx->indirect)
    {
        if (bp)
            strcpy(bp, termcap_name);
        else
        {
            ctx->malloc_size = (int)(strlen(termcap_name) + 1);
            ctx->bp = (char *)xmalloc(ctx->malloc_size);
            strcpy(ctx->bp, termcap_name);
        }
        /* Mark that we satisfied from env directly (no file read) */
        ctx->tcenv = termcap_name;
        return (NULL);
    }
    /* tc= indirection present: remember env string for initial copy */
    ctx->tcenv = termcap_name;
    return (NULL);
}

static void copy_line_strip_backslash(char **bp1, char **src)
{
    int c;

    while ((*(*bp1)++ = c = *(*src)++) && c != '\n')
    {
        if (c == '\\' && **src == '\n')
        {
            (*bp1)--;
            (*src)++;
        }
    }
    *(*bp1) = '\0';
}

static int process_entries(t_tgetent_ctx *ctx, char *name, t_buffer *buf, int fd)
{
    char *term;
    char *bp2;
    char *tmp;

    term = ctx->indirect ? ctx->indirect : name;
    while (term)
    {
        if (scan_file(term, fd, buf) == 0)
            return (0);
        if (term != name)
            free(term);
        if (ctx->malloc_size)
        {
            ctx->malloc_size = ctx->bp1 - ctx->bp + buf->size;
            tmp = (char *)xrealloc(ctx->bp, ctx->malloc_size);
            ctx->bp1 += tmp - ctx->bp;
            ctx->bp = tmp;
        }
        bp2 = ctx->bp1;
        tmp = buf->ptr;
        copy_line_strip_backslash(&ctx->bp1, &tmp);
        term = tgetst1(find_capability(bp2, "tc"), NULL);
    }
    return (1);
}

static int init_buffers(t_tgetent_ctx *ctx, t_buffer *buf)
{
    if (!ctx->bp)
    {
        ctx->malloc_size = ctx->indirect ? (int)(strlen(ctx->tcenv) + 1) : buf->size;
        ctx->bp = (char *)xmalloc(ctx->malloc_size);
    }
    ctx->bp1 = ctx->bp;
    if (ctx->indirect)
    {
        strcpy(ctx->bp, ctx->tcenv);
        ctx->bp1 += strlen(ctx->tcenv);
    }
    return (1);
}

static char *fallback_entry(const char *name)
{
    if (!name)
        return (NULL);
    if (strcmp(name, "xterm") == 0 || strstr(name, "xterm-256color"))
        return ("xterm|xterm-256color:am:bs:co#80:li#24:cl=\\E[H\\E[2J:ce=\\E[K:cd=\\E[J:cm=\\E[%i%2;%2H:nd=\\E[C:up=\\E[A:le=\\E[D:ho=\\E[H:so=\\E[7m:se=\\E[27m:md=\\E[1m:me=\\E[0m:us=\\E[4m:ue=\\E[24m:");
    if (strcmp(name, "vt100") == 0)
        return ("vt100|vt100:am:bs:co#80:li#24:cl=\\E[H\\E[2J:cm=\\E[%i%2;%2H:nd=\\E[C:up=\\E[A:le=\\E[D:ho=\\E[H:");
    return (NULL);
}

static int apply_fallback(char *bp, const char *name)
{
    char *fb;
    char *dst;
    size_t len;

    fb = fallback_entry(name);
    if (!fb)
        return (0);
    len = strlen(fb);
    if (!bp)
    {
        dst = (char *)xmalloc(len + 1);
        strcpy(dst, fb);
        access_glob()->term_entry = dst;
    }
    else
    {
        strncpy(bp, fb, len);
        bp[len] = '\0';
        access_glob()->term_entry = bp;
    }
    termcap_mark_fallback();
    return (1);
}

int tgetent(char *bp, const char *name)
{
    t_tgetent_ctx ctx;
    t_buffer buf;
    char *termcap_file;
    int fd;

    if (bp)
        access_glob()->term_entry = bp;
    ctx = (t_tgetent_ctx){bp, NULL, 0, NULL, NULL};
    termcap_file = handle_env_direct((char *)name, bp, &ctx);
    /* If TERMCAP provided inline entry with no tc=, return it now */
    if (!termcap_file && ctx.tcenv && !ctx.indirect && (bp || ctx.bp))
    {
        access_glob()->term_entry = ctx.bp ? ctx.bp : bp;
        return (1);
    }
    /* Otherwise, read database file */
    if (!termcap_file)
        termcap_file = "/etc/termcap";
    fd = open(termcap_file, O_RDONLY, 0);
    if (fd < 0)
    {
        free(ctx.indirect);
        if (apply_fallback(bp, name))
            return (1);
        return (-1);
    }
    buf.size = 2048;
    buf.beg = (char *)xmalloc(buf.size + 1);
    buf.ptr = buf.beg;
    buf.ateof = 0;
    buf.full = 0;
    init_buffers(&ctx, &buf);
    if (!process_entries(&ctx, (char *)name, &buf, fd))
    {
        close(fd);
        free(buf.beg);
        if (ctx.malloc_size)
            free(ctx.bp);
        if (apply_fallback(bp, name))
            return (1);
        return (0);
    }
    close(fd);
    free(buf.beg);
    if (ctx.malloc_size)
        ctx.bp = (char *)xrealloc(ctx.bp, ctx.bp1 - ctx.bp + 1);
    access_glob()->term_entry = ctx.bp ? ctx.bp : bp;
    return (1);
}