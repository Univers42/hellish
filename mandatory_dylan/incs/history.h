/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 20:10:20 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 20:10:20 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HISTORY_H
# define HISTORY_H

# include "common.h"
# include "env.h"

# define HIST_FILE ".minishell_history"

typedef struct s_hist
{
	bool	active;
	int		append_fd;
	t_vec	hist_cmds;
}	t_hist;

t_string	parse_single_cmd(t_string hist, size_t *cur);
t_strings	parse_hist_file(t_string hist);
char		*get_hist_file_path(t_envs *_env);
void		parse_history_file(t_hist *_hist, t_envs *env);
t_string	encode_cmd_hist(char *cmd);
bool		worthy_of_being_remembered(t_hist *hist, t_rl *rl);
void		manage_history(t_hist *hist, t_rl *rl);
void		manage_history_input(t_hist *hist, t_rl *rl, t_string *input);
void		init_history(t_hist *hist, t_vec *env);
void		free_hist(t_hist *hist);

# endif