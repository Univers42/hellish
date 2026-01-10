/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vectors.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:29:26 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:29:26 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../shell.h"
# include <errno.h>
# include <string.h>
# include <fcntl.h>
# include <unistd.h>
# include <stdio.h> // for debug printing

/* Initialize vector (no allocation). */
bool	vec_init(t_vec *ret)
{
	*ret = (t_vec){0};
	ret->elem_size = 1;
	ret->ctx = NULL;
	return (true);
}

/* Ensure there's space for at least one more element. */
bool	vec_ensure_space(t_vec *v)
{
	size_t	es;
	size_t	new_cap;
	void	*new_buff;

	es = v->elem_size ? v->elem_size : 1;
	/* ensure elem_size is sane for arithmetic below */
	if (v->elem_size == 0)
		v->elem_size = 1;
	/* if there's already enough capacity, do nothing */
	if (v->ctx != NULL && v->len + 1 <= v->cap)
		return (true);
	/* compute an initial capacity that is at least 1 */
	{
		size_t base = 64 / es;
		if (64 % es)
			base += 1;
		if (base == 0)
			base = 1;
		new_cap = (v->cap == 0) ? base : (v->cap * 2);
	}
	new_buff = malloc(new_cap * es);
	if (new_buff == NULL)
		return (false);
	if (v->ctx != NULL)
	{
		memcpy(new_buff, v->ctx, v->len * es);
		free(v->ctx);
	}
	v->ctx = new_buff;
	v->cap = new_cap;
	return (true);
}

bool	vec_ensure_space_n(t_vec *v, size_t n)
{
	size_t	es;
    size_t	new_cap;
    void	*new_buff;

	es = v->elem_size ? v->elem_size : 1;
	if (v->elem_size == 0)
		v->elem_size = 1;
    if (n == 0)
        return (true);
    if (v->ctx != NULL && v->len + n <= v->cap)
        return (true);
    /* compute initial capacity as ceil(64 / es) or reuse v->cap */
    {
        size_t base = 64 / es;
        if (64 % es)
            base += 1;
        if (base == 0)
            base = 1;
        new_cap = (v->cap == 0) ? base : v->cap;
    }
    while (new_cap < v->len + n)
        new_cap = new_cap * 2 + 1;
    new_buff = malloc(new_cap * es);
    if (new_buff == NULL)
        return (false);
    if (v->ctx != NULL)
    {
        memcpy(new_buff, v->ctx, v->len * es);
        free(v->ctx);
    }
    v->ctx = new_buff;
    v->cap = new_cap;
    return (true);
}


/* Push element (copy from `el`). */
bool	vec_push(t_vec *v, const void *el)
{
#if TRACE_DEBUG
    // Debug: catch accidental elem_size==1 for AST node vectors
    // If you see this, you forgot to set elem_size = sizeof(t_ast_node)
    if (v->elem_size == 1 && v->cap > 0 && v->len < v->cap) {
        // Try to detect AST node pushes by size
        fprintf(stderr, "[minishell] ERROR: vec_push called with elem_size==1 for AST node vector. This will corrupt memory!\n");
        fprintf(stderr, "[minishell] Set v->elem_size = sizeof(t_ast_node) before using this vector.\n");
        fflush(stderr);
    }
#endif
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
	/* reserve one extra byte for '\0' when dealing with byte buffers */
	if (!vec_ensure_space_n(v, n + (v->elem_size == 1)))
		return (false);
	memcpy((char *)v->ctx + v->len * v->elem_size, el, n * v->elem_size);
	v->len += n;
	if (v->elem_size == 1)
		((char *)v->ctx)[v->len] = '\0';
	return (true);
}

bool	vec_push_nstr(t_vec *v, const char *str, size_t n)
{
    ft_assert(v != NULL && v->elem_size == 1 && str != NULL);
    /* ensure space for n bytes + terminating NUL */
    if (!vec_ensure_space_n(v, n + 1))
        return (false);
    if (n > 0)
        memcpy((char *)v->ctx + v->len, str, n);
    v->len += n;
    ((char *)v->ctx)[v->len] = '\0';
    return (true);
}

bool vec_push_str(t_vec *v, const char *str)
{
    if (str == NULL)
        return (false);
    return vec_push_nstr(v, str, ft_strlen(str));
}


/* Push a single byte (for byte-oriented vectors elem_size==1) */
bool	vec_push_byte(t_vec *v, unsigned char byte)
{
	if (v->elem_size == 0)
		v->elem_size = 1;
	if (!vec_ensure_space(v))
		return (false);
	((unsigned char *)v->ctx)[v->len++] = byte;
	/* Add null terminator for byte vectors, ensuring space was reserved */
	if (v->elem_size == 1 && v->len < v->cap)
		((unsigned char *)v->ctx)[v->len] = '\0';
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
#if TRACE_DEBUG
    // Debug: catch accidental elem_size==1 for AST node vectors
    if (v->elem_size == 1 && v->cap > 0) {
        fprintf(stderr, "[minishell] ERROR: vec_idx called with elem_size==1 for AST node vector. This will corrupt memory!\n");
        fprintf(stderr, "[minishell] Set v->elem_size = sizeof(t_ast_node) before using this vector.\n");
        fflush(stderr);
    }
#endif
    ft_assert(idx < v->len);
    return ((char *)v->ctx + idx * v->elem_size);
}

/* Provide vec_ctx_idx used by other modules (same as vec_idx) */
void	*vec_ctx_idx(t_vec *v, size_t idx)
{
	return (vec_idx(v, idx));
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
			if (errno == EINTR)
				continue;
			perror("read");
			exit(1);
		}
	}
	/* Ensure null terminator is present */
	if (ret->ctx && ret->elem_size == 1)
	{
		if (!vec_ensure_space_n(ret, 1))
			return;
		((char *)ret->ctx)[ret->len] = '\0';
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
	/* ensure ctx cleared */
	v->ctx = NULL;
}

bool	vec_ends_with_str(t_vec *v, const char *str)
{
	size_t	str_len;
	size_t	v_len;

	if (!v || !str || v->elem_size != 1)
		return (false);
	str_len = ft_strlen(str);
	v_len = v->len;
	if (str_len > v_len)
		return (false);
	return (ft_strncmp((char *)v->ctx + v_len - str_len, str, str_len) == 0);
}

void	vec_clear(t_vec *v)
{
	if (!v)
		return;
	v->len = 0;
	if (v->ctx && v->elem_size == 1)
		((char *)v->ctx)[0] = '\0';
}