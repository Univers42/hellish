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
# include "pal.h"
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
# include "pal_wait.h"
# include <signal.h>
# include "helpers.h"
# include "expander.h"
# include "sh_input.h"
# include "executor_types.h"
# include "lexer.h"
# include "parser.h"

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

/* bash's hook for a command that PATH could not resolve. Debian/Ubuntu
   define it in /etc/bash.bashrc, and it is what turns "vim: command not
   found" into the "sudo apt install vim" suggestion. See cmd_not_found.c.
   Returns the handler's exit status, or -1 for "no handler ran". */
# define CNF_HANDLER "command_not_found_handle"

int					try_cmd_not_found_handler(t_shell *state, t_vec *args);
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
void				note_cmd_lineno(t_shell *state, t_ast_node *node);
void				snapshot_positionals(t_shell *state, t_vec *out);
void				free_positional_snapshot(t_vec *w);
t_execution_state	execute_simple_command(t_shell *state,
						t_executable_node *exe);
void				reap_background_children(t_shell *state);
bool				should_execute(t_execution_state prev_status, t_tt prev_op);
size_t				find_next_separator(t_ast_node *node,
						size_t start, bool *found_amp);
t_execution_state	execute_range(t_shell *state, t_executable_node *exe,
						size_t start, size_t end);
void				async_child_signals(void);
void				fg_job_stopped(t_shell *st, t_execution_state *res,
						int status);
t_execution_state	execute_range_background(t_shell *state,
						t_executable_node *exe,
						size_t start, size_t end);
t_execution_state	execute_simple_list(t_shell *state,
						t_executable_node *exe);
t_execution_state	execute_subshell(t_shell *state, t_executable_node *exe);
t_execution_state	execute_coproc(t_shell *state, t_executable_node *exe);
t_execution_state	run_compound(t_shell *state, t_executable_node *exe);
t_execution_state	fork_compound(t_shell *state, t_executable_node *exe);
void				coproc_store(t_shell *state, char *name, int *fds,
						pid_t pid);
void				execute_top_level(t_shell *state);
t_execution_state	execute_tree_node(t_shell *state, t_executable_node *exe);
t_shell_func		*func_lookup(t_shell *state, const char *name);
void				retire_body(t_shell *state, t_ast_node *body);
void				drain_dead_funcs(t_shell *state);
t_execution_state	execute_if(t_shell *state, t_executable_node *exe);
t_execution_state	execute_while(t_shell *state, t_executable_node *exe);
t_execution_state	execute_for(t_shell *state, t_executable_node *exe);
t_execution_state	execute_case(t_shell *state, t_executable_node *exe);
t_execution_state	execute_func_def(t_shell *state, t_executable_node *exe);
int					handle_loop_ctl(t_shell *state);
t_execution_state	execute_arith_cmd(t_shell *state, t_executable_node *exe);
t_execution_state	execute_for_arith(t_shell *state, t_executable_node *exe);
t_execution_state	execute_tree_node_ext(t_shell *state,
						t_executable_node *exe, t_ast_type t);
void				free_local_saves(t_shell *state);
t_execution_state	execute_anon_func(t_shell *state,
						t_executable_node *exe);
t_execution_state	execute_func_call(t_shell *state, t_shell_func *fn,
						t_vec *argv);
int					find_cmd_path(t_shell *state, char *cmd_name,
						char **path_of_exe);
void				prehash_external(t_shell *state, char *argv0);
void				path_cache_sync(t_shell *state);
t_execution_state	res_status(int status);
t_execution_state	res_pid(int pid);
void				exe_res_set_status(t_shell *st, t_execution_state *res);
t_execution_state	pipeline_status(t_shell *state, t_vec_exe_res *results);
void				set_pipestatus(t_shell *state, t_vec_exe_res *results);
void				set_pipestatus_one(t_shell *state, int status);
t_execution_state	execute_pipeline_one(t_shell *state,
						t_executable_node *exe);
