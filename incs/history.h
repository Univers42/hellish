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

/* hist_cmds is the truth; readline's own list is a mirror kept in step so
   the arrow keys agree with what `history` prints.

   appended / readmark exist only for `history -a` and `history -n`, which
   are defined against "what this session has already written / already
   read" rather than against the whole list. quiet_expand suppresses the
   echo expand_history() normally does, so `history -p` prints each result
   exactly once -- including one that expanded to itself. */
typedef struct s_history
{
	bool		hist_active;
	bool		quiet_expand;
	bool		recorded; /* this cycle's line is already in the list */
	int			append_fd;
	size_t		appended;
	size_t		readmark;
	t_vec		hist_cmds;
	char		*file; /* the file this session streams into */
}	t_history;

void		manage_history(t_shell *state);
/* Record this cycle's line BEFORE it runs, as bash does -- which is what
   makes `history` list itself. manage_history then only cleans up. */
void		history_record(t_shell *state, bool early);
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
void		add_history_line(t_shell *state, const char *cmd);

#endif