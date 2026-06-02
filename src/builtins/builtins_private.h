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
# include "env.h"
# include "libft.h"
# include "ft_builtins.h"
# include <stdlib.h>
# include "helpers.h"
# include "sh_input.h"

# define CD_ERROR "cd: error retrieving current directory: getcwd: \
				cannot access parent directories: \
				No such file or directory\n"
# define PWD_ERR_CUR_DIR "pwd: error retrieving current directory: \
				getcwd: cannot access parent directories: \
				No such file or directory\n"
# define OLDPWD_NO_SET "%s: cd: OLDPWD not set\n"
# define OLDPWD_NAME "OLDPWD"
# define PWD_NAME "PWD"

typedef struct s_rdopt
{
	char	*ifs;
	bool	raw;
	size_t	first;
}	t_rdopt;

typedef struct s_getopts
{
	char	*optstring;
	char	*name;
	int		optind;
	int		count;
	bool	silent;
}	t_getopts;

typedef struct s_ulim
{
	char		opt;
	int			res;
	long		scale;
	const char	*label;
}	t_ulim;

void	try_unset(t_shell *state, char *key);
int		builtin_unset(t_shell *state, t_vec argv);
int		parse_flags(t_vec argv, int *n, int *e);
int		print_args(int e, t_vec argv, size_t i);
int		builtin_echo(t_shell *state, t_vec argv);
int		builtin_env(t_shell *state, t_vec argv);
void	exit_clean(t_shell *state, int code);
int		builtin_exit(t_shell *state, t_vec argv);
void	parse_export_arg(char *str, char **ident, char **val);
char	strip_surrounding_quotes(char **val);
void	consume_following_value(t_vec av, int *i, char **val);
int		handle_identifier(t_shell *st, char *id, char *val, const char *argv0);
int		process_arg(t_shell *st, t_vec av, int *ip);
void	collect_and_print_exported(t_shell *st);
int		builtin_export(t_shell *st, t_vec av);
int		cd_home(int *e, t_shell *state);
int		builtin_pwd(t_shell *state, t_vec argv);
int		builtin_read(t_shell *state, t_vec argv);

void	print_exit_if_readline(t_shell *state);
int		handle_no_args(t_shell *state, t_vec argv);
size_t	handle_double_dash(t_shell *state, t_vec argv, size_t i);
int		handle_non_numeric(t_shell *state, t_vec argv, size_t i, int *ret);
char	*expand_export_value(t_shell *st, char *val, bool allow_expand);
bool	ft_is_valid_ident(char *id);

void	update_pwd_vars(t_shell *state);
bool	is_redir_operator(char *s);
int		count_real_args(t_vec argv);
char	*get_first_real_arg(t_vec argv);
int		check_args(t_vec argv);
int		cd_do_chdir(t_shell *state, t_vec argv, int *e);
void	cd_refresh_cwd(t_shell *state, t_vec argv, char *cwd);
int		handle_too_many_args(t_shell *state, t_vec argv, size_t i);

int		parse_redir_len(const char *arg);
bool	redir_needs_next(const char *arg);
char	*exe_path(char **path_dirs, char *exe_name, int *perm_denied);

int		list_traps(t_shell *state);
int		set_one_trap(t_shell *state, const char *action, int num);
char	*sig_to_name(int num);
int		hash_add_from_path(t_shell *state, const char *name);
int		handle_hash_flags(t_shell *state, char **av, int ac);

char	*dup_ifs(t_shell *state);
int		is_ifs(char c, const char *ifs);
int		is_ifs_ws(char c, const char *ifs);
char	*read_one_line(bool raw, int *eof);
char	*next_field(char **pp, const char *ifs, bool raw);
void	skip_delim(char **pp, const char *ifs);
char	*last_field(char *p, const char *ifs, bool raw);
void	rd_set_var(t_shell *state, char *name, char *value_owned);
void	assign_words(t_shell *state, char *line, t_vec argv, t_rdopt *o);
size_t	parse_read_opts(t_vec argv, bool *raw);
int		fc_resolve_idx(t_shell *state, const char *s);
int		fc_list(t_shell *state, char **av, int ac, bool reverse);
int		fc_write_tmp(t_shell *state, char *tmpf, int first, int last);
int		fc_run_editor(t_shell *state, const char *editor, char *tmpf);
int		fc_edit_run(t_shell *state, const char *editor, int first, int last);
int		handle_set_o(t_shell *state, t_vec argv);
int		kill_sig_from_name(const char *name);
int		kill_list_sigs(void);
int		kill_one_target(t_shell *state, const char *target, int sig);
void	gopt_set_char(t_shell *state, const char *name, char c);
char	*gopt_arg(t_shell *state, t_vec argv, int idx);
void	gopt_commit_optind(t_shell *state, int optind);
int		gopt_want_arg(t_shell *state, t_vec argv, t_getopts *g, char *cur);
void	gopt_init(t_shell *state, t_vec argv, t_getopts *g);
int		one_option(t_shell *state, t_vec argv, t_getopts *g, char *cur);
int		type_one_path(t_shell *state, const char *name);
int		type_one(t_shell *state, const char *name);
void	apply_flag_word(t_shell *state, const char *w);
t_ulim	*ulim_table(void);
void	ulimit_show(const t_ulim *u, int hard, int with_label);
int		ulimit_set(t_shell *st, const t_ulim *u, char *v, int hard);

#endif
