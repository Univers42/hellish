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

#include "shell.h"
#include <fcntl.h>
#include <readline/readline.h>
#include <sys/wait.h>
#include <unistd.h>
# include "redir.h"

// returns writable fd
int	ft_mktemp(t_shell *state, t_ast_node *node)
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

void	gather_heredoc(t_shell *state, t_ast_node *node, bool is_pipe)
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
			== 0,
			.is_pipe_heredoc = is_pipe
		};
		write_heredoc(state, wr_fd, &req);
		free(sep.ctx);
	}
}

static void process_redirect_group(t_shell *state, t_ast_node *parent, size_t start, size_t end, bool in_pipeline)
{
	size_t k;
	int shared_idx = -1;

	for (k = start; k < end; ++k)
	{
		t_ast_node *curr = &((t_ast_node *)parent->children.ctx)[k];
		if (curr->node_type != AST_REDIRECT)
			continue;
		t_token tt = ((t_ast_node *)curr->children.ctx)[0].token;
		if (tt.tt == TT_HEREDOC)
		{
			if (shared_idx == -1)
			{
				int wr = ft_mktemp(state, curr);
				if (wr >= 0)
				{
					shared_idx = curr->redir_idx;
					t_string sep = word_to_hrdoc_string(((t_ast_node *)curr->children.ctx)[1]);
					t_heredoc_req req = (t_heredoc_req){
						.sep = (char *)sep.ctx,
						.expand = !contains_quotes(((t_ast_node *)curr->children.ctx)[1]),
						.remove_tabs = ft_strncmp(((t_ast_node *)curr->children.ctx)[0].token.start, "<<-", 3) == 0,
						.is_pipe_heredoc = (in_pipeline || parent->node_type == AST_COMMAND_PIPELINE)
					};
					write_heredoc(state, wr, &req);
					free(sep.ctx);
				}
			}
			else
			{
				t_redir *r = (t_redir *)vec_idx(&state->redirects, shared_idx);
				if (!r || !r->fname)
					continue;
				curr->redir_idx = shared_idx;
				curr->has_redirect = true;
				int append_fd = open(r->fname, O_WRONLY | O_APPEND);
				if (append_fd < 0)
					critical_error_errno_context(r->fname);
				t_string sep = word_to_hrdoc_string(((t_ast_node *)curr->children.ctx)[1]);
				t_heredoc_req req = (t_heredoc_req){
					.sep = (char *)sep.ctx,
					.expand = !contains_quotes(((t_ast_node *)curr->children.ctx)[1]),
					.remove_tabs = ft_strncmp(((t_ast_node *)curr->children.ctx)[0].token.start, "<<-", 3) == 0,
					.is_pipe_heredoc = (in_pipeline || parent->node_type == AST_COMMAND_PIPELINE)
				};
				write_heredoc(state, append_fd, &req);
				free(sep.ctx);
			}
		}
		else
			gather_heredoc(state, curr, (in_pipeline || parent->node_type == AST_COMMAND_PIPELINE));
	}
}

int	gather_heredocs(t_shell *state, t_ast_node *node, bool in_pipeline)
{
	size_t	i;

	i = 0;
	while (i < node->children.len && !get_g_sig()->should_unwind)
	{
		t_ast_node *child = &((t_ast_node *)node->children.ctx)[i];
		if (child->node_type != AST_REDIRECT)
		{
			bool child_in_pipeline = (node->node_type == AST_COMMAND_PIPELINE);
			gather_heredocs(state, child, child_in_pipeline);
			i++;
			continue;
		}
		bool can_group = false;
		if (node->node_type == AST_SIMPLE_COMMAND)
			can_group = true;
		else if (node->node_type == AST_COMMAND && node->children.len > 0)
		{
			t_ast_node *first = &((t_ast_node *)node->children.ctx)[0];
			if (first->node_type == AST_SIMPLE_COMMAND)
				can_group = true;
		}
		if (!can_group)
		{
			gather_heredoc(state, child, (in_pipeline || node->node_type == AST_COMMAND_PIPELINE));
			i++;
			continue;
		}
		size_t j = i;
		while (j < node->children.len && ((t_ast_node *)node->children.ctx)[j].node_type == AST_REDIRECT)
			j++;
		process_redirect_group(state, node, i, j, in_pipeline || node->node_type == AST_COMMAND_PIPELINE);
		i = j;
	}
	if (node->node_type == AST_REDIRECT)
	{
		gather_heredoc(state, node, (in_pipeline || node->node_type == AST_COMMAND_PIPELINE));
	}
	return (0);
}
