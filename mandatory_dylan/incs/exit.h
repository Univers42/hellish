/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 21:44:13 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 21:44:13 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXIT_H
# define EXIT_H

# include "common.h"
/* ensure printing prototypes are visible */
# include "ft_printf/ft_printf.h"

# define COMMAND_NOT_FOUND	127
# define EXE_PERM_DENIED	126
# define EXE_BASE_ERROR		125
# define CANCELED			130
# define SYNTAX_ERR			2
# define AMBIGUOUS_REDIRECT	1

# include "dsa.h"
# include "parser.h"


void	err_cmd_not_found(const char *context, const char *cmd);
void	err_no_path_var(const char *context, const char *cmd);
void	err_1_errno(const char *context, const char *p1);
void	err_2(const char *context, const char *p1, const char *p2);
void	unexpected_simple(const char *context, const char *tok, int tok_len);
void	critical_error(const char *error);
void	critical_error_errno(void);
void	critica_error_errno_context(const char *context);
void	warning_error(const char *error);
void	warning_error_errno(void);


# endif