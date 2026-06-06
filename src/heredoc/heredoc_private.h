/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc_private.h                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:31:36 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:11:17 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HEREDOC_PRIVATE_H
# define HEREDOC_PRIVATE_H

# include "libft.h"
# include "shell.h"
# include "redir.h"
# include "env.h"
# include "expander.h"
# include <unistd.h>
# include "helpers.h"
# include <sys/wait.h>
# include <readline/readline.h>
# include <fcntl.h>

/* Spec for one << operator found during the pre-scan in collect_specs.
   delim is the literal delimiter string (quotes stripped).  dash is true
   for <<- (strip leading tabs).  line is the source line number where the
   << operator appeared so advance_hd can match bodies to operators. */
typedef struct s_hd
{
	char	*delim;
	bool	dash;
	size_t	line;
}	t_hd;

/* Iteration context for walk_and_strip / advance_hd.  sp/n is the full
   spec array; si is the index of the next spec to process; got counts
   how many bodies were actually extracted (some specs may lack a matching
   delimiter in the string and are skipped). */
typedef struct s_walk_ctx
{
	t_hd	*sp;
	int		n;
	int		si;
	int		got;
}	t_walk_ctx;

int		ft_mktemp(t_shell *state, t_ast_node *node);
char	*first_non_tab(char *line);
bool	get_line_heredoc(t_shell *state,
			t_hdoc *req, t_string *alloc_line);
void	gather_heredoc(t_shell *state, t_ast_node *node, bool is_pipe);
void	process_redirect_group(t_shell *state, t_ast_node *parent,
			size_t start, size_t end);
int		gather_heredocs(t_shell *state, t_ast_node *node, bool in_pipeline);
void	expand_dolar(t_shell *state, int *i, t_string *full_file, char *line);
void	expand_braced(t_shell *state, int *i, t_string *full_file, char *line);
void	expand_bs(int *i, t_string *full_file, char *line);
void	expand_line(t_shell *state, t_string *full_file, char *line);
bool	is_sep(t_hdoc *req, t_string *alloc_line);
void	process_line(t_shell *state, t_hdoc *req);
void	write_heredoc(t_shell *state, int wr_fd, t_hdoc *req);
bool	contains_quotes(t_ast_node node);
bool	is_delim_line(const char *line, size_t len, t_hd *s);
bool	split_heredocs(const char *str, char **stripped, char **bodies);

#endif