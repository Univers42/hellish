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
}	t_pf;

/* One parsed %-directive, conversion character excluded. Width/precision are
   stored numerically because '*' sources them from positional arguments; the
   spec string handed to snprintf is rebuilt from these fields. */
typedef struct s_spec
{
	char		flags[8];
	long long	width;
	long long	prec;
	bool		has_width;
	bool		has_prec;
}	t_spec;

const char	*pf_arg(t_pf *pf);
void		pf_err_num(t_pf *pf, const char *arg);
char		pf_escape(const char *s, int *i, bool *stop);
long long	pf_num(t_pf *pf, const char *arg);
void		pf_emit_b(t_string *out, const char *arg, bool *stop);
void		pf_conv(t_pf *pf, t_spec *sp, char conv);
void		pf_parse_spec(t_pf *pf, const char *fmt, int *i, t_spec *sp);
void		pf_build_spec(char *dst, t_spec *sp, char conv);

#endif
