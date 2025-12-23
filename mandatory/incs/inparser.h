#ifndef INPARSER_H
#define INPARSER_H

#include "inlexer.h"


typedef enum e_node_type
{
	NODE_CMD,
	NODE_PIPE,
}	t_node_type;

typedef struct s_redir
{
	t_tok_type		type;
	char			*target;
	struct s_redir	*next;
}	t_redir;

typedef struct s_cmd
{
	char	**args;
	t_redir	*redir;
}	t_cmd;

typedef struct s_ast
{
	t_node_type		type;
	t_cmd			*cmd;
	struct s_ast	*left;
	struct s_ast	*right;
}	t_ast;

typedef struct s_arg_vec
{
	char				**args;
	size_t				count;
	size_t				capacity;
}	t_arg_vec;

t_ast	*new_ast_node(t_node_type type);
t_redir	*new_redir_node(t_tok_type type, char *target);
t_cmd	*new_cmd_node(void);
void	add_redir_back(t_redir **redir, t_redir *new);
void	free_redir(t_redir *redir);
void	free_ast(t_ast *ast);

#endif
