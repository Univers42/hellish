/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dsa.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 17:01:29 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 17:01:29 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DSA_H
# define DSA_H

# include "common.h"

/* Provide only an opaque token/type declaration here to avoid conflicts with
   lexer.h which defines the concrete token type. Use void* for embedded token
   storage so this header can be included early. */
typedef void *t_token;
typedef int  t_tt;

# ifndef DSA_DEQUE
#  define DSA_DEQUE
typedef struct s_deque
{
	size_t	cap;
	size_t	len;
	size_t	start;
	size_t	end;
	size_t	elem_size;
	void	*buff;
}	t_deque;

/*
 * Initialize deque. If size > 0, allocates backing buffer for `size` elements.
 * Returns true on success.
 */
static inline bool	deque_init(t_deque *ret, int size, size_t elem_size)
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
static inline void	deque_ensure_space(t_deque *ret)
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
static inline bool	deque_push_end(t_deque *ret, const void *item)
{
	deque_ensure_space(ret);
	ft_assert(ret->cap > ret->len || ret->cap != 0);
	ret->end = (ret->cap == 0) ? 0 : (ret->end + 1) % ret->cap;
	memcpy((char *)ret->buff + ret->end * ret->elem_size, item, ret->elem_size);
	ret->len++;
	return (true);
}

/* Push element at start. `item` points to the source element memory. */
static inline bool	deque_push_start(t_deque *ret, const void *item)
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
static inline void	*deque_pop_end(t_deque *ret)
{
	void	*src;
	ft_assert(ret->len > 0);
	src = (char *)ret->buff + ret->end * ret->elem_size;
	ret->end = ft_smod((int)ret->end - 1, (int)ret->cap);
	ret->len--;
	return (src);
}

/* Pop element from start. Returns pointer into internal buffer (valid until next reallocation). */
static inline void	*deque_pop_start(t_deque *ret)
{
	void	*src;
	ft_assert(ret->len > 0);
	src = (char *)ret->buff + ret->start * ret->elem_size;
	ret->start = (ret->start + 1) % ret->cap;
	ret->len--;
	return (src);
}

/* Index into deque; returns pointer into internal buffer. */
static inline void	*deque_idx(t_deque *ret, size_t idx)
{
	ft_assert(idx < ret->len);
	return ((char *)ret->buff + ((ret->start + idx) % ret->cap) * ret->elem_size);
}

static inline void *deque_peek(t_deque *ret)
{
	return (deque_idx(ret, 0));
}

static inline void deque_peek_into(t_deque *ret, void *out)
{
	ft_assert(ret != NULL && out != NULL);
	memcpy(out, deque_idx(ret, 0), ret->elem_size);
}

static inline void	deque_reset(t_deque *ret)
{
	/* keep buffer untouched but reset logical state */
	if (!ret)
		return;
	ret->start = 0;
	ret->len = 0;
	ret->end = (ret->cap == 0) ? 0 : (ret->cap - 1);
}

/* Clone proto into ret (allocates). Returns true on success. */
static inline bool	deque_clone(t_deque *ret, const t_deque proto)
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
static inline void	deque_destroy(t_deque *ret, void (*free_elem)(void *))
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
static inline void	deque_clear(t_deque *ret, void (*free_elem)(void *))
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
# endif // DSA_DEQUE

# ifndef DSA_VEC
#  define DSA_VEC
typedef struct s_vec
{
	size_t	cap;
	size_t	len;
	size_t	elem_size;
	void	*ctx;
}t_vec;

typedef t_vec	t_string;		// a single dynamic string
typedef t_vec	t_obj_str;		// elements objects of string
typedef t_vec	t_strings;		// array of C-string pointers (references)
typedef t_vec	t_env;			// environment: vector of t_env_entry structs (defined in env.h)
typedef t_vec	t_ints;

/* Initialize vector (no allocation). */
static inline bool	vec_init(t_vec *ret)
{
	*ret = (t_vec){0};
	ret->elem_size = 1;
	return (true);
}

