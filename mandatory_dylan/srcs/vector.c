/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vector.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 16:01:56 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 16:01:56 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../incs/dsa.h"

/* Initialize vector (no allocation). */
bool	vec_init(t_vec *ret)
{
	*ret = (t_vec){0};
	ret->elem_size = 1;
	return (true);
}

/* Ensure capacity for at least one more element. */
bool	vec_ensure_space(t_vec *v)
{
	size_t	new_cap;
	void	*new_buff;

	if (v->len + 1 < v->cap && v->ctx != NULL)
		return (true);
	new_cap = (v->cap == 0) ? (64 / (v->elem_size ? v->elem_size : 1)) : (v->cap * 2);
	new_buff = malloc(new_cap * v->elem_size);
	if (new_buff == NULL)
		return (false);
	if (v->ctx != NULL)
	{
		memcpy(new_buff, v->ctx, v->len * v->elem_size);
		free(v->ctx);
	}
	v->ctx = new_buff;
	v->cap = new_cap;
	return (true);
}

bool	vec_ensure_space_n(t_vec *v, size_t n)
{
    size_t	new_cap;
    void	*new_buff;

    if (n == 0)
        return (true);
    if (v->ctx != NULL && v->len + n <= v->cap)
        return (true);
    new_cap = (v->cap == 0) ? (64 / (v->elem_size ? v->elem_size : 1)) : v->cap;
    while (new_cap < v->len + n)
        new_cap = new_cap * 2 + 1;
    new_buff = malloc(new_cap * v->elem_size);
    if (new_buff == NULL)
        return (false);
    if (v->ctx != NULL)
    {
        memcpy(new_buff, v->ctx, v->len * v->elem_size);
        free(v->ctx);
    }
    v->ctx = new_buff;
    v->cap = new_cap;
    return (true);
}


/* Push element (copy from `el`). */
bool	vec_push(t_vec *v, const void *el)
{
    if (!vec_ensure_space(v))
        return (false);
    memcpy((char *)v->ctx + v->len * v->elem_size, el, v->elem_size);
    v->len++;
    return (true);
}

bool	vec_npush(t_vec *v, const void *el, size_t n)
{
    if (n == 0)
        return (true);
    if (!vec_ensure_space_n(v, n))
        return (false);
    memcpy((char *)v->ctx + v->len * v->elem_size, el, n * v->elem_size);
    v->len += n;
    return (true);
}

bool	vec_push_nstr(t_vec *v, const char *str, size_t n)
{
    ft_assert(v != NULL && v->elem_size == 1 && str != NULL);
    /* ensure space for n bytes + terminating NUL */
    if (!vec_ensure_space_n(v, n + 1))
        return (false);
    memcpy((char *)v->ctx + v->len, str, n);
    v->len += n;
    ((char *)v->ctx)[v->len] = '\0';
    return (true);
}

bool vec_push_str(t_vec *v, const char *str)
{
    return vec_push_nstr(v, str, ft_strlen(str));
}


/* Push a single byte (for byte-oriented vectors elem_size==1) */
bool	vec_push_byte(t_vec *v, unsigned char byte)
{
	if (!vec_ensure_space(v))
		return (false);
	((unsigned char *)v->ctx)[v->len++] = byte;
	return (true);
}

/* Pop element and return pointer into internal buffer (valid until reallocation). */
void	*vec_pop(t_vec *v)
{
	ft_assert(v->len > 0);
	v->len--;
	return ((char *)v->ctx + v->len * v->elem_size);
}


/* Return pointer to element at idx. */
void	*vec_idx(t_vec *v, size_t idx)
{
	ft_assert(idx < v->len);
	return ((char *)v->ctx + idx * v->elem_size);
}

/* append all data read from fd into a byte-oriented vector (elem_size==1) */
void vec_append_fd(int fd, t_vec *ret)
{
	char	buf[1024];
	ssize_t	n;

	if (!ret)
		return ;
	while (1)
	{
		n = read(fd, buf, sizeof(buf));
		if (n == 0)
			break ;
		if (n > 0)
			vec_npush(ret, buf, (size_t)n);
		else
		{
			perror("read");
			exit(1);
		}
	}
}

/* Free elements with user callback (if provided), free buffer and reset struct. */
void	vec_destroy(t_vec *v, void (*free_elem)(void *))
{
	size_t i;

	if (v == NULL)
		return ;
	if (free_elem != NULL && v->ctx != NULL)
	{
	 i = 0;
	 while (i < v->len)
	 {
		 void *el = (char *)v->ctx + i * v->elem_size;
		 free_elem(el);
		 i++;
	 }
	}
	free(v->ctx);
	*v = (t_vec){0};
}

void	vec_clear(t_vec *v)
{
	v->cap = 0;
	v->len = 0;
	v->elem_size = 0;
	
}