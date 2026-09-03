/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtins_private.h                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:02:26 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 02:00:08 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BUILTINS_PRIVATE_H
# define BUILTINS_PRIVATE_H

# include "shell.h"
# include "pal.h"
# include "env.h"
# include "libft.h"
# include "ft_builtins.h"
# include <stdlib.h>
# include "helpers.h"
# include "sh_input.h"

/* print's option set (builtin_zsh_print.c); here because the norm keeps
   typedefs out of .c files. */
typedef struct s_pflags
{
	bool	raw;
	bool	nonl;
	bool	lines;
	bool	prompt;
}	t_pflags;

/* vcs_info's per-call state (builtin_zsh_vcs.c): the branch and root from
   the prompt's cached git reader, the captured stagedstr/unstagedstr
   styles, and the GIT_* bits deciding whether %c / %u render. */
typedef struct s_vcs
{
	const char	*branch;
	const char	*root;
	const char	*staged;
	const char	*unstaged;
	int			bits;
}	t_vcs;

/* un-export (export_helpers3.c, builtin_declare5.c) */
bool	export_wants_unexport(t_vec av, size_t first_operand);
int		export_unexport_arg(t_shell *st, const char *word);
int		declare_unexport(t_shell *state, t_vec argv, size_t i);
int		declare_flag_bits(const char *w);
/* a bare `wait`, bash's reporting rules (builtin_wait2.c) */
int		wait_all(t_shell *state);

# define CD_ERROR "cd: error retrieving current directory: getcwd: \
				cannot access parent directories: \
				No such file or directory\n"
# define PWD_ERR_CUR_DIR "pwd: error retrieving current directory: \
				getcwd: cannot access parent directories: \
				No such file or directory\n"
# define OLDPWD_NO_SET "%s: cd: OLDPWD not set\n"
# define OLDPWD_NAME "OLDPWD"
# define PWD_NAME "PWD"

/* One parsed `history` invocation. bash lets the modifiers be bundled
   (`history -cd 1`) but treats -a/-n/-r/-w as mutually exclusive, so the
   file operation gets its own slot rather than sharing `act`. `first` is
   the index of the first operand left in argv once options are consumed. */
/* One `pretty` feature: the curated name a user types, the SHOPT_* bit it
   IS (no separate state -- see builtin_pretty.c), and one line of help. */
typedef struct s_pret
{
	const char		*name;
	unsigned int	bit;
	const char		*desc;
}	t_pret;

typedef struct s_histopt
{
	char	fileop;
	char	act;
	bool	clear;
	int		first;
}	t_histopt;

typedef struct s_rdopt
{
	char	*ifs;
	bool	raw;
	size_t	first;
	char	*aname;
	char	*prompt;
	long	nchars;
	bool	exact;
	char	delim;
	long	tmo_ms;
}	t_rdopt;

/* compgen's parsed options: -W's list (borrowed from argv) and the single
   action letter -A/-f/-d/... resolves to. */
typedef struct s_cgopt
{
	char	*words;
	char	act;
	char	*xfilter;
	char	*prefix;
	char	*suffix;
}	t_cgopt;

/* complete's parsed options. All pointers borrow from argv; comp_store
   copies per NAME, since one command can register several names. */
typedef struct s_cmpopt
{
	char	*words;
	char	*func;
	char	*opts;
	char	act;
	bool	print;
	bool	remove;
	bool	defsel;
}	t_cmpopt;

typedef struct s_getopts
{
	char	*optstring;
	char	*name;
	int		optind;
	int		count;
	bool	silent;
	bool	bad_name;
}	t_getopts;

typedef struct s_ulim
{
	char		opt;
	int			res;
	long		scale;
	const char	*label;
}	t_ulim;

/* Cursor for the recursive-descent test-expression parser
   (builtin_test_expr.c): the argv slice being parsed, the read position,
   and an error flag that poisons the whole evaluation (exit 2). */
typedef struct s_tx
{
	char	**av;
	int		ac;
	int		i;
	int		err;
}	t_tx;

