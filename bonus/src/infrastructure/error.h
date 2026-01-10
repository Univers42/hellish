/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:12:28 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:12:28 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ERROR_H
# define ERROR_H
# include "common.h"

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

void	err_cmd_not_found(t_state *state, char *cmd);
void	err_no_path_var(t_state *state, char *cmd);
void	err_1_errno(t_state *state, char *p1);
void	err_2(t_state *state, char *p1, char *p2);
t_ast_node	unexpected(t_state *state, t_parser *parser,
	t_ast_node ret, t_deque_tt *tokens);


#endif