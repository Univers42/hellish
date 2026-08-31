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
	OPT_FLAG_DEBUG_AST = 1u << 10,
	OPT_FLAG_NORC = 1u << 11
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
	SETOPT_ZSH = 1u << 17,
	SETOPT_KSHARRAYS = 1u << 18,
	SETOPT_LOCALOPTS = 1u << 19,
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

/* The zsh dialect gate (src/core/zsh_mode.c).  Every zsh-only construct in
   the lexer, parser, expander and builtins asks zsh_mode() first, so the
   default dialect stays exactly the bash the golden suite pins. */
bool	zsh_mode(t_shell *state);
bool	zsh_mode_swap(t_shell *state, bool on);
bool	zsh_path(const char *path);
/* True when arrays count from 1 -- zsh mode with ksh_arrays not set. */
bool	zsh_arrays(t_shell *state);
/* One written subscript -> the 0-based index the array store uses. */
long	sub_to_index(t_shell *state, long sub, long count);

void	parse_and_execute_input(t_shell *state);
/* getcwd(NULL,0) onto the active fn_* heap; see helpers/x_getcwd.c */
char	*x_getcwd(void);
/* release the pushd/popd dir stack at exit; builtins/builtin_dirstack.c */
void	free_dirstack(t_shell *state);
void	free_compspecs(t_shell *state);
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

/* One `complete` registration: what to offer when the user tabs an
   argument of NAME.  Lives in state->compspecs.
     `words` is -W (a literal word list), `func` is -F (a shell function
   that fills COMPREPLY), `act` is the letter of -A/-f/-d/-c/-v/... and 0
   for none, `opts` collects -o names verbatim.  A spec may carry several
   at once; bash runs the function first and appends the rest. */
typedef struct s_compspec
{
	char	*name;
	char	*words;
	char	*func;
	char	*opts;
	char	act;
}	t_compspec;

/* A user-defined shell function.  The body is the AST of the compound
   list between the braces, deep-cloned at definition time so the
   original parse tree can be freed.  Functions live in state->functions
   (a t_vec of t_shell_func) and are called by execute_simple_command. */
typedef struct s_shell_func
{
	char		*name; /* heap-allocated function name */
	t_ast_node	body; /* deep-cloned AST of the function body */
	char		*src; /* file it was DEFINED in (see below) */
	char		*text; /* its source text, for declare -f */
	bool		zsh; /* defined under the zsh dialect (see t_call_frame) */
}	t_shell_func;

/* src: the file this function was DEFINED in (strdup'd, NULL at top level).
   BASH_SOURCE[0] must name the defining file, not whatever happens to be
   sourcing when the function is CALLED -- that is what lets a plugin locate
   its own directory from inside a helper.
   text: the definition's SOURCE TEXT, captured before the body was cloned
   (ast_span.c explains why that timing is the whole trick); NULL when the
   span could not be recovered, and declare -f says so rather than inventing
   a body.

   One entry per live function call or `source`, innermost last. This is the
   backing store for FUNCNAME and BASH_SOURCE, which had nowhere to come from
   before: the shell tracked only the two int counters func_depth and
   source_depth, so a sourced file could not name itself and $0 answered
   /usr/bin/hellish. Issue #71 calls BASH_SOURCE the single highest-leverage
   gap for plugins, because without it every module has to hardcode its path.
   Frames OWN their strings. Borrowing t_shell_func.name/.src would repeat the
   use-after-free that retire_body() exists to prevent: a function may free
   its own definition mid-call --

       deactivate () { ... unset -f deactivate ; }

   which is not contrived, it is how every Python venv ends -- and the frame
   for that call is still on the stack, so the next publish would read freed
   memory. Two small strdups per call is the cheap way to make that
   impossible; the publish already allocates, so it is not the hot cost.

   zsh: the dialect that was in force when this frame was pushed, restored by
   frame_pop.  The frame is already the exact bracket around a call or a
   source, so it is the natural place to hang it -- and hanging it there is
   what makes a .zsh plugin work at all.  Sourcing the plugin parses it in zsh
   mode, but its functions are CALLED later from an interactive prompt where
   the mode is off, and `${(f)x}` in a body has to keep meaning what its
   author wrote.  So each function records the dialect it was defined in
   (t_shell_func.zsh) and execute_func_call re-arms it for the call; the
   frame puts back whatever the caller had.  Push sites declare the new
   dialect right after pushing, so nesting composes without a second stack.

   setopt/shopt: the option words as they were on entry, put back by
   frame_pop only when the body asked for `setopt localoptions`.  zsh's
   options are GLOBAL by default and that one word makes them function-local,
   which is not a nicety -- oh-my-zsh's extract says

       setopt localoptions extendedglob

   and extendedglob changes how every later pattern PARSES.  Letting that
   escape the function would silently re-interpret globs typed at the prompt
   half an hour later, with nothing to connect the two. */
