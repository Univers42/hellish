/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   brace_expand.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BRACE_EXPAND_H
# define BRACE_EXPAND_H

# include "libft.h"

t_vec	brace_expand_str(const char *s);
int		brace_match(const char *s, int open);
int		brace_next(const char *s, int i);
bool	brace_group_opens_at(const char *s, int i, int *close);
int		brace_find_expandable(const char *s, int *close);
t_vec	brace_alternatives(const char *s, int open, int close);
bool	brace_gen_sequence(const char *body, t_vec *out);
void	free_str_elem(void *el);

#endif
