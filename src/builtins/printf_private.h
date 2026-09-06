/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printf_private.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PRINTF_PRIVATE_H
# define PRINTF_PRIVATE_H

# include "builtins_private.h"
# include <errno.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>

/* Running state for one printf invocation (kept in one struct to stay within
   the argument-count limit while threading output + the arg cursor around).
   err becomes the builtin's exit status: bash keeps converting after a bad
   numeric argument but still exits 1 at the end. */
typedef struct s_pf
{
	t_string	*out;
	char		**av;
	int			argc;
	int			argi;
	bool		used;
	bool		stop;
	int			err;
	char		*ctx;
	char		*vname;
	t_shell		*state;
}	t_pf;

/* One parsed %-directive, conversion character excluded. Width/precision are
   stored numerically because '*' sources them from positional arguments; the
   spec string handed to snprintf is rebuilt from these fields. */
/* The stack render buffer, and the ceiling on a field width. The width cap
   is a sanity bound against `printf "%99999999999999s"`, not a correctness
   limit -- anything under it is rendered in full, on the heap if needed. */
# define PF_STACK_BUF 4096
# define PF_WIDTH_MAX 268435456

/* Render target for one conversion: where to write and how much room there
   is. Bundled because the 42 norm caps a function at four arguments and
   pf_conv_str already needs the shell, the spec string and the argument. */
typedef struct s_pfbuf
{
	char	*p;
	size_t	cap;
}	t_pfbuf;

typedef struct s_spec
{
	char		flags[8];
	long long	width;
	long long	prec;
	bool		has_width;
	bool		has_prec;
}	t_spec;

const char	*pf_arg(t_pf *pf);
size_t		pf_render_size(t_spec *sp, size_t arglen);
void		pf_buf_open(t_spec *sp, t_pfbuf *b, char *stack, size_t arglen);
void		pf_buf_close(t_pfbuf *b, char *stack);
void		pf_emit_b_padded(t_pf *pf, t_spec *sp, const char *arg);
void		pf_emit_sized(t_pf *pf, t_spec *sp, char *fmt, const char *arg);
int			pf_conv_str(t_pf *pf, char *fmt, const char *arg, t_pfbuf *b);
/* printf renders unsigned conversions through the full 64-bit range;
   spelled as a typedef so the declarations below keep one alignment
   column (`unsigned long long` is too wide for it). */
typedef unsigned long long	t_ull;

void		pf_err_num(t_pf *pf, const char *arg);
bool		pf_conv_time(t_pf *pf, t_spec *sp, const char *fmt, int *i);
char		pf_escape(const char *s, int *i, bool *stop);
char		*pf_quote(const char *arg);
void		pq_ansi(t_string *out, const char *s);
void		pq_ansi_char(t_string *out, unsigned char c);
void		pf_conv_quote(t_pf *pf, t_spec *sp, const char *arg);
long long	pf_num(t_pf *pf, const char *arg);
void		pf_conv_float(t_pf *pf, char *fmt, const char *arg,
				t_pfbuf *b);
void		pf_emit_b(t_string *out, const char *arg, bool *stop);
t_ull		pf_unum(t_pf *pf, const char *arg);
void		pf_conv(t_pf *pf, t_spec *sp, char conv);
void		pf_parse_spec(t_pf *pf, const char *fmt, int *i, t_spec *sp);
void		pf_build_spec(char *dst, t_spec *sp, char conv);
int			pf_fmt_index(t_vec argv, char **vname);

#endif