/* Block-buffered byte source for the read builtin (builtin_read4.c): on a
   seekable fd 0 bytes are taken from buf and the fd is lseek'd back over
   the unconsumed tail when the line ends; on pipes/ttys it degrades to the
   POSIX-required one-byte reads. */
typedef struct s_rdbuf
{
	char	buf[128];
	ssize_t	len;
	ssize_t	pos;
	bool	seekable;
}	t_rdbuf;

int		try_unset(t_shell *state, char *key);
int		confirm_update(void);
int		unset_operands(t_shell *state, t_vec argv, size_t i, int fmode);
int		builtin_unset(t_shell *state, t_vec argv);
int		parse_flags(t_vec argv, int *n, int *e);
int		print_args(t_vec *out, int e, t_vec argv, size_t i);
int		builtin_echo(t_shell *state, t_vec argv);
void	exit_clean(t_shell *state, int code);
int		builtin_exit(t_shell *state, t_vec argv);
void	parse_export_arg(char *str, char **ident, char **val);
char	strip_surrounding_quotes(char **val);
int		handle_identifier(t_shell *st, char *id, char *val, const char *argv0);
int		process_arg(t_shell *st, t_vec av, int i);
bool	bad_opt_word(const char *a, const char *valid);
size_t	export_skip_opts(t_shell *st, t_vec av, int *err);
void	collect_and_print_exported(t_shell *st);
int		builtin_export(t_shell *st, t_vec av);
int		builtin_pwd(t_shell *state, t_vec argv);
int		builtin_read(t_shell *state, t_vec argv);

void	print_exit_if_readline(t_shell *state);
bool	exit_stopped_guard(t_shell *state);
int		handle_no_args(t_shell *state, t_vec argv);
size_t	handle_double_dash(t_shell *state, t_vec argv, size_t i);
int		handle_non_numeric(t_shell *state, t_vec argv, size_t i, long long *r);
int		exit_parse_ll(const char *s, long long *out);
int		shift_operand(t_shell *state, t_vec argv, int *out);
void	subscript_assign(t_shell *state, t_env *ret);
char	*declare_assign_eq(const char *word);
char	*expand_export_value(t_shell *st, char *val, bool allow_expand);
bool	ft_is_valid_ident(char *id);

/* [[ ]] conditional: eval_test = flat single test (also [ and test);
   eval_bracketed = dispatcher (validate/strip close, route [[ to db_or).
   eval_two/three/four are the POSIX argument-count disambiguation rules
   (builtin_test_posix.c); eval_four returns -1 to mean "use the grammar". */
int		eval_test(char **av, int ac);
int		eval_two(char **av);
int		eval_three(char **av);
int		eval_four(char **av);
int		tx_or(t_tx *t);
bool	tx_is_binop(const char *s);
int		tx_test_unary(char **a);
bool	test_var_isset(t_shell *st, char *name);

/* compgen / complete (#72 phase 4). cg_* generate completions, comp_*
   store and print the `complete` registrations kept in state->compspecs.
   CG_OPT_ERR is what both option scanners return for "usage error"; the
   caller turns it into status 2. */
# define CG_OPT_ERR 0xFFFFFFFFFFFFFFFFUL

int		builtin_compgen(t_shell *state, t_vec argv);
int		builtin_complete(t_shell *state, t_vec argv);
int		cg_emit(t_cgopt *o, const char *s, const char *pfx);
void	cg_add(t_vec *out, const char *s, const char *pfx);
int		cg_flush(t_cgopt *o, t_vec *out);
int		cg_source(t_shell *st, t_cgopt *o, const char *pfx);
int		cg_aliases(t_shell *st, t_cgopt *o, const char *pfx);
int		cg_glob_paths(t_cgopt *o, const char *pfx);
int		cg_print(t_cgopt *o, const char *cand);
int		cg_emit_n(t_cgopt *o, const char *s, int n, const char *pfx);
int		cg_is_dir(const char *path);
char	*cg_join_prefix(const char *pfx, const char *name);
char	cg_action_of(const char *name);
size_t	cg_parse_opts(t_shell *st, t_vec argv, t_cgopt *o);
void	comp_free_spec(t_compspec *c);
void	comp_vec_push(t_shell *st, t_compspec *c);
void	comp_print_one(t_compspec *c);
void	comp_print_all(t_shell *st);
int		comp_remove(t_shell *st, t_vec argv, size_t i);
size_t	comp_parse_opts(t_shell *st, t_vec argv, t_cmpopt *o);