typedef struct s_call_frame
{
	char				*func;
	char				*src;
	bool				zsh;
	unsigned int		setopt;
	unsigned int		shopt;
}	t_call_frame;

/* One saved variable for function scope: its value at the moment it was made
   local / before positional params were replaced, restored on return. */
typedef struct s_scope_save
{
	int		depth;
	char	*key;
	char	*value;
	bool	existed;
	/* The var_attrs entry as it was on entry, so `local -n` unwinds like
	   every other local. Saving only the VALUE was not enough: the nameref
	   attribute lives in a separate table, so an inner `local -n r=w` used
	   to leak past its own function's return and the caller's `r` silently
	   started following the callee's target. kind 0 means "no attribute",
	   which is also what restoring 0 through attr_set means. */
	char	attr_kind;
	char	*attr_target;
}	t_scope_save;

/* The ONLY way to fill one. Every field must be set; a partially built save
   frees garbage in restore_one. */
void	scope_save_capture(t_shell *state, const char *key, t_scope_save *s);

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
	int					cmd_no; /* PS1 \# : REPL turns this session, from 1.
								   NOT the same as \! -- history survives the
								   session and this does not. */
	bool				should_exit; /* set by `exit` builtin */
	bool				builtin_fatal; /* special builtin got a MALFORMED
										  request, not merely a failing one:
										  read and cleared by
										  strict_builtin_failed() so the
										  abort happens after teardown */
	/* --- loop/function control flow --- */
	int					loop_break; /* pending break depth (>0 = active) */
	int					loop_continue; /* pending continue depth */
	int					loop_depth; /* current nesting depth of loops */
	int					func_return; /* pending return value from `return` */
	int					func_depth; /* current function call depth */
	int					source_depth; /* nesting depth of `.`/`source` runs */
	/* --rcfile=FILE, else NULL (borrowed from argv) */
	char				*rcfile;
	/* t_call_frame stack backing FUNCNAME and BASH_SOURCE */
	t_vec				call_frames;
	/* call_frames changed since the two env arrays were last rebuilt */
	bool				frames_dirty;
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
	/* >0 while a prompt is being rendered. An error raised in there must
	   never end the session: a prompt redraws on every keystroke, and a
	   shell that exits because of a typo in PS1 gives the user no way back
	   in to fix it. arith_fail reads this and reports without exiting. */
	int					prompt_depth;
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
	t_hash				func_index; /* name -> functions slot+1 (O(1) lookup) */
	t_vec				dead_funcs; /* bodies retired during their own call */
	t_job_table			job_table; /* background job list */
	bool				exit_warned; /* stopped-job exit warning given */
	bool				exit_attempt; /* this turn tried to leave */
	pid_t				shell_pid; /* the REPL's own pid (fork detector) */
	pid_t				shell_pgid; /* shell's group, 0 if it owns no tty */
	pid_t				fg_pgid; /* group of the running foreground job */
	bool				jobctl; /* this process may drive job control */
	t_pal_procs			pal_procs; /* platform process registry (win32) */
	t_vec				arr_marks; /* live ${a[@]} deferral markers */
	t_vec				var_attrs; /* declare -i/-n attribute table */
	t_vec				dirstack; /* pushd/popd dir stack */
	t_vec				compspecs; /* t_compspec: `complete` registrations */
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
/* progcomp arrived as a truthful "no": the bit existed, defaulted off, and
** controlled nothing -- it was there so /etc/profile.d/bash_completion.sh's
** `shopt -q progcomp` got an answer instead of an error on every login
** (#51). It now MEANS what it says: src/completion/progcomp*.c reads it
** before it will consult a `complete` spec, and `shopt -s progcomp` turns
** on a dispatch that works end to end (#72 phase 4).
**
** It still defaults OFF, and bash defaults it on. That is the one place
** hellish deliberately differs, and the reason is measured, not cautious:
** the option is exactly the gate /etc/profile.d/bash_completion.sh checks,
** so switching it on makes every Debian and Ubuntu login source a
** 3800-line framework that hellish cannot yet run. The CI runner proved it
** -- one `syntax error near unexpected token '('` at login, which is #51's
** complaint arriving by the door #51 opened.
**
** The blocker is architectural: exec_string LEXES a whole sourced file
** before executing any of it, so `shopt -s extglob` on line 47 has not run
** when the extglob case pattern on line 1810 is tokenised. Lexing
** incrementally would fix it and flip this default in one line.
** tests/plugin_corpus_test.py carries the row that will notice. */
# define SHOPT_PROGCOMP 0x800

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
