/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   deque.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 16:01:50 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 16:01:50 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../incs/dsa.h"

/*
 * Initialize deque. If size > 0, allocates backing buffer for `size` elements.
 * Returns true on success.
 */
bool	deque_init(t_deque *ret, int size, size_t elem_size)
{
	*ret = (t_deque){0};
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
void	deque_ensure_space(t_deque *ret)
{
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
bool	deque_push_end(t_deque *ret, const void *item)
{
	deque_ensure_space(ret);
	ft_assert(ret->cap > ret->len || ret->cap != 0);
	ret->end = (ret->cap == 0) ? 0 : (ret->end + 1) % ret->cap;
	memcpy((char *)ret->buff + ret->end * ret->elem_size, item, ret->elem_size);
	ret->len++;
	return (true);
}

/* Push element at start. `item` points to the source element memory. */
bool	deque_push_start(t_deque *ret, const void *item)
{
	deque_ensure_space(ret);
	if (ret->len)
		ret->start = ft_smod((int)ret->start - 1, (int)ret->cap);
	else
		ret->start = 0;
	memcpy((char *)ret->buff + ret->start * ret->elem_size, item, ret->elem_size);
	ret->len++;
	return (true);
}

/* Pop element from end. Returns pointer into internal buffer (valid until next reallocation). */
void	*deque_pop_end(t_deque *ret)
{
	void	*src;
	ft_assert(ret->len > 0);
	src = (char *)ret->buff + ret->end * ret->elem_size;
	ret->end = ft_smod((int)ret->end - 1, (int)ret->cap);
	ret->len--;
	return (src);
}

/* Pop element from start. Returns pointer into internal buffer (valid until next reallocation). */
void	*deque_pop_start(t_deque *ret)
{
	void	*src;
	ft_assert(ret->len > 0);
	src = (char *)ret->buff + ret->start * ret->elem_size;
	ret->start = (ret->start + 1) % ret->cap;
	ret->len--;
	return (src);
}

/* Index into deque; returns pointer into internal buffer. */
void	*deque_idx(t_deque *ret, size_t idx)
{
	ft_assert(idx < ret->len);
	return ((char *)ret->buff + ((ret->start + idx) % ret->cap) * ret->elem_size);
}

void *deque_peek(t_deque *ret)
{
	return (deque_idx(ret, 0));
}

void deque_peek_into(t_deque *ret, void *out)
{
	ft_assert(ret != NULL && out != NULL);
	memcpy(out, deque_idx(ret, 0), ret->elem_size);
}

void	deque_reset(t_deque *ret)
{
	/* keep buffer untouched but reset logical state */
	if (!ret)
		return;
	ret->start = 0;
	ret->len = 0;
	ret->end = (ret->cap == 0) ? 0 : (ret->cap - 1);
}

/* Clone proto into ret (allocates). Returns true on success. */
bool	deque_clone(t_deque *ret, const t_deque proto)
{
	if (!deque_init(ret, (int)proto.cap, proto.elem_size))
		return (false);
	if (proto.len == 0)
		return (true);
	/* place elements contiguously */
	ret->len = proto.len;
	ret->start = 0;
	ret->end = ret->len - 1;
	memcpy(ret->buff, proto.buff, proto.len * proto.elem_size);
	return (true);
}

/* Free elements with user callback (if provided), free buffer and reset struct. */
void	deque_destroy(t_deque *ret, void (*free_elem)(void *))
{
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
	*ret = (t_deque){0};
}

/* Clear deque contents but keep allocated buffer.
 * If free_elem != NULL, call it on each element before clearing.
 */
void	deque_clear(t_deque *ret, void (*free_elem)(void *))
{
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