/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   termcap.h                                          :+:      :+:    :+:   */
/*                                                                            */
/* ************************************************************************** */

#ifndef TERMCAP_H
#define TERMCAP_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct s_buffer
{
	char *beg;
	int size;
	char *ptr;
	int ateof;
	int full;
} t_buffer;

typedef struct s_tgetent_ctx
{
	char *bp;
	char *bp1;
	int malloc_size;
	char *indirect;
	char *tcenv;
} t_tgetent_ctx;

typedef struct s_tglob
{
	char pc;
	short ospeed;
	int *speeds;
	char *up;
	char *bc;
	int tputs_baud_rate;
	char *esctab;
	char *term_entry;
} t_tglob;

t_tglob *access_glob(void);
int tgetent(char *buffer, const char *termtype);
int tgetnum(const char *name);
int tgetflag(const char *name);
char *tgetstr(const char *name, char **area);
int tputs(const char *string, int nlines, int (*outfn)(int));
char *tparam(const char *fmt, char *out, int size,
			 int a0, int a1, int a2, int a3);
char *tgoto(const char *cm, int hpos, int vpos);
char *gobble_line(int fd, t_buffer *bufp, char *append_end);
int scan_file(char *str, int fd, t_buffer *bufp);
char *find_capability(char *bp, char *cap);
char *tgetst1(char *ptr, char **area);
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
int tc_atoi(const char *s);
int termcap_used_fallback(void);
void termcap_mark_fallback(void);

#endif