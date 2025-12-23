#include "parser.h"

int	parse_redirections(t_cmd *cmd, t_tok_vec *vec, size_t *i)
{
	t_redir	*redir;

	while (vec->tokens[*i]->type == TOK_REDIR_IN
		|| vec->tokens[*i]->type == TOK_REDIR_OUT
		|| vec->tokens[*i]->type == TOK_REDIR_APPEND)
	{
		if (*i + 1 >= vec->count)
			return (1);
		if (vec->tokens[*i + 1]->type != TOK_WORD)
			return (1);
		redir = new_redir_node(vec->tokens[*i]->type, strdup(vec->tokens[*i + 1]->value.str));
		if (!redir)
			return (1);
		add_redir_back(&cmd->redir, redir);
		*i += 2;
	}
	return (0);
}

t_ast	*parse_cmd(t_tok_vec *vec, size_t *i)
{
	t_ast		*ast;
	t_cmd		*cmd_data;
	t_arg_vec	arg_data;

	cmd_data = new_cmd_node();
	vec_init(&arg_data);
	if (!cmd_data)
		return (NULL);
	while (*i < vec->count && vec->tokens[*i]->type != TOK_PIPE)
	{
		if (vec->tokens[*i]->type == TOK_REDIR_IN
			|| vec->tokens[*i]->type == TOK_REDIR_OUT
			|| vec->tokens[*i]->type == TOK_REDIR_APPEND)
		{
			if (parse_redirections(cmd_data, vec, i))
				return (free_redir(cmd->data->redir), free(cmd_data), NULL);
		}
		else if (vec->tokens[*i]->type == TOK_WORD)
		{
			if (vec_add_arg(&arg_data, strdup(vec->tokens[*i]->value.str)))
				return (free_arg_vec(&arg_data), free_redir(cmd_data->redir), free(cmd_data), NULL);
			*i++;
		}
	}
	vec_add_arg(&arg_data, NULL);
	cmd_data->args = arg_data.args;
	ast = new_ast_node(NODE_CMD);
	if (!ast)
		return (free_arg_vec(&arg_data), free_redir(cmd_data->redir), free(cmd_data), NULL);
	ast->cmd = cmd_data;
	return (ast);
}
