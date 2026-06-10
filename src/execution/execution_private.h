/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execution_private.h                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:05:22 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 17:12:27 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXECUTION_PRIVATE_H
# define EXECUTION_PRIVATE_H

# include "shell.h"
# include <stdbool.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
# include <errno.h>
# include "sh_error.h"
# include "env.h"
# include "redir.h"
# include <stddef.h>
# include <stdio.h>
# include "libft.h"
# include <readline/readline.h>
# include <string.h>
# include <sys/wait.h>
# include <signal.h>
# include "helpers.h"
# include "expander.h"
# include "sh_input.h"
# include "executor_types.h"

# define MSG_REDIR_DATA_ERR "%s: internal error: redirects \
							present but no redirect data\n"
# define MSG_AMBIGUOUS_REDIR "%s: ambiguous redirect\n"
# define MSG_INT_ERR_REDIR_IDX "%s: internal error: \
invalid redirect index %d\n"

# define NAME "shell"

/* Iteration context for execute_pipeline_children.  Bundles everything
   the loop body needs across prepare_child_exec and finalize_child_parent
   so we do not have to pass eight separate arguments to each helper.
   pp points at the pipe pair created for the current stage; prev_infd is
   threaded forward as each stage's stdin fd. */
typedef struct s_exec_child_ctx
{
	t_shell				*state;
	t_executable_node	*exe;			/* template exe node */
	t_executable_node	*curr_exe;		/* per-child node */
	int					(*pp)[2];		/* pointer to pipe pair */
	int					prev_infd;		/* preserved input fd */
	size_t				idx;			/* current child index */
	size_t				last_index;		/* last child index */
	t_vec_exe_res		*results;		/* results vector */
}	t_exec_child_ctx;

/* Helper prototypes -- declared here rather than in the public header so
   they are available to every .c in this directory regardless of inclusion
   order, without polluting the global namespace. */
char				*env_expand(t_shell *state, char *key);
void				free_tab(char **tab);
void				err_1_errno(t_shell *state, char *p1);
void				err_2(t_shell *state, char *p1, char *p2);
t_execution_state	execute_cmd_bg(t_shell *state, t_executable_node *exe,
						t_executable_cmd *cmd);
bool				check_is_a_dir(char *path, bool *enoent);
int					cmd_not_found(t_shell *state, char *cmd_name);
int					no_such_file_or_dir(t_shell *state,
						char *cmd_name, char *path_of_exe);
char				*exe_path(char **path_dirs, char *exe_name,
						int *perm_denied);
t_execution_state	execute_builtin_cmd_fg(t_shell *state,
						t_executable_cmd *cmd, t_executable_node *exe);
t_execution_state	execute_command(t_shell *state, t_executable_node *exe);
void				set_up_redir_pipeline_child(bool is_last,
						t_executable_node *exe,
						t_executable_node *curr_exe, int (*pp)[2]);
void				execute_pipeline_children(t_shell *state,
						t_executable_node *exe,
						t_vec_exe_res *results);
t_execution_state	execute_pipeline(t_shell *state, t_executable_node *exe);
t_execution_state	execute_simple_command(t_shell *state,
						t_executable_node *exe);
void				reap_background_children(t_shell *state);
bool				should_execute(t_execution_state prev_status, t_tt prev_op);
size_t				find_next_separator(t_ast_node *node,
						size_t start, bool *found_amp);
t_execution_state	execute_range(t_shell *state, t_executable_node *exe,
						size_t start, size_t end);
t_execution_state	execute_range_background(t_shell *state,
						t_executable_node *exe,
						size_t start, size_t end);
t_execution_state	execute_simple_list(t_shell *state,
						t_executable_node *exe);
t_execution_state	execute_subshell(t_shell *state, t_executable_node *exe);
void				execute_top_level(t_shell *state);
t_execution_state	execute_tree_node(t_shell *state, t_executable_node *exe);
t_shell_func		*func_lookup(t_shell *state, const char *name);
t_execution_state	execute_if(t_shell *state, t_executable_node *exe);
t_execution_state	execute_while(t_shell *state, t_executable_node *exe);
t_execution_state	execute_for(t_shell *state, t_executable_node *exe);
t_execution_state	execute_case(t_shell *state, t_executable_node *exe);
t_execution_state	execute_func_def(t_shell *state, t_executable_node *exe);
t_execution_state	execute_func_call(t_shell *state, t_shell_func *fn,
						t_vec *argv);
int					find_cmd_path(t_shell *state, char *cmd_name,
						char **path_of_exe);
void				prehash_external(t_shell *state, char *argv0);
t_execution_state	res_status(int status);
t_execution_state	res_pid(int pid);
void				exe_res_set_status(t_execution_state *res);
t_execution_state	pipeline_status(t_shell *state, t_vec_exe_res *results);
int					actually_run(t_shell *state, t_vec *args);
void				update_underscore_var(t_shell *state,
						t_executable_cmd *cmd);
void				set_up_redirection(t_shell *state,
						t_executable_node *exe);
int					fd_setup_needed(t_executable_node *exe);
void				take_backup_fds(int *bak, int persist);
void				restore_backup_fds(int *bak, int persist);
void				restore_fds(int *bak);
int					prep_redir(t_shell *state, t_executable_node *exe,
						int *bak, int persist);
void				apply_redirs_from_vec(t_shell *state,
						t_executable_node *exe);
void				apply_redirs_from_ast(t_shell *state,
						t_executable_node *exe);
int					handle_direct_path_error(t_shell *state, char *cmd_name,
						char **path_of_exe);
int					run_builtin_or_continue(t_shell *state, t_vec *args);
int					find_exe_path_wrapper(t_shell *state,
						char *cmd0, char **out_path);
void				try_exec_with_fallback(char *path_of_exe,
						t_vec *args, char **envp);
void				cleanup_after_exec_failure(t_vec *args,
						char *path_of_exe, char **envp);
int					map_errno_to_exit(void);
void				set_for_var(t_shell *state, char *name, char *val);
void				restore_one(t_shell *state, t_scope_save *s);
void				restore_temp_assigns(t_shell *state, t_vec *saves);

/* Zero-initialise a t_executable_node with the given fds and node
   pointer.  next_infd=-1 means "no pipe-read-end to close".  The redirs
   vec is left zeroed (elem_size=0) -- callers that need a redirect list
   must call vec_init(&exe.redirs) themselves. */
static inline t_executable_node	create_exe_node(int infd,
										int outfd,
										t_ast_node *node,
										bool modify_parent_ctx)
{
	return ((t_executable_node){
		.infd = infd,
		.outfd = outfd,
		.next_infd = -1,
		.node = node,
		.redirs = (t_vec){0},
		.modify_parent_ctx = modify_parent_ctx,
	});
}

#endif