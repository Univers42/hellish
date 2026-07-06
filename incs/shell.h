/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   shell.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:08 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:08 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* t_shell is the single source of truth for the running shell instance.
   Every subsystem (lexer, parser, executor, builtins, job control) takes
   a t_shell* and reads or writes fields here -- it is the god struct that
   ties everything together.  There is exactly ONE t_shell alive at any
   given time (subshells fork and get a copy in the child).
   Field groupings (logical order in the struct):
     - I/O state: input, metinp, rl, edit_mode
     - Execution state: tree, last_cmd_st*, should_exit, loop_*, func_*
     - Environment: env, cwd
     - Shell options: opt_* booleans, option_flags
     - Job/async: job_table, bg_job_count, proc_subs, last_bg_pid
     - Aliases, cmd cache, functions
     - Heredoc tracking: redirects, heredoc_idx, hd_*
     - Miscellaneous: pos, local_saves, traps, readonly_vars, prng */

#ifndef SHELL_H
# define SHELL_H

# include "alias.h"
# include "public/signals.h"
# include "public/error.h"
# include "ast.h"
# include "executor.h"
# include "redir.h"
# include "history.h"
# include "prompt.h"
# include "job_control.h"
# include "sh_alias.h"
# include "cmd_hash.h"

# include <stdint.h>

enum e_opt_flag
{
	OPT_FLAG_HELP = 1u << 0,
	OPT_FLAG_VERBOSE = 1u << 1,
	OPT_FLAG_POSIX = 1u << 2,
	OPT_FLAG_DEBUG_LEXER = 1u << 8,
	OPT_FLAG_DEBUG_PARSER = 1u << 9,
	OPT_FLAG_DEBUG_AST = 1u << 10
};

void	parse_and_execute_input(t_shell *state);
/* getcwd(NULL,0) onto the active fn_* heap; see helpers/x_getcwd.c */
char	*x_getcwd(void);
/* release the pushd/popd dir stack at exit; builtins/builtin_dirstack.c */
void	free_dirstack(t_shell *state);
/* remember / recover a finished bg child's status; src/execution/bg_done.c */
void	bg_done_record(t_shell *state, pid_t pid, int status);
int		bg_done_take(t_shell *state, pid_t pid, int *status);
/* opt-in ft_malloc live-bytes report at exit; see helpers/alloc_stats.c */
void	alloc_live_report(void);

typedef enum e_option
{
	OPT_DEBUG,
	OPT_DEBUGGER,
	OPT_HELP,
	OPT_INIT_FILE,
	OPT_LOGIN,
	OPT_POSIX,
	OPT_PRETTY_PRINT,
	OPT_RCFILE,
	OPT_RESTRICTED,
	OPT_VERBOSE,
	OPT_COMMAND
}	t_option;

/* One open process substitution (<(cmd) or >(cmd)).
   The shell opens a named pipe in /proc/self/fd, forks the child to
   write/read it, and passes the fd path to the parent command.  After
   the parent command exits, cleanup_proc_subs() collects these. */
typedef struct s_procsub_entry
{
	pid_t	pid; /* child PID running the substitution body */
	int		fd; /* the open fd on the shell side */
	char	*path; /* /proc/self/fd/<fd> string given to the command */
}	t_procsub_entry;

typedef t_vec	t_vec_procsub;

/* A user-defined shell function.  The body is the AST of the compound
   list between the braces, deep-cloned at definition time so the
   original parse tree can be freed.  Functions live in state->functions
   (a t_vec of t_shell_func) and are called by execute_simple_command. */
typedef struct s_shell_func
{
	char		*name; /* heap-allocated function name */
	t_ast_node	body; /* deep-cloned AST of the function body */
}	t_shell_func;

/* One saved variable for function scope: its value at the moment it was made
   local / before positional params were replaced, restored on return. */
typedef struct s_scope_save
{
	int		depth;
	char	*key;
	char	*value;
	bool	existed;
}	t_scope_save;

/* Positional parameters $1..$count, stored outside the env so function calls
   swap them in O(1) (a struct copy) instead of mutating the env hash 10x per
   call. args[i] == $(i+1); args is NULL-terminated; cnt_str caches $#. */
typedef struct s_pos
{
	char	**args;
	int		count;
	char	cnt_str[12];
	bool	args_owned;
}	t_pos;

/* Depth-indexed pool of reusable argv backing vectors: one slot per nesting
   level (command substitution, subshells, function bodies). A simple command
   borrows its slot, fills/reuses the backing array, and resets (not frees) it
   on teardown, so the steady state does zero per-command malloc/free.
   Past this depth a command falls back to a fresh vector. */
# define ARGV_POOL_DEPTH 64

/* Ring of finished-but-not-yet-waited background children. reap_background_
   children() reaps with waitpid() and would otherwise discard the exit status,
   so a later `wait <pid>` would race; we stash (pid, status) here and `wait`
   recovers it -- matching bash, which remembers a finished job until waited. */
# define BG_DONE_MAX 128

typedef struct s_bg_done
{
	pid_t	pid;
	int		status;
}	t_bg_done;

