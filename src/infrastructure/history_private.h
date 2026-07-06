/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_private.h                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:20:43 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 17:22:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Internal header for the history subsystem. File format: each command is one
   logical line terminated by a bare '\n'. Real newlines (from \-continuation or
   quoted newlines inside the command) are stored as '\' + '\n'. Backslashes are
   doubled. This escaping is symmetric: encode_cmd_hist writes, parse_single_cmd
   reads. See history.c and history2.c for the encode/decode pair. */
#ifndef HISTORY_PRIVATE_H
# define HISTORY_PRIVATE_H

/* Cap on history entries kept in memory and on disk (like bash HISTFILESIZE).
   Bounds startup load time and the per-line readline/fork footprint. */
# ifndef HIST_MAX
#  define HIST_MAX 2000
# endif

# include "shell.h"
# include <fcntl.h>
# include <stdio.h>
# include <readline/history.h>
# include <stdbool.h>
# include <unistd.h>
# include "helpers.h"
# include "env.h"

/* Scanner state for hist_join_line (history_join*.c): input cursor, the
   joined output, quote/substitution context, and the queue of pending
   here-doc tags (each stored with a leading '-' for <<- or '+' for <<). */
typedef struct s_hjoin
{
	const char	*s;
	size_t		i;
	t_string	out;
	t_vec		tags;
	bool		sq;
	bool		dq;
	bool		btick;
	bool		body;
	bool		cpat;
	int			csub;
	int			arith;
	int			pdepth;
	int			dpar;
}	t_hjoin;

char		*hist_join_line(const char *cmd);
void		hj_init(t_hjoin *h, const char *cmd);
bool		hj_at_heredoc(t_hjoin *h);
bool		hj_dollar(t_hjoin *h);
void		hj_pop_tag(t_hjoin *h);
void		hj_heredoc_tag(t_hjoin *h);
void		hj_heredoc_body(t_hjoin *h);
void		hj_copy_escaped(t_hjoin *h);
void		hj_depth_step(t_hjoin *h, int d);
bool		hj_last_word_kw(const char *s, size_t n);
char		*hj_finish(t_hjoin *h);

t_string	parse_single_cmd(t_string hist, size_t *cur);
t_vec		parse_hist_file(t_string hist);
void		add_history_line(const char *cmd);
void		parse_history_file(t_shell *state);
t_string	encode_cmd_hist(char *cmd);
void		manage_history(t_shell *state);
bool		worthy_of_being_remembered(t_shell *state);
void		init_history(t_shell *state);
void		free_hist(t_shell *state);
char		*get_hist_file_path(t_shell *state);
char		*hist_entry_at(t_shell *state, int idx);
char		*hist_last(t_shell *state);
char		*hist_search_prefix(t_shell *state, const char *prefix, int len);
char		*hist_search_contains(t_shell *state, const char *needle, int len);
char		*resolve_bang(t_shell *state, const char *s, size_t *adv);
int			in_sq(const char *s, size_t pos);
char		*replace_first(const char *s, const char *old, const char *nw);
char		*quick_sub(t_shell *state, const char *input);

#endif