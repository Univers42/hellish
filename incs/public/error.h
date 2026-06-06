/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:04:40 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/21 00:04:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Error reporting helpers.  "critical" means exit(1) immediately;
   "warning" prints but continues.  The _errno variants pull in strerror
   so the caller doesn't have to.  All output goes to stderr (ft_eprintf).
   Inlined to keep the call overhead zero for what are essentially
   macros with type safety. */

#ifndef ERROR_H
# define ERROR_H

# include "libft.h"
# include <errno.h>

/* Print message and exit(1).  For OOM and other unrecoverable failures. */
static inline void	critical_error(char *error)
{
	ft_eprintf("[ERROR] %s\n", error);
	exit(1);
}

/* Like critical_error but appends strerror(errno) -- use after syscalls. */
static inline void	critical_error_errno(void)
{
	ft_eprintf("[ERROR] %s\n", strerror(errno));
	exit(1);
}

/* critical_error_errno with a context prefix (e.g. the failing filename). */
static inline void	critical_error_errno_ctx(char *ctx)
{
	ft_eprintf("[ERROR] %s: %s\n", ctx, strerror(errno));
	exit(1);
}

/* Non-fatal warning: print and return.  Shell keeps running. */
static inline void	warning_error(char *error)
{
	ft_eprintf("[WARNING] %s\n", error);
}

/* Non-fatal warning with strerror. */
static inline void	warning_error_errno(void)
{
	ft_eprintf("[WARNING] %s\n", strerror(errno));
}

typedef struct s_shell	t_shell;

void	warning_error_errno(void);
void	err_1_errno(t_shell *state, char *p1);
void	err_2(t_shell *state, char *p1, char *p2);

#endif