int		tx_test_binary(char **a);
int		db_eval_flat(char **av, int n);
t_shell	**db_state_cell(void);
int		db_regex_match(const char *str, const char *pat);
int		eval_bracketed(t_shell *st, char **av, int ac, int dbr);

void	update_pwd_vars(t_shell *state);
bool	is_redir_operator(char *s);
int		umask_symbolic(int flags);
int		umask_opts(char **av, size_t len, int *idx);
int		umask_report(mode_t m, int flags);
int		umask_sym_parse(const char *s, int initial);

/* cd: options (-L logical default / -P physical, -e, -@), parsed before the
   operand. echo => print the destination (CDPATH hit, `cd -`, or `cd a b`).
   quiet => zsh's -q: the move happens but the chpwd hooks do not fire. */
typedef struct s_cdopt
{
	bool	physical;
	bool	eflag;
	bool	atflag;
	bool	echo;
	bool	quiet;
}	t_cdopt;

int		cd_invalid_opt(t_shell *state, const char *arg);
int		cd_parse_opts(t_shell *state, t_vec argv, t_cdopt *o, size_t *first);
int		cd_collect_ops(t_vec argv, size_t first, char **op0, char **op1);
int		cd_target_home(t_shell *state, char **out);
int		cd_target_dash(t_shell *state, char **out, t_cdopt *o);
char	*cd_canonicalize(const char *path);
char	*cd_logical_path(t_shell *state, const char *target);
int		cd_apply(t_shell *state, t_cdopt *o, char *target);
char	*cd_cdpath(t_shell *state, char *op, bool *echo);
int		cd_two_arg(t_shell *state, t_cdopt *o, char *old, char *neww);

int		parse_redir_len(const char *arg);
bool	redir_needs_next(const char *arg);
char	*exe_path(char **path_dirs, char *exe_name, int *perm_denied);

int		list_traps(t_shell *state);
int		set_one_trap(t_shell *state, const char *action, int num);
int		trap_sig_from_name(const char *s);
int		print_traps_for(t_shell *state, t_vec argv);
char	*sig_to_name(int num);
int		trap_arg_is_reset(const char *s);
int		trap_reset_all(t_shell *state, t_vec argv, size_t i);
int		trap_set_all(t_shell *state, t_vec argv, size_t i);
void	print_one_trap(t_shell *state, int num);
int		hash_add_from_path(t_shell *state, const char *name);
int		handle_hash_flags(t_shell *state, char **av, int ac);

char	*dup_ifs(t_shell *state);
int		is_ifs(char c, const char *ifs);
int		is_ifs_ws(char c, const char *ifs);
char	*read_one_line(t_rdopt *o, int *eof);
int		rd_wait_input(t_rdopt *o);
bool	rd_at_delim(char ch, t_rdopt *o, bool bs);
char	*next_field(char **pp, const char *ifs, bool raw);
void	skip_delim(char **pp, const char *ifs);
char	*last_field(char *p, const char *ifs, bool raw);
size_t	parse_read_opts2(t_vec argv, t_rdopt *o);
void	rd_assign_array(t_shell *state, char *line, t_rdopt *o);
void	rd_set_var(t_shell *state, char *name, char *value_owned);
void	assign_words(t_shell *state, char *line, t_vec argv, t_rdopt *o);
long	rd_secs_ms(const char *s);
int		fc_resolve_idx(t_shell *state, const char *s);
int		fc_list(t_shell *state, char **av, int ac, bool reverse);
int		fc_write_tmp(t_shell *state, char *tmpf, int first, int last);
int		fc_run_editor(t_shell *state, const char *editor, char *tmpf);
int		fc_edit_run(t_shell *state, const char *editor, int first, int last);

