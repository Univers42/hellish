/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtins.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:25 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:25 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_BUILTINS_H
# define FT_BUILTINS_H

# include "libft.h"
# include "shell.h"
# include "helpers.h"

typedef int	(*t_builtin_fn)(t_shell *state, t_vec argv);

int		cd_home(int *e, t_shell *state);
void	parse_numeric_escape(char **str);
int		e_parser(char *str);
int		parse_flags(t_vec argv, int *n, int *e);
int		print_args(int e, t_vec argv, size_t i);
void	try_unset(t_shell *state, char *key);
int		builtin_echo(t_shell *state, t_vec argv);
int		builtin_pwd(t_shell *state, t_vec argv);
int		builtin_exit(t_shell *state, t_vec argv);
int		builtin_cd(t_shell *state, t_vec argv);
int		builtin_env(t_shell *state, t_vec argv);
int		builtin_export(t_shell *state, t_vec argv);
int		builtin_unset(t_shell *state, t_vec argv);
int		builtin_type(t_shell *state, t_vec argv);
int		builtin_set(t_shell *state, t_vec argv);
int		builtin_shift(t_shell *state, t_vec argv);
int		set_positional_args(t_shell *state, char **args, size_t n);
int		apply_set_flags(t_shell *state, t_vec argv);
int		set_long_option(t_shell *state, char sign, const char *name);
void	xtrace_print(t_vec *argv);
void	nounset_abort(t_shell *state, const char *name, int len);
void	scope_save(t_shell *state, const char *key);
void	scope_leave(t_shell *state);
t_vec	apply_temp_assigns(t_shell *state, t_vec *pre);
void	restore_temp_assigns(t_shell *state, t_vec *saves);
int		builtin_local(t_shell *state, t_vec argv);
int		builtin_colon(t_shell *state, t_vec argv);
int		builtin_break(t_shell *state, t_vec argv);
int		builtin_continue(t_shell *state, t_vec argv);
int		builtin_eval(t_shell *state, t_vec argv);
int		builtin_source(t_shell *state, t_vec argv);
int		builtin_true(t_shell *state, t_vec argv);
int		builtin_false(t_shell *state, t_vec argv);
int		builtin_umask(t_shell *state, t_vec argv);
int		builtin_command(t_shell *state, t_vec argv);
int		builtin_return(t_shell *state, t_vec argv);
int		builtin_getopts(t_shell *state, t_vec argv);
int		builtin_exec(t_shell *state, t_vec argv);
int		builtin_wait(t_shell *state, t_vec argv);
int		builtin_times(t_shell *state, t_vec argv);
int		builtin_trap(t_shell *state, t_vec argv);
int		builtin_readonly(t_shell *state, t_vec argv);
void	run_pending_traps(t_shell *state);
void	run_exit_trap(t_shell *state);
bool	is_readonly_var(t_shell *state, const char *key);

typedef struct s_signame
{
	const char	*name;
	int			num;
}	t_signame;
int		builtin_test(t_shell *state, t_vec argv);
int		builtin_alias(t_shell *state, t_vec argv);
int		builtin_unalias(t_shell *state, t_vec argv);
int		builtin_hash(t_shell *state, t_vec argv);
int		builtin_jobs(t_shell *state, t_vec argv);
int		builtin_fg(t_shell *state, t_vec argv);
int		builtin_bg(t_shell *state, t_vec argv);
int		builtin_fc(t_shell *state, t_vec argv);
int		builtin_history(t_shell *state, t_vec argv);
int		builtin_let(t_shell *state, t_vec argv);
int		builtin_kill(t_shell *state, t_vec argv);
int		builtin_printf(t_shell *state, t_vec argv);
int		(*builtin_func(char *name))(t_shell *state, t_vec argv);

#endif