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
typedef struct s_shell t_shell;
typedef struct s_ast_node t_ast_node;

# ifndef REDIR_TYPE_DEFINED
#  define REDIR_TYPE_DEFINED
typedef struct redir_s
{
	bool		direction_in;
	int			fd;
	char		*fname;
	bool		should_delete;
}	t_redir;
# endif

// heredoc.c
typedef struct s_heredoc_req
{
	t_string	full_file;
	bool		finished;
	char		*sep;
	bool		expand;
	bool		remove_tabs;
}	t_heredoc_req;

int			gather_heredocs(t_shell *state, t_ast_node *node);
bool		contains_quotes(t_ast_node node);
void		write_heredoc(t_shell *state, int wr_fd, t_heredoc_req *req);
int			ft_mktemp(t_shell *state, t_ast_node *node);
char		*first_non_tab(char *line);
void		expand_line(t_shell *state, t_string *full_file, char *line);

#endif