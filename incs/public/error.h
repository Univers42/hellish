/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:04:40 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/21 00:04:40 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ERROR_H
# define ERROR_H

# include "ft_printf.h"
# include <errno.h>

static inline void	critical_error(char *error)
{
	ft_eprintf("[ERROR] %s\n", error);
	exit(1);
}

static inline void	critical_error_errno(void)
{
	ft_eprintf("[ERROR] %s\n", strerror(errno));
	exit(1);
}

static inline void	critical_error_errno_context(char *context)
{
	ft_eprintf("[ERROR] %s: %s\n", context, strerror(errno));
	exit(1);
}

static inline void	warning_error(char *error)
{
	ft_eprintf("[WARNING] %s\n", error);
}

static inline void	warning_error_errno(void)
{
	ft_eprintf("[WARNING] %s\n", strerror(errno));
}

#endif

#ifndef PUBLIC_ERROR_H
# define PUBLIC_ERROR_H

typedef struct s_shell t_shell;

void	critical_error_errno_context(const char *msg);
void	warning_error_errno(void);
void	warning_error(const char *msg);
void	err_1_errno(t_shell *state, char *p1);
void	err_2(t_shell *state, char *p1, char *p2);

#endif