typedef struct s_shell
{
	/* --- I/O and readline state --- */
	t_string			input; /* current input line buffer */
	t_vec_env			env; /* the shell's variable store */
	t_string			cwd; /* current working directory string */
	t_ast_node			tree; /* parsed AST for current input */
	int					metinp; /* input method: INP_RL / FILE / ARG */
	char				*dft_ctx; /* default shell name for error msgs */
	char				*ctx; /* active error context (argv[0]) */
	/* --- special variables (borrowed ptrs or small buffers, not freed) --- */
	char				*pid; /* $$ as a string (set once at init) */
	char				*last_bg_pid; /* $! last background PID string */
	char				*last_cmd_st; /* $? string -> statbuf, never freed */
	t_execution_state	last_cmd_st_exe; /* structured copy of last status */
	/* --- history and session --- */
	t_history			hist; /* readline history state */
	bool				should_exit; /* set by `exit` builtin */
	/* --- loop/function control flow --- */
	int					loop_break; /* pending break depth (>0 = active) */
	int					loop_continue; /* pending continue depth */
	int					loop_depth; /* current nesting depth of loops */
	int					func_return; /* pending return value from `return` */
	int					func_depth; /* current function call depth */
	int					source_depth; /* nesting depth of `.`/`source` runs */
	/* --- positional parameters and local variable saves --- */
	t_pos				pos; /* $1..$N, $#, $* for current scope */
	t_vec				local_saves; /* t_scope_save stack for `local` */
	int					getopts_pos; /* OPTIND state for `getopts` builtin */
	/* --- expansion state --- */
	bool				input_expanded; /* alias expansion already done */
	int					last_cmdsub_status; /* $? inside $(...) body */
	/* --- set -o options (each maps to one POSIX flag) --- */
	bool				opt_errexit; /* -e: exit on first error */
	bool				opt_nounset; /* -u: error on unset variable use */
	bool				opt_xtrace; /* -x: print commands before running */
	bool				opt_noglob; /* -f: disable pathname expansion */
	bool				opt_noclobber; /* -C: refuse to overwrite via > */
	bool				opt_allexport; /* -a: export every variable on set */
	bool				opt_noexec; /* -n: parse but don't execute */
	bool				opt_verbose; /* -v: print input lines as read */
	bool				opt_pipefail; /* pipefail: status = last failure */
	bool				opt_posix; /* --posix / set -o posix: no extensions */
	/* small scratch buffers -- avoids allocs for $-, $LINENO and $? */
	char				flagbuf[16]; /* scratch for build_flagstr ($-) */
	char				linebuf[16]; /* scratch for lineno_str ($LINENO) */
	char				statbuf[16]; /* scratch for set_cmd_status ($?) */
	/* split-$PATH cache, validated on use against the exact PATH string */
	char				**path_dirs; /* cached ft_split of $PATH on ':' */
	char				*path_dirs_src; /* PATH string the split came from */
	int					errexit_off; /* >0: -e is suspended (in conditions) */
	/* --- traps and readonly vars --- */
	char				*traps[32]; /* trap strings, indexed by signal num */
	t_vec				readonly_vars; /* names that cannot be reassigned */
	/* --- heredoc runtime state --- */
	t_vec_redir			redirects; /* active redirections for current cmd */
	int					heredoc_idx; /* next slot in the redirect vector */
	char				*hd_src; /* raw heredoc body string (pre-expand) */
	size_t				hd_pos; /* read position within hd_src */
	char				*hd_stripped; /* tab-stripped heredoc body */
	bool				gather_in_func; /* true while gathering heredocs */
	bool				gathering_compound; /* mid incomplete compound cmd */
	/* --- readline and PRNG --- */
	t_rl				rl; /* readline line/column tracking */
	t_prng				prng; /* cheap PRNG for $RANDOM */
	uint32_t			option_flags; /* bitmask of e_opt_flag values */
	/* --- job control and async --- */
	int					bg_job_count; /* running background job count */
	t_bg_done			bg_done[BG_DONE_MAX]; /* finished bg job statuses */
	int					bg_done_next; /* next write slot in bg_done ring */
	t_vec_procsub		proc_subs; /* open process substitutions */
	t_vec				functions; /* t_shell_func list (user-defined fns) */
	t_job_table			job_table; /* background job list */
	t_vec				dirstack; /* pushd/popd dir stack */
	/* --- alias and command cache --- */
	t_hash				aliases; /* alias name -> t_alias_entry */
	t_hash				cmd_cache; /* command name -> resolved path cache */
	int					edit_mode; /* 0=vi, 1=emacs (rl_editing_mode) */
	/* --- argv slab pool (zero-malloc fast path for simple commands) --- */
	t_vec				argv_pool[ARGV_POOL_DEPTH];
	int					argv_pool_depth; /* current borrow depth */
}	t_shell;

/* Directory matcher ctx for glob expansion */
typedef struct s_dir_matcher
{
	char		*path;
	DIR			*dir;
	t_vec_glob	glob;
	size_t		offset;
	t_vec		*args;
}	t_dir_matcher;

static inline t_shell	shell_init(void)
{
	return ((t_shell){0});
}

#endif
