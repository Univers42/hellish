/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc_private.h                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:31:36 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 15:15:08 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HEREDOC_PRIVATE_H
# define HEREDOC_PRIVATE_H

# include "libft.h"
# include "shell.h"
# include "redir.h"
# include "env.h"
# include <unistd.h>
# include "redir.h"
# include "helpers.h"
# include <sys/wait.h>
# include <readline/readline.h>
# include <fcntl.h>

int		ft_mktemp(t_shell *state, t_ast_node *node);
char	*first_non_tab(char *line);
bool	get_line_heredoc(t_shell *state,
			t_heredoc_req *req, t_string *alloc_line);
void	gather_heredoc(t_shell *state, t_ast_node *node, bool is_pipe);
void	process_redirect_group(t_shell *state, t_ast_node *parent,
			size_t start, size_t end);
int		gather_heredocs(t_shell *state, t_ast_node *node, bool in_pipeline);
void	expand_dolar(t_shell *state, int *i, t_string *full_file, char *line);
void	expand_bs(int *i, t_string *full_file, char *line);
void	expand_line(t_shell *state, t_string *full_file, char *line);
bool	is_sep(t_heredoc_req *req, t_string *alloc_line);
void	process_line(t_shell *state, t_heredoc_req *req);
void	write_heredoc(t_shell *state, int wr_fd, t_heredoc_req *req);
bool	contains_quotes(t_ast_node node);

#endif