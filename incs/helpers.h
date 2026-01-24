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

#ifndef HELPERS_H
# define HELPERS_H

# include "shell.h"
# include "alias.h"

// Forward declarations to avoid circular dependency
typedef struct executable_cmd_s		t_executable_cmd;
typedef struct executable_node_s	t_executable_node;
typedef struct s_exe_res			t_exe_res;

void	free_redirects(t_vec_redir *v);
void	free_all_state(t_shell *state);
void	free_executable_cmd(t_executable_cmd cmd);
void	free_executable_node(t_executable_node *node);
char	*getpid_hack(void);
void	free_tab(char **tab);
int		write_to_file(char *str, int fd);
void	forward_exit_status(t_exe_res res);
void	set_cmd_status(t_shell *state, t_exe_res res);
int		ft_checked_atoi(const char *str, int *ret, int flags);
bool	is_var_name_p1(char c);
bool	is_var_name_p2(char c);

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

#endif
