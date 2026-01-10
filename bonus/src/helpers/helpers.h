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

void	free_redirects(t_vec_redir *v);
void	free_all_state(t_state *state);
void	free_executable_cmd(t_executable_cmd cmd);
void	free_executable_node(t_executable_node *node);
char	*getpid_hack(void);
void	free_tab(char **tab);
int		write_to_file(char *str, int fd);
void	forward_exit_status(t_exe_res res);
void	set_cmd_status(t_state *state, t_exe_res res);

#endif