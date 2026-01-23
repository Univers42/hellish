/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:04:45 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 15:08:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ENV_H
# define ENV_H

# include <stddef.h>
# include <stdbool.h>

typedef struct s_shell	t_shell;
typedef struct s_env	t_env;
typedef struct s_vec	t_vec;
typedef struct s_vec	t_vec_env;

t_env	env_create(char *key, char *value, bool exported);
char	*env_expand(t_shell *state, char *key);
char	**env_to_vec_env(t_shell *state, char **envp);
void	free_tab(char **tab);
void	init_cwd(t_shell *state);
void	set_home(t_shell *state);
void	set_cwd(t_shell *state);
void	set_shlvl(t_shell *state);
void	set_path(t_shell *state);
void	set_underscore(t_shell *state);
void	ensure_essential_env_vars(t_shell *state);
#endif
