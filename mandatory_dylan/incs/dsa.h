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

bool	deque_init(t_deque *ret, int size, size_t elem_size);
void	deque_ensure_space(t_deque *ret);
bool	deque_push_end(t_deque *ret, const void *item);
bool	deque_push_start(t_deque *ret, const void *item);
void	*deque_pop_end(t_deque *ret);
void	*deque_pop_start(t_deque *ret);
void	*deque_idx(t_deque *ret, size_t idx);
void	*deque_peek(t_deque *ret);
void	deque_peek_into(t_deque *ret, void *out);
void	deque_reset(t_deque *ret);
bool	deque_clone(t_deque *ret, const t_deque proto);
void	deque_destroy(t_deque *ret, void (*free_elem)(void *));
void	deque_clear(t_deque *ret, void (*free_elem)(void *));

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

bool	vec_init(t_vec *ret);
bool	vec_ensure_space(t_vec *v);
bool	vec_ensure_space_n(t_vec *v, size_t n);
bool	vec_push(t_vec *v, const void *el);
bool	vec_npush(t_vec *v, const void *el, size_t n);
bool	vec_push_nstr(t_vec *v, const char *str, size_t n);
bool	vec_push_str(t_vec *v, const char *str);
bool	vec_push_byte(t_vec *v, unsigned char byte);
void	*vec_pop(t_vec *v);
void	*vec_idx(t_vec *v, size_t idx);
void	vec_append_fd(int fd, t_vec *ret);
void	vec_destroy(t_vec *v, void (*free_elem)(void *));

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

void		ast_postorder_traversal(t_ast_node *node, void (*fn)(t_ast_node *node));
void		free_node(t_ast_node *node);
void		free_ast(t_ast_node *node);
t_ast_node	make_ast_token(t_token t, int offset, int end_offset, t_tt tt);

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

bool	hash_init(t_hash *h, size_t capacity);
size_t	hash_func(const char *key, size_t cap);
int		hash_find_idx(t_hash *h, const char *key);
bool	hash_resize(t_hash *h);
bool	hash_set(t_hash *h, const char *key, void *value);
void	*hash_get(t_hash *h, const char *key);
void	hash_destroy(t_hash *h, void (*free_val)(void *));

# endif // HASH

#endif