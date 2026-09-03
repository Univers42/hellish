/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:48:03 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:48:03 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Miscellaneous helper functions used across multiple subsystems.
   free_all_state is the big cleanup called at exit or before exec.
   The var-name predicates (is_var_name_p*) are used by both the lexer
   and the env expand code -- they live here to avoid duplication. */

#ifndef HELPERS_H
# define HELPERS_H

# include "shell.h"
# include "alias.h"

/* Forward declarations to avoid circular dependency */
typedef struct executable_cmd_s		t_executable_cmd;
typedef struct executable_node_s	t_executable_node;
typedef struct s_execution_state	t_execution_state;

void	free_redirects(t_vec_redir *v);
void	free_all_state(t_shell *state);
void	free_executable_cmd(t_shell *state, t_executable_cmd cmd);
void	free_executable_node(t_executable_node *node);
void	free_tab(char **tab);
int		write_to_file(char *str, int fd);
void	forward_exit_status(t_execution_state res);
void	set_cmd_status(t_shell *state, t_execution_state res);
int		ft_checked_atoi(const char *str, int *ret, int flags);
bool	is_var_name_p1(char c);
int		sh_skip_quoted(const char *s, int len, int i);
bool	is_var_name_p2(char c);
char	*sq_quote(const char *val);

# ifdef VERBOSE

void	verbose(int flag, const char *str, ...);

# endif

static inline bool	vec_str_ends_with_str(t_string *s, char *s2)
{
	size_t	len_s2;

	len_s2 = ft_strlen(s2);
	if (s->len < len_s2)
		return (false);
	if (ft_strcmp(((char *)s->ctx) + s->len - len_s2, s2) == 0)
		return (true);
	return (false);
}

/* Config load path (src/core/rc_load*.c). */
bool	str_ends_with(const char *s, const char *suf);
char	*path_join(const char *a, const char *b);
void	sort_strvec(t_vec *v, size_t from);
char	*xdg_config_hellish(t_shell *state, const char *home);
void	collect(const char *dir, const char *suffix, t_vec *out);
void	collect_plugins(const char *dir, t_vec *out);
void	rc_load_all(t_shell *state, const char *home);
void	rc_load_after(t_shell *state, const char *home);

/* Composable rc hooks (src/core/hooks*.c). */
void	run_hook_funcs(t_shell *state, char *var, const char *arg);
void	hook_run_one(t_shell *state, const char *fn, const char *arg);
void	run_zsh_prompt_hooks(t_shell *state, const char *which,
			const char *arg);
void	run_preexec(t_shell *state);

#endif
