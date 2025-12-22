#include "parser.h"

void	free_arr(char **arr)
{
	int	i;

	if (!arr || !*arr)
		return ;
	i = 0;
	while (arr[i])
	{
		free(arr[i]);
		i++;
	}
	free(arr);
}

void	free_redir(t_redir *redir)
{
	t_redir	*curr;
	t_redir	*next;

	if (!redir)
		return ;
	curr = redir;
	while (curr)
	{
		next = curr->next;
		free(curr->target);
		free(curr);
		curr = next;
	}
}

void	free_ast(t_ast *ast)
{
	if (!ast)
		return ;
	free_ast(ast->left);
	free_ast(ast->right);
	if (ast->cmd != NULL)
	{
		free_redir(ast->cmd->redir);
		free_arr(ast->cmd->args);
		free(ast->cmd);
	}
	free(ast);
}