t_execution_state	negate_status(t_shell *state, t_execution_state res);
t_execution_state	pipeline_single(t_shell *state, t_executable_node *exe);
int					actually_run(t_shell *state, t_vec *args);
void				update_underscore_var(t_shell *state,
						t_executable_cmd *cmd);
void				set_up_redirection(t_shell *state,
						t_executable_node *exe);
int					fd_setup_needed(t_executable_node *exe);
int					save_fd(int fd);
void				take_backup_fds(int *bak, int persist);
void				restore_backup_fds(int *bak, int persist);
void				restore_fds(int *bak);
bool				redir_err_is_fatal(t_shell *state,
						t_executable_cmd *cmd);
bool				strict_builtin_failed(t_shell *state,
						t_executable_cmd *cmd, int status);
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

/* zsh's multi-variable for (execute_for_zsh.c). The names live as one
   space-separated span in the node token; zfor_count is how the loop knows
   its stride, so a one-name loop is unchanged. */
int					zfor_count(const char *names, int len);
size_t				for_stride(t_ast_node *node);
void				free_word_vec(t_vec *w);
void				zfor_bind_row(t_shell *state, t_ast_node *node,
						t_vec *w, size_t base);

/* Brace groups, and zsh's `} always { }` (execute_always.c). A group with a
   second child carries a cleanup block that runs however the body exited. */
t_execution_state	execute_brace_group(t_shell *state,
						t_executable_node *exe);

/* zsh's multi-name function definition (execute_func_zsh.c): one body under
   several names, each brace-expanded. Returns how many were defined. */
int					zfunc_define_all(t_shell *state, t_token *tok,
						t_ast_node *body);

/* Chunked exec_string (exec_string3.c / exec_string4.c, issue #105): one
   in-flight chunk of an eval/source string. [start,end) index the ORIGINAL
   text; spliced is this chunk's own alias expansion (each chunk is spliced
   exactly once, so alias bodies are never re-expanded); asts holds the
   fully parsed statements, executed only when the whole chunk parsed. */
typedef struct s_chunkctx
{
	size_t			start;
	size_t			end;
	char			*chunk;
	char			*spliced;
	t_deque_tok		tt;
	t_parser		parser;
	t_vec			asts;
}	t_chunkctx;

void				chunk_close(t_chunkctx *c);
void				chunk_grow(t_shell *state, const char *s, size_t n,
						t_chunkctx *c);
int					exec_chunks(t_shell *state, const char *str);
void				skip_delimiters(t_deque_tok *tt);
int					run_one_stmt(t_shell *state, t_deque_tok *tt,
						bool *stop);
int					run_parsed(t_shell *state, t_ast_node *ast);
bool				must_stop(t_shell *state);

/* select's loop state (execute_select.c): the expanded words, the body
   wrapped once, the loop variable, whether the next round reprints the
   menu, and the status the loop ends with. */
typedef struct s_select
{
	t_vec				words;
	t_executable_node	body;
	char				*name;
	bool				menu;
	t_execution_state	status;
}	t_select;

/* One menu rendering (execute_select2.c / execute_select3.c), bash's
   print_select_list geometry: max_len is the cell width (the widest
   "N) word" plus two), idx_len the digits of the item count, first_len
   the digits of the row count (the first column's index field), rows the
   row count. */
typedef struct s_selmenu
{
	t_vec		*words;
	t_string	*out;
	int			max_len;
	int			idx_len;
	int			first_len;
	int			rows;
}	t_selmenu;

t_execution_state	execute_select(t_shell *state, t_executable_node *exe);
t_vec				for_expand_words(t_shell *state, t_ast_node *node,
						size_t wc);
void				select_print_menu(t_shell *state, t_vec *words);
int					sel_columns(t_shell *state);
void				sel_indent(t_string *out, int from, int to);
int					sel_item(t_selmenu *m, int ind, int il);
int					sel_displen(const char *s);
int					sel_numlen(int n);
bool				sel_number(const char *s, long *n);
void				sel_write(int fd, const char *s, size_t n);
void				sel_geometry(t_selmenu *m, int width);

#endif