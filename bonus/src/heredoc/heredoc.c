/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:00 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft/dsa/dyn_str.h"
#include "../libft/libft.h"
#include "../shell.h"
#include <fcntl.h>
#include <readline/readline.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../libft/libft.h"

// returns writable fd
int	ft_mktemp(t_state *state, t_ast_node *node)
{
	t_redir		ret;
	char		*temp;
	int			wr_fd;
	t_string	fname;

	ret = (t_redir){.direction_in = true, .should_delete = true};
	vec_init(&fname);
	vec_push_str(&fname, "/tmp/heredoc_");
	if (state->pid)
		vec_push_str(&fname, state->pid);
	vec_push_str(&fname, "_");
	temp = ft_itoa(state->heredoc_idx++);
	vec_push_str(&fname, temp);
	ret.fname = (char *)fname.ctx;
	free(temp);
	wr_fd = open(ret.fname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (wr_fd < 0)
		critical_error_errno_context(ret.fname);
	ret.fd = open(ret.fname, O_RDONLY);
	if (ret.fd < 0)
		critical_error_errno_context(ret.fname);
	vec_push(&state->redirects, &ret);
	node->redir_idx = state->redirects.len - 1;
	node->has_redirect = true;
	return (wr_fd);
}

char	*first_non_tab(char *line)
{
	while (*line == '\t')
		line++;
	return (line);
}

void	gather_heredoc(t_state *state, t_ast_node *node)
{
	int				wr_fd;
	t_string		sep;
	t_heredoc_req	req;

	ft_assert(node->children.len >= 1);
	if (((t_ast_node *)node->children.ctx)[0].token.tt == TT_HEREDOC)
	{
		wr_fd = ft_mktemp(state, node);
		sep = word_to_hrdoc_string(((t_ast_node *)node->children.ctx)[1]);
		ft_assert(sep.ctx != 0);
		req = (t_heredoc_req){
			.sep = (char *)sep.ctx,
			.expand = !contains_quotes(((t_ast_node *)node->children.ctx)[1]),
			.remove_tabs
			= ft_strncmp(((t_ast_node *)node->children.ctx)[0].token.start, "<<-", 3)
			== 0};
		write_heredoc(state, wr_fd, &req);
		free(sep.ctx);
	}
}

int	gather_heredocs(t_state *state, t_ast_node *node)
{
	size_t	i;

	i = 0;
	while (i < node->children.len && !g_should_unwind)
	{
		gather_heredocs(state, &((t_ast_node *)node->children.ctx)[i]);
		i++;
	}
	if (node->node_type == AST_REDIRECT)
	{
		gather_heredoc(state, node);
	}
	return (0);
}
