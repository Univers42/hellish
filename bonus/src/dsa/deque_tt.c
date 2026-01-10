/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   deque_tt.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:29:34 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:29:34 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include <stdlib.h>
#include <string.h>
#include "../shell.h"

/*
 * Initialize deque. If size > 0, allocates backing buffer for `size` elements.
 * Returns true on success.
 */
bool	ft_deque_init(void *vret, int size, size_t elem_size)
{
	ft_deque *ret = (ft_deque *)vret;
	*ret = (ft_deque){0};
	ret->elem_size = elem_size;
	if (size > 0)
	{
		ret->buff = malloc((size_t)size * elem_size);
		if (ret->buff == NULL)
			return (false);
		ret->cap = (size_t)size;
		ret->end = ret->cap - 1;
	}
	return (true);
}

/* Ensure there's space for one more element, grow if needed. */
void	ft_deque_ensure_space(void *vret)
{
	ft_deque *ret = (ft_deque *)vret;
	size_t	new_cap;
	void	*new_buff;
	size_t	i;

	if (ret->len < ret->cap)
		return ;
	new_cap = (ret->cap == 0) ? (64 / (ret->elem_size ? ret->elem_size : 1)) : (ret->cap * 2 + 1);
	new_buff = malloc(new_cap * ret->elem_size);
	ft_assert(new_buff != NULL);
	i = 0;
	while (i < ret->len)
	{
		memcpy((char *)new_buff + i * ret->elem_size,
			   (char *)ret->buff + ((ret->start + i) % ret->cap) * ret->elem_size,
			   ret->elem_size);
		i++;
	}
	free(ret->buff);
	ret->buff = new_buff;
	ret->cap = new_cap;
	ret->start = 0;
	ret->end = (ret->len == 0) ? 0 : (ret->len - 1);
}

/* Push element at end. `item` points to the source element memory. */
bool	ft_deque_push_end(void *vret, const void *item)
{
	ft_deque *ret = (ft_deque *)vret;
	ft_deque_ensure_space(ret);
	ft_assert(ret->cap > ret->len || ret->cap != 0);
	ret->end = (ret->cap == 0) ? 0 : (ret->end + 1) % ret->cap;
	memcpy((char *)ret->buff + ret->end * ret->elem_size, item, ret->elem_size);
	ret->len++;
	return (true);
}

/* Push element at start. `item` points to the source element memory. */
bool	ft_deque_push_start(void *vret, const void *item)
{
	ft_deque *ret = (ft_deque *)vret;
	ft_deque_ensure_space(ret);
	if (ret->len)
		ret->start = ft_smod((int)ret->start - 1, (int)ret->cap);
	else
		ret->start = 0;
	memcpy((char *)ret->buff + ret->start * ret->elem_size, item, ret->elem_size);
	ret->len++;
	return (true);
}

/* Pop element from end. Returns pointer into internal buffer (valid until next reallocation). */
void	*ft_deque_pop_end(void *vret)
{
	ft_deque *ret = (ft_deque *)vret;
	void	*src;
	ft_assert(ret->len > 0);
	src = (char *)ret->buff + ret->end * ret->elem_size;
	ret->end = ft_smod((int)ret->end - 1, (int)ret->cap);
	ret->len--;
	return (src);
}

/* Pop element from start. Returns pointer into internal buffer (valid until next reallocation). */
void	*ft_deque_pop_start(void *vret)
{
	ft_deque *ret = (ft_deque *)vret;
	void	*src;
	ft_assert(ret->len > 0);
	src = (char *)ret->buff + ret->start * ret->elem_size;
	ret->start = (ret->start + 1) % ret->cap;
	ret->len--;
	return (src);
}

/* Index into deque; returns pointer into internal buffer. */
void	*ft_deque_idx(void *vret, size_t idx)
{
	ft_deque *ret = (ft_deque *)vret;
	ft_assert(idx < ret->len);
	return ((char *)ret->buff + ((ret->start + idx) % ret->cap) * ret->elem_size);
}

void *ft_deque_peek(void *vret)
{
	return (ft_deque_idx(vret, 0));
}

void ft_deque_peek_into(void *vret, void *out)
{
	ft_deque *ret = (ft_deque *)vret;
	ft_assert(ret != NULL && out != NULL);
	memcpy(out, ft_deque_idx(ret, 0), ret->elem_size);
}

void	ft_deque_reset(void *vret)
{
	ft_deque *ret = (ft_deque *)vret;
	/* keep buffer untouched but reset logical state */
	if (!ret)
		return;
	ret->start = 0;
	ret->len = 0;
	ret->end = (ret->cap == 0) ? 0 : (ret->cap - 1);
}

/* Clone proto into ret (allocates). */
void	ft_deque_clone(void *vret, const void *vproto)
{
	ft_deque *ret = (ft_deque *)vret;
	const ft_deque *proto = (const ft_deque *)vproto;

	if (!ft_deque_init(ret, (int)proto->cap, proto->elem_size))
		return ;
	if (proto->len == 0)
		return ;
	/* place elements contiguously */
	ret->len = proto->len;
	ret->start = 0;
	ret->end = ret->len - 1;
	memcpy(ret->buff, proto->buff, proto->len * proto->elem_size);
	return ;
}

/* Free elements with user callback (if provided), free buffer and reset struct. */
void	ft_deque_destroy(void *vret, void (*free_elem)(void *))
{
	ft_deque *ret = (ft_deque *)vret;
	size_t	i;

	if (ret == NULL)
		return ;
	if (free_elem != NULL && ret->buff != NULL && ret->cap != 0)
	{
		i = 0;
		while (i < ret->len)
		{
			void *el = (char *)ret->buff + ((ret->start + i) % ret->cap) * ret->elem_size;
			free_elem(el);
			i++;
		}
	}
	free(ret->buff);
	*ret = (ft_deque){0};
}

/* Clear deque contents but keep allocated buffer.
 * If free_elem != NULL, call it on each element before clearing.
 */
void	ft_deque_clear(void *vret, void (*free_elem)(void *))
{
	ft_deque *ret = (ft_deque *)vret;
	size_t	i;

	if (ret == NULL)
		return ;
	if (free_elem != NULL && ret->buff != NULL && ret->cap != 0)
	{
		i = 0;
		while (i < ret->len)
		{
			void *el = (char *)ret->buff + ((ret->start + i) % ret->cap) * ret->elem_size;
			free_elem(el);
			i++;
		}
	}
	/* keep buffer allocated, reset indices/length */
	ret->len = 0;
	ret->start = 0;
	ret->end = (ret->cap == 0) ? 0 : (ret->cap - 1);
}