/* Ensure capacity for at least one more element. */
static inline bool	vec_ensure_space(t_vec *v)
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

static inline bool	vec_ensure_space_n(t_vec *v, size_t n)
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
static inline bool	vec_push(t_vec *v, const void *el)
{
    if (!vec_ensure_space(v))
        return (false);
    memcpy((char *)v->ctx + v->len * v->elem_size, el, v->elem_size);
    v->len++;
    return (true);
}

static inline bool	vec_npush(t_vec *v, const void *el, size_t n)
{
    if (n == 0)
        return (true);
    if (!vec_ensure_space_n(v, n))
        return (false);
    memcpy((char *)v->ctx + v->len * v->elem_size, el, n * v->elem_size);
    v->len += n;
    return (true);
}

static inline bool	vec_push_nstr(t_vec *v, const char *str, size_t n)
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

static inline bool vec_push_str(t_vec *v, const char *str)
{
    return vec_push_nstr(v, str, ft_strlen(str));
}


/* Push a single byte (for byte-oriented vectors elem_size==1) */
static inline bool	vec_push_byte(t_vec *v, unsigned char byte)
{
	if (!vec_ensure_space(v))
		return (false);
	((unsigned char *)v->ctx)[v->len++] = byte;
	return (true);
}

/* Pop element and return pointer into internal buffer (valid until reallocation). */
static inline void	*vec_pop(t_vec *v)
{
	ft_assert(v->len > 0);
	v->len--;
	return ((char *)v->ctx + v->len * v->elem_size);
}


/* Return pointer to element at idx. */
static inline void	*vec_ctx_idx(t_vec *v, size_t idx)
{
	ft_assert(idx < v->len);
	return ((char *)v->ctx + idx * v->elem_size);
}

/* append all data read from fd into a byte-oriented vector (elem_size==1) */
static inline void vec_append_fd(int fd, t_vec *ret)
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

/* keep existing vec_ctx_idx but provide the expected alias vec_idx(...) */
static inline void	*vec_idx(t_vec *v, size_t idx)
{
	return (vec_ctx_idx(v, idx));
}

/* Free elements with user callback (if provided), free buffer and reset struct. */
static inline void	vec_destroy(t_vec *v, void (*free_elem)(void *))
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
# endif // DSA_VEC

#ifndef TREE_STRUCTURE
# define TREE_STRUCTURE



typedef enum e_ast_type
{
	AST_SIMPLE_LIST,
	AST_COMMAND_PIPELINE,
	AST_REDIRECT,
	AST_SIMPLE_COMMAND,
	AST_SUBSHELL,
	AST_COMPOUND_LIST,
	AST_COMMAND,
	AST_WORD,
	AST_ASSIGNMENT_WORD,
	AST_TOKEN
}	t_ast_type;

typedef struct s_ast_node
{
	t_ast_type	node_type;
	t_token		token; /* forward-declared; real layout in lexer.h */
	bool		has_redirect;
	int			redir_idx;
	t_vec		children;
}	t_ast_node;

/* take as context the t_ast_node itself */
typedef t_vec	t_node;

static inline void	ast_postorder_traversal(t_ast_node *node, void (*fn)(t_ast_node *node))
{
	size_t	i;

	if (!node || !fn)
		return ;
	i = 0;
	while (i < node->children.len)
	{
		ast_postorder_traversal((t_ast_node *)vec_ctx_idx(&node->children, i), fn);
		i++;
	}
	fn(node);
}

static inline void	free_node(t_ast_node *node)
{
	/* avoid accessing token internals here (forward-declared) — construct minimal node */
	if (!node)
		return ;
	free(node->children.ctx);
	*node = (t_ast_node){0};
}

static inline void	free_ast(t_ast_node *node)
{
	ast_postorder_traversal(node, free_node);
}

