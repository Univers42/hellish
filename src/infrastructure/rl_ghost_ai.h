/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_ghost_ai.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RL_GHOST_AI_H
# define RL_GHOST_AI_H

# include <stddef.h>

/* State for the asynchronous AI ghost-text. One in-flight worker at a time:
   `line` is what we fired for, `sug` the full suggested command once it lands,
   `pid`/`fd` the worker + its non-blocking pipe, `fired` debounces to one
   request per settled line. */
typedef struct s_aig
{
	char	line[512];
	char	sug[1024];
	int		pid;
	int		fd;
	int		fired;
}	t_aig;

t_aig		*aig(void);
void		aig_reset(t_aig *a);
void		aig_fire(t_aig *a, const char *line);
void		aig_poll(t_aig *a);
const char	*ai_ghost_get(const char *line);
int			rl_ai_event(void);
int			ai_history_has(const char *line);
void		ghost_redisplay(void);
int			rl_ghost_accept(int count, int key);
void		setup_ai_input(void);
int			cmd_resolvable(const char *s);

#endif
