#include "parser.h"

t_ast	*new_ast_node(t_node_type type)
{
	t_ast	*node;

	node = malloc(sizeof(t_ast));
	if (!node)
		return (NULL);
	node->type = type;
	node->cmd = NULL;
	node->left = NULL;
	node->right = NULL;
	return (node);
}

t_redir	*new_redir_node(t_tok_type type, char *target)
{
	t_redir	*redir;

	redir = malloc(sizeof(t_redir));
	if (!redir)
		return (NULL);
	redir->type = type;
	redir->target = target;
	redir->next = NULL;
	return (redir);
}

t_cmd	*new_cmd_node(void)
{
	t_cmd	*cmd;

	cmd = malloc(sizeof(t_cmd));
	if (!cmd)
		return (NULL);
	cmd->args = NULL;
	cmd->redir = NULL;
	return (cmd);
}

void	add_redir_back(t_redir **redir, t_redir *new)
{
	t_redir	*curr;

	if (!*redir)
	{
		*redir = new;
		return ;
	}
	curr = *redir;
	while (curr->next)
		curr = curr->next;
	curr->next = new;
}
