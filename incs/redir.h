/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redir.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:42:42 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:42:42 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REDIR_H
# define REDIR_H

# include "alias.h"
# include <stdbool.h>

/* forward declarations to avoid including heavy headers */
typedef struct s_shell		t_shell;
typedef struct s_ast_node	t_ast_node;

# ifndef REDIR_TYPE_DEFINED
#  define REDIR_TYPE_DEFINED

typedef struct redir_s
{
	bool		direction_in;
	int			fd;
	int			src_fd;
	char		*fname;
	bool		should_delete;
	bool		close_fd;
	bool		is_dup;
}	t_redir;

# endif

// heredoc.c
typedef struct s_hdoc
{
	t_string	full_file;
	bool		finished;
	char		*sep;
	bool		expand;
	bool		remove_tabs;
	bool		is_pipe_heredoc;
}	t_hdoc;

int			gather_heredocs(t_shell *state, t_ast_node *node, bool in_pipeline);
bool		split_heredocs(const char *str, char **stripped, char **bodies);
bool		capture_heredoc_to_node(t_shell *state, t_ast_node *node);
bool		capture_heredoc_from_buff(t_shell *state, t_ast_node *node);
int			materialize_heredoc(t_shell *state, t_ast_node *node,
				int *redir_idx);
bool		contains_quotes(t_ast_node node);
void		write_heredoc(t_shell *state, int wr_fd, t_hdoc *req);
int			ft_mktemp(t_shell *state, t_ast_node *node);
char		*first_non_tab(char *line);
void		expand_line(t_shell *state, t_string *full_file, char *line);

static inline t_hdoc	create_heredoc(char *sep, bool expand,
								bool remove_tabs, bool is_pipe_heredoc)
{
	return ((t_hdoc)
		{
			.sep = sep,
			.expand = expand,
			.remove_tabs = remove_tabs,
			.is_pipe_heredoc = is_pipe_heredoc
		});
}

static inline t_redir	create_redir(bool direction_in, int src_fd,
								bool should_delete)
{
	return ((t_redir){
		.direction_in = direction_in,
		.fd = 0,
		.src_fd = src_fd,
		.fname = NULL,
		.should_delete = should_delete,
		.close_fd = false,
		.is_dup = false
	});
}
#endif