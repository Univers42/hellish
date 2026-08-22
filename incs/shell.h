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
# include "pal_types.h"
# include "job_control.h"
# include "sh_alias.h"
# include "cmd_hash.h"

# include <stdint.h>

enum e_opt_flag
{
	OPT_FLAG_HELP = 1u << 0,
	OPT_FLAG_VERBOSE = 1u << 1,
	OPT_FLAG_POSIX = 1u << 2,
	OPT_FLAG_LOGIN = 1u << 3,
	OPT_FLAG_VERSION = 1u << 4,
	OPT_FLAG_DEBUG_LEXER = 1u << 8,
	OPT_FLAG_DEBUG_PARSER = 1u << 9,
	OPT_FLAG_DEBUG_AST = 1u << 10
};

/* The `set -o` options that do not own a dedicated opt_* bool.  bash accepts
   every one of these as a short letter and/or a long name, and a script that
   opens with `set -euo pipefail` or `set -m` must not die on a usage error --
   so the roster is complete even where the semantics are not.  braceexpand
   is genuinely wired (it gates the expander's brace pass); the rest are
   recorded so `set -o`, `$-` and a later `set +o <name>` report back exactly
   what the script asked for instead of lying by omission.  The roster and
   the letter<->name mapping live in src/builtins/set_opts4.c. */
enum e_setopt
{
	SETOPT_BRACEEXPAND = 1u << 0,
	SETOPT_ERRTRACE = 1u << 1,
	SETOPT_FUNCTRACE = 1u << 2,
	SETOPT_HASHALL = 1u << 3,
	SETOPT_HISTEXPAND = 1u << 4,
	SETOPT_HISTORY = 1u << 5,
	SETOPT_IGNOREEOF = 1u << 6,
	SETOPT_ICOMMENTS = 1u << 7,
	SETOPT_KEYWORD = 1u << 8,
	SETOPT_MONITOR = 1u << 9,
	SETOPT_NOLOG = 1u << 10,
	SETOPT_NOTIFY = 1u << 11,
	SETOPT_ONECMD = 1u << 12,
	SETOPT_PHYSICAL = 1u << 13,
	SETOPT_PRIVILEGED = 1u << 14,
	SETOPT_EMACS = 1u << 15,
	SETOPT_VI = 1u << 16,
	SETOPT_DEFAULT = SETOPT_BRACEEXPAND | SETOPT_HASHALL | SETOPT_ICOMMENTS
};

/* One row of the `set -o` roster; the table itself is in set_opts4.c.  Both
   the `set` builtin and the command-line parser look options up through it,
   so the two can never disagree about which options exist. */
typedef struct s_setopt
{
	const char	*name;
	char		letter;
	uint32_t	bit;
}	t_setopt;

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

/* Trap table size: slot 0 is the EXIT pseudo-signal, slots 1..64 are real
   signal numbers. bash accepts numeric trap conditions up to 64 (the Linux
   realtime maximum) and rejects 65+, so the table must reach that far even
   though only 1..31 have names in the lookup table. */
# define SH_NSIG 65
/* Non-signal trap conditions, stored in the same table above the real
   signals: DEBUG fires before each simple command, RETURN when a function
   returns, ERR when a command fails (same suppression rules as set -e).
   Like bash without functrace/errtrace, these are NOT inherited by called
   functions — execute_func_call save/resets them across the call. */
# define TRAP_DEBUG 65
# define TRAP_RETURN 66
# define TRAP_ERR 67
# define SH_NTRAP 68

typedef struct s_bg_done
{
	pid_t	pid;
	int		status;
}	t_bg_done;

