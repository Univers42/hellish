/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:32:22 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:32:22 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HISTORY_H
# define HISTORY_H

# include "shell.h"

typedef struct s_history
{
	bool		hist_active;
	int			append_fd;
	t_vec		hist_cmds;
}	t_history;

void		manage_history(t_shell *state);
void		init_history(t_shell *state);
void		free_hist(t_shell *state);
void		parse_history_file(t_shell *state);
t_string	encode_cmd_hist(char *cmd);
char		*get_hist_file_path(t_shell *state);
t_string	parse_single_cmd(t_string hist, size_t *cur);
t_vec		parse_hist_file(t_string hist);
void		parse_history_file(t_shell *state);
t_string	encode_cmd_hist(char *cmd);
void		manage_history(t_shell *state);
bool		worthy_of_being_remembered(t_shell *state);

#endif