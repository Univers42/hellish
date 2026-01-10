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

typedef struct redir_s
{
	bool		direction_in;
	int			fd;
	char		*fname;
	bool		should_delete;
}	t_redir;

// heredoc.c
typedef struct s_heredoc_req
{
	t_string	full_file;
	bool		finished;
	char		*sep;
	bool		expand;
	bool		remove_tabs;
}	t_heredoc_req;

int			gather_heredocs(t_state *state, t_ast_node *node);
bool		contains_quotes(t_ast_node node);
void		write_heredoc(t_state *state, int wr_fd, t_heredoc_req *req);
int			ft_mktemp(t_state *state, t_ast_node *node);
char		*first_non_tab(char *line);
void		expand_line(t_state *state, t_string *full_file, char *line);
bool		is_special_char(char c);
bool		is_space(char c);

#endif