/* Helper renamed to avoid C-name collision and match correct signature */
static inline t_ast_node	make_ast_token(t_token t, int offset, int end_offset, t_tt tt)
{
	t_ast_node	ret;

	/* Cannot access token internals here (forward-declared) — construct minimal node */
	ret = (t_ast_node){0};
	ret.node_type = AST_TOKEN;
	/* callers expecting token fields must be compiled after lexer.h */
	(void)t; (void)offset; (void)end_offset; (void)tt;
	return (ret);
}

#endif //AST

# ifndef HASH
#  define HASH

typedef struct s_hash_entry
{
	char	*key;
	void	*value;
}	t_hash_entry;

typedef struct s_hash
{
	size_t			cap;
	size_t			len;
	t_hash_entry	*buff;
}	t_hash;

/* Initialize hash table with given capacity. */
static inline bool	hash_init(t_hash *h, size_t capacity)
{
	*h = (t_hash){0};
	if (capacity == 0)
		capacity = 64;
	h->buff = calloc(capacity, sizeof(t_hash_entry));
	if (h->buff == NULL)
		return (false);
	h->cap = capacity;
	return (true);
}

/* Simple hash function for strings. */
static inline size_t	hash_func(const char *key, size_t cap)
{
	size_t	hash;

	hash = 5381;
	while (*key)
		hash = ((hash << 5) + hash) + (unsigned char)*key++;
	return (hash % cap);
}

/* Find entry by key. Returns index or -1 if not found. */
static inline int	hash_find_idx(t_hash *h, const char *key)
{
	size_t	idx;
	size_t	i;

	if (h->cap == 0 || key == NULL)
		return (-1);
	idx = hash_func(key, h->cap);
	i = 0;
	while (i < h->cap)
	{
		if (h->buff[idx].key != NULL && ft_strcmp(h->buff[idx].key, key) == 0)
			return ((int)idx);
		idx = (idx + 1) % h->cap;
		i++;
	}
	return (-1);
}

/* --- HASH: ensure hash_set is visible before hash_resize uses it --- */
static inline bool	hash_set(t_hash *h, const char *key, void *value);

/* Resize hash table and rehash entries. */
static inline bool	hash_resize(t_hash *h)
{
	t_hash		new_h;
	size_t		i;

	if (!hash_init(&new_h, h->cap * 2))
		return (false);
	i = 0;
	while (i < h->cap)
	{
		if (h->buff[i].key != NULL)
			hash_set(&new_h, h->buff[i].key, h->buff[i].value);
		i++;
	}
	free(h->buff);
	*h = new_h;
	return (true);
}

/* Insert or update key-value pair. */
static inline bool	hash_set(t_hash *h, const char *key, void *value)
{
	size_t	idx;
	size_t	i;

	if (h->cap == 0 || key == NULL)
		return (false);
	if (h->len >= h->cap / 2)
		if (!hash_resize(h))
			return (false);
	idx = hash_func(key, h->cap);
	i = 0;
	while (i < h->cap)
	{
		if (h->buff[idx].key == NULL)
		{
			h->buff[idx].key = (char *)key;
			h->buff[idx].value = value;
			h->len++;
			return (true);
		}
		if (ft_strcmp(h->buff[idx].key, key) == 0)
		{
			h->buff[idx].value = value;
			return (true);
		}
		idx = (idx + 1) % h->cap;
		i++;
	}
	return (false);
}

/* Get value by key. Returns NULL if not found. */
static inline void	*hash_get(t_hash *h, const char *key)
{
	int	idx;

	idx = hash_find_idx(h, key);
	if (idx >= 0)
		return (h->buff[idx].value);
	return (NULL);
}

/* Free hash table and reset struct. */
static inline void	hash_destroy(t_hash *h, void (*free_val)(void *))
{
	size_t	i;

	if (h == NULL)
		return ;
	if (free_val != NULL)
	{
		i = 0;
		while (i < h->cap)
		{
			if (h->buff[i].key != NULL)
				free_val(h->buff[i].value);
			i++;
		}
	}
	free(h->buff);
	*h = (t_hash){0};
}

# endif // HASH

#endif