int		kill_sig_from_name(const char *name);
int		kill_list_sigs(void);
int		kill_one_target(t_shell *state, const char *target, int sig);
void	gopt_set_char(t_shell *state, const char *name, char c);
void	gopt_set_name(t_shell *state, t_getopts *g, char c);
char	*gopt_arg(t_shell *state, t_vec argv, int idx);
void	gopt_commit_optind(t_shell *state, int optind);
int		gopt_want_arg(t_shell *state, t_vec argv, t_getopts *g, char *cur);
void	gopt_init(t_shell *state, t_vec argv, t_getopts *g);
int		one_option(t_shell *state, t_vec argv, t_getopts *g, char *cur);
int		type_one_path(t_shell *state, const char *name);
int		type_one(t_shell *state, const char *name);
int		type_is_builtin(const char *name);
int		type_is_keyword(const char *name);
int		type_find_in_path(t_shell *state, const char *name, char **out);
int		type_dispatch(t_shell *state, const char *name, char mode);
void	set_print_env(t_shell *state);
int		declare_nameref(t_shell *state, t_vec argv, size_t i);
int		declare_integer(t_shell *state, t_vec argv, size_t i);

struct s_sh
{
	const char		*name;
	unsigned int	bit;
};
bool	apply_flag_letters(t_shell *state, const char *w, bool *want_o);
t_ulim	*ulim_table(void);
void	ulimit_show(const t_ulim *u, int hard, int with_label);
int		ulimit_set(t_shell *st, const t_ulim *u, char *v, int hard);

int		list_set_options(t_shell *state);
/* shopt's action ('s' set, 'u' unset, 'p' print reusable, 0 print plain)
   travelling with its -q modifier. Bundled because -o handling needs both
   alongside state, argv and the argument index, and the norm allows a
   function four parameters. */
typedef struct s_shopt_act
{
	char	act;
	int		quiet;
}	t_shopt_act;

size_t	shopt_flags(t_vec argv, char *act, int *quiet, int *use_o);
int		shopt_setopt(t_shell *state, t_vec argv, size_t i, t_shopt_act a);
int		shopt_unknown(t_shell *state, const char *name, char act,
			int quiet);

/* wait plumbing shared between builtin_proc.c and builtin_proc2.c */
int		reaped_job_status(t_shell *state, pid_t pid);
int		wait_one(t_shell *state, const char *arg);
int		wait_n(t_shell *state);

/* history builtin internals (builtin_history*.c). */
void	hist_list_init(t_shell *state);
void	hist_push(t_shell *state, char *owned);
void	hist_rl_remove(t_shell *state, int idx);
int		hist_clear(t_shell *state);
int		hist_delete(t_shell *state, t_vec argv, int first);
int		hist_store(t_shell *state, t_vec argv, int first);
int		hist_expand_args(t_shell *state, t_vec argv, int first);
int		hist_fileop(t_shell *state, t_vec argv, t_histopt *o);
char	*expand_history(t_shell *state, const char *input);

/* pretty builtin internals (builtin_pretty*.c). */
t_pret	*pretty_table(void);
int		pretty_mode(t_shell *state, t_vec argv, int first);
int		pretty_show(t_shell *state, bool reusable);
int		pretty_list(t_shell *state);
void	glob_opts_sync(t_shell *state);

int		declare_func_attrs(t_shell *state, t_vec argv, size_t i);
int		declare_functions(t_shell *state, t_vec argv, size_t i,
			bool bodies);

void	dirstack_print(t_shell *state);

char	scan_term(const char *w);
int		declare_names(t_shell *state, t_vec argv, size_t i);
int		list_all(t_shell *state);
int		list_named(t_shell *state, t_vec argv, size_t i);

#endif
