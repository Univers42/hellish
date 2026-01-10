/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   deque.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 01:49:10 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 01:49:10 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DEQUE_H
# define DEQUE_H

# include "common.h"

typedef struct s_deque_s
{
	size_t	cap;
	size_t	len;
	size_t	start;
	size_t	end;
	size_t	elem_size;
	void	*buff;
}	ft_deque;

/* NOTE: change deque API to accept a generic pointer (void *) so callers
   may pass either ft_deque* or a wrapper t_deque_tt* whose first member is
   an ft_deque. This preserves existing call sites. */
bool	ft_deque_init(void *ret, int size, size_t elem_size);
void	ft_deque_ensure_space(void *ret);
bool	ft_deque_push_end(void *ret, const void *item);
bool	ft_deque_push_start(void *ret, const void *item);
void	*ft_deque_pop_end(void *ret);
void	*ft_deque_pop_start(void *ret);
void	*ft_deque_idx(void *ret, size_t idx);
void	*ft_deque_peek(void *ret);
void	ft_deque_peek_into(void *ret, void *out);
void	ft_deque_reset(void *ret);
void	ft_deque_clone(void *ret, const void *proto);
void	ft_deque_destroy(void *ret, void (*free_elem)(void *));
void	ft_deque_clear(void *ret, void (*free_elem)(void *));


# endif