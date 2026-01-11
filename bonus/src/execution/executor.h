/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:38:39 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:38:39 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXECUTOR_H
# define EXECUTOR_H

# include "shell.h"
# include "../infrastructure/error.h"

// Forward declarations to avoid circular dependency
typedef struct s_ast_node t_ast_node;
typedef struct s_vec t_vec;
typedef t_vec t_vec_int;

typedef enum s_res_t
{
	RES_OK,
	RES_FatalError,
	RES_MoreInput,
	RES_Init,
}	t_res_t;

typedef struct s_exe_res
{
	int	status;
	int	pid;
	bool	c_c;
}	t_exe_res;

typedef struct executable_cmd_s
{
	t_vec_env	pre_assigns;
	t_vec	argv;
	char		*fname;
}	t_executable_cmd;

typedef struct executable_node_s
{
	int			infd;
	int			outfd;
	int			next_infd;
	t_ast_node	*node;
	t_vec_int	redirs;
	bool		modify_parent_context;
}	t_executable_node;


void		execute_top_level(t_shell *state);
t_exe_res	execute_pipeline(t_shell *state, t_executable_node *exe);

void		set_up_redirection(t_shell *state, t_executable_node *exe);
t_exe_res	execute_simple_command(t_shell *state, t_executable_node *exe);
void		forward_exit_status(t_exe_res res);
t_exe_res	execute_command(t_shell *state, t_executable_node *exe);
t_exe_res	execute_tree_node(t_shell *state, t_executable_node *exe);
t_exe_res	execute_simple_list(t_shell *state, t_executable_node *exe);
t_string	word_to_string(t_ast_node node);
t_string	word_to_hrdoc_string(t_ast_node node);
void		set_cmd_status(t_shell *state, t_exe_res res);

t_exe_res	res_status(int status);
t_exe_res	res_pid(int pid);
void	exe_res_set_status(t_exe_res *res);

/* missing prototypes referenced by execution units */
int		find_cmd_path(t_shell *state, char *cmd_name, char **path_of_exe);
int		expand_simple_command(t_shell *state, t_ast_node *node,
			t_executable_cmd *ret, t_vec_int *redirects);
/* builtin lookup (signature also present in incs/builtins.h) */
int		(*builtin_func(char *name))(t_shell *state, t_vec argv);

# endif