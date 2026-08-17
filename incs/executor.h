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

/* Executor public API: turning an AST into side-effects.
   The main entry is execute_top_level() which walks the tree returned by
   the parser.  Every execute_* function returns t_execution_state so the
   exit code and any ctrl-c signal propagate up cleanly. */

#ifndef EXECUTOR_H
# define EXECUTOR_H

# include "public/executor_types.h"
# include "alias.h"

/* forward declarations to avoid heavy includes */
typedef struct s_shell		t_shell;
typedef struct s_ast_node	t_ast_node;

/* An expanded, ready-to-run simple command: pre-command assignments,
   the argv array, and the resolved executable path.  pooled=true means
   argv.ctx comes from argv_pool (don't free it, just reset the vec). */
typedef struct executable_cmd_s
{
	t_vec_env	pre_assigns; /* NAME=VALUE pairs before the command */
	t_vec		argv; /* NULL-terminated argv word list */
	char		*fname; /* resolved path (heap) or NULL for builtins */
	bool		pooled; /* true if argv was borrowed from argv_pool */
}	t_executable_cmd;

/* argv backing pool (free_utils2.c): borrow on expand, return on teardown. */
void				argv_pool_acquire(t_shell *state, t_executable_cmd *cmd);
void				argv_pool_release(t_shell *state, t_executable_cmd *cmd);
void				free_argv_pool(t_shell *state);

/* Execution context for one node in the pipeline: which fds to hook up,
   which AST node to execute, and whether this command runs in the parent
   process (builtins, function calls) and is allowed to modify state. */
typedef struct executable_node_s
{
	int			infd; /* read end of pipe from previous stage */
	int			outfd; /* write end of pipe to next stage */
	int			next_infd; /* saved next read end (pipeline setup) */
	t_ast_node	*node; /* the AST node being executed */
	t_vec_int	redirs; /* indices into state->redirects */
	bool		modify_parent_ctx; /* true for builtins that modify state */
}	t_executable_node;

void				execute_top_level(t_shell *state);
int					exec_string(t_shell *state, char *str);
t_execution_state	execute_pipeline(t_shell *state,
						t_executable_node *exe);
void				set_up_redirection(t_shell *state,
						t_executable_node *exe);
t_execution_state	execute_simple_command(t_shell *state,
						t_executable_node *exe);
void				forward_exit_status(t_execution_state res);
t_execution_state	execute_command(t_shell *state,
						t_executable_node *exe);
t_execution_state	execute_tree_node(t_shell *state,
						t_executable_node *exe);
t_execution_state	execute_simple_list(t_shell *state,
						t_executable_node *exe);
t_string			word_to_string(t_ast_node node);
t_string			word_to_hrdoc_string(t_ast_node node);
void				set_cmd_status(t_shell *state, t_execution_state res);
t_execution_state	res_status(int status);
t_execution_state	res_pid(int pid);
void				fire_debug_trap(t_shell *state);
void				fire_err_trap(t_shell *state, int code);
void				fire_return_trap(t_shell *state, int code);
void				trap_save_reset(t_shell *state, char **save);
void				trap_restore(t_shell *state, char **save);
void				exe_res_set_status(t_shell *st, t_execution_state *res);
int					find_cmd_path(t_shell *state, char *cmd_name,
						char **path_of_exe);
int					exec_external_argv(t_shell *state, t_vec *args);
int					run_external_sync(t_shell *state, t_vec *args);
int					expand_simple_command(t_shell *state, t_ast_node *node,
						t_executable_cmd *ret, t_vec_int *redirects);
int					(*builtin_func(char *name))(t_shell *state, t_vec argv);

#endif