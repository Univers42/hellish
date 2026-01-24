/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:04:29 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 18:44:12 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HELPERS_H
# define HELPERS_H

# include <stdlib.h>

/* forward declare shell */
typedef struct s_shell	t_shell;

void	free_all_state(t_shell *state);
void	manage_history(t_shell *state);

#endif