typedef struct s_shell
{
	/* --- I/O and readline state --- */
	t_string			input; /* current input line buffer */
	t_string			alias_exp; /* alias-expanded copy fed to the lexer */
	t_vec_env			env; /* the shell's variable store */
	t_string			cwd; /* current working directory string */
	t_ast_node			tree; /* parsed AST for current input */
	int					metinp; /* input method: INP_RL / FILE / ARG */
	char				upd_tag[32]; /* pending update version, "" if none */
	long				upd_seen; /* when upd_tag was last refreshed */
	char				*dft_ctx; /* default shell name for error msgs */
	char				*ctx; /* active error context (argv[0]) */
	/* --- special variables (borrowed ptrs or small buffers, not freed) --- */
	char				*pid; /* $$ as a string (set once at init) */
	char				*last_bg_pid; /* $! last background PID string */
	char				*last_cmd_st; /* $? string -> statbuf, never freed */
	t_execution_state	last_cmd_st_exe; /* structured copy of last status */
	long long			last_cmd_ms; /* wall-clock ms of last command */
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
	int					getopts_pos; /* getopts char pos inside current word */
	char				*getopts_ref; /* OPTIND value getopts last wrote; */
	/* compared by ADDRESS only (never dereferenced) to detect that the
	   user assigned OPTIND since the previous getopts call -- any outside
	   assignment stores a fresh string, so a pointer mismatch means the
	   intra-word scan position must be reset (mirrors bash's sv_optind). */
	/* --- expansion state --- */
	bool				input_expanded; /* alias expansion already done */
	bool				alias_exp_owned; /* alias_exp.ctx is its own alloc,
										not a borrow of input.ctx */
	int					last_cmdsub_status; /* $? inside $(...) body */
	bool				cmdsub_in_place; /* this process IS a disposable $( )
										body whose single external command
										may execve without forking again */
	/* Same trick for a background child (`cmd &`): the ONE simple command
	   this process was forked to run may execve in place, so $! names the
	   command and not a wrapper (issue #13).  Stored as the AST node's
	   address rather than a bool so a nested fork -- a $( ) or <( ) inside
	   the command's own words -- can never mistake itself for the
	   authorised command; a different node simply does not match. */
	t_ast_node			*bg_exec_node;
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
	uint32_t			setopt; /* e_setopt bitset: the rest of `set -o` */
	unsigned int		shopt; /* shopt -s bitset (SHOPT_* in shell.h) */
	bool				opt_interactive; /* -i: force $- to carry 'i' */
	/* small scratch buffers -- avoids allocs for $-, $LINENO and $? */
	/* flagbuf: 20 option letters + the invocation letter + NUL */
	char				flagbuf[32]; /* scratch for build_flagstr ($-) */
	char				linebuf[24]; /* scratch for $LINENO/$RANDOM/... */
	long long			start_sec; /* epoch at startup, for $SECONDS */
	char				statbuf[16]; /* scratch for set_cmd_status ($?) */
	/* split-$PATH cache, validated on use against the exact PATH string */
	char				**path_dirs; /* cached ft_split of $PATH on ':' */
	char				*path_dirs_src; /* PATH string the split came from */
	int					errexit_off; /* >0: -e is suspended (in conditions) */
	/* --- traps and readonly vars --- */
	char				*traps[SH_NTRAP]; /* trap strings, by signal num */
	int					trap_depth; /* >0 while a trap body runs */
	t_vec				readonly_vars; /* names that cannot be reassigned */
	/* --- heredoc runtime state --- */
	t_vec_redir			redirects; /* active redirections for current cmd */
	int					heredoc_idx; /* next slot in the redirect vector */
	bool				hd_defer; /* gather heredocs per top-level range */
	t_vec				*for_snapshot; /* live "$@" copy of a running for */
	bool				cycle_has_hd; /* "<<" seen in this cycle's input */
	bool				cycle_streamed; /* ranges parse+exec'd in-stream */
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
	t_vec				dead_funcs; /* bodies retired during their own call */
	t_job_table			job_table; /* background job list */
	bool				exit_warned; /* stopped-job exit warning given */
	pid_t				shell_pid; /* the REPL's own pid (fork detector) */
	pid_t				shell_pgid; /* shell's group, 0 if it owns no tty */
	pid_t				fg_pgid; /* group of the running foreground job */
	bool				jobctl; /* this process may drive job control */
	t_pal_procs			pal_procs; /* platform process registry (win32) */
	t_vec				arr_marks; /* live ${a[@]} deferral markers */
	t_vec				var_attrs; /* declare -i/-n attribute table */
	t_vec				dirstack; /* pushd/popd dir stack */
	/* --- alias and command cache --- */
	t_hash				aliases; /* alias name -> t_alias_entry */
	t_hash				cmd_cache; /* command name -> resolved path cache */
	int					edit_mode; /* 0=vi, 1=emacs (rl_editing_mode) */
	/* --- argv slab pool (zero-malloc fast path for simple commands) --- */
	t_vec				argv_pool[ARGV_POOL_DEPTH];
	int					argv_pool_depth; /* current borrow depth */
}	t_shell;

/* shopt option bits (state->shopt). Only the ones with observable
   behaviour or that scripts commonly toggle are modelled. */
# define SHOPT_NULLGLOB 0x001
# define SHOPT_DOTGLOB 0x002
# define SHOPT_GLOBSTAR 0x004
# define SHOPT_NOCASEGLOB 0x008
# define SHOPT_EXTGLOB 0x010
# define SHOPT_LASTPIPE 0x020
# define SHOPT_HISTAPPEND 0x040
# define SHOPT_CHECKWINSIZE 0x080
# define SHOPT_AUTOCD 0x100
# define SHOPT_CDSPELL 0x200
# define SHOPT_LITHIST 0x400

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
