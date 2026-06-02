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
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>

/* Running state for one printf invocation (kept in one struct to stay within
   the argument-count limit while threading output + the arg cursor around). */
typedef struct s_pf
{
	t_string	*out;
	char		**av;
	int			argc;
	int			argi;
	bool		used;
	bool		stop;
}	t_pf;

char		pf_escape(const char *s, int *i, bool *stop);
long long	pf_to_num(const char *arg);
void		pf_emit_b(t_string *out, const char *arg, bool *stop);
void		pf_conv(t_pf *pf, const char *spec, int speclen, char conv);
void		pf_scan_spec(const char *fmt, int *i);

#endif
