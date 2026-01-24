/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_private.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:21:39 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 01:24:43 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RL_PRIVATE_H
# define RL_PRIVATE_H

# include "shell.h"
# include <readline/readline.h>
# include <stdbool.h>
# include <sys/wait.h>
# include <unistd.h>
# include "sh_input.h"
# include <stdio.h>
# include <wchar.h>
# include "helpers.h"

int		return_last_line(t_shell *state, t_string *ret);
int		return_new_line(t_shell *state, t_string *ret);
int		buff_readline(t_shell *state, t_string *ret, char *prompt);
void	buff_readline_update(t_buff_readline *l);
void	buff_readline_reset(t_buff_readline *l);
void	buff_readline_init(t_buff_readline *ret);
void	update_context(t_shell *state);
int		get_more_input_notty(t_shell *state);
int		visible_width_cstr(const char *s);

#endif