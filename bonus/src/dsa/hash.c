/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   hash.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 00:48:19 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 00:48:19 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../shell.h"
# include "hash.h"


/* Initialize hash table with given capacity. */
bool	hash_init(t_hash *h, size_t capacity)
{
	*h = (t_hash){0};
	if (capacity == 0)
		capacity = 64;
	h->elem_size = sizeof(t_hash_entry);
	h->ctx = calloc(capacity, sizeof(t_hash_entry));
	if (h->ctx == NULL)
		return (false);
	h->cap = capacity;
	h->len = 0;
	return (true);
}

/* Simple hash function for strings. */
size_t	hash_func(const char *key, size_t cap)
{
	size_t	hash = 5381;

	while (*key)
		hash = ((hash << 5) + hash) + (unsigned char)*key++;
	return (hash % cap);
}

/* Find entry by key. Returns index or -1 if not found. */
int	hash_find_idx(t_hash *h, const char *key)
{
	size_t	idx;
	size_t	i;
	t_hash_entry *buff = (t_hash_entry *)h->ctx;

	if (h->cap == 0 || key == NULL)
		return (-1);
	idx = hash_func(key, h->cap);
	i = 0;
	while (i < h->cap)
	{
		if (buff[idx].key != NULL && strcmp(buff[idx].key, key) == 0)
			return ((int)idx);
		idx = (idx + 1) % h->cap;
		i++;
	}
	return (-1);
}


/* Resize hash table and rehash entries. */
bool	hash_resize(t_hash *h)
{
	t_hash		new_h;
	size_t		i;
	t_hash_entry *buff = (t_hash_entry *)h->ctx;

	if (!hash_init(&new_h, h->cap * 2))
		return (false);
	for (i = 0; i < h->cap; ++i)
	{
		if (buff[i].key != NULL)
			hash_set(&new_h, buff[i].key, buff[i].value);
	}
	free(h->ctx);
	*h = new_h;
	return (true);
}

/* Insert or update key-value pair. */
bool	hash_set(t_hash *h, const char *key, void *value)
{
	size_t	idx;
	size_t	i;
	t_hash_entry *buff;

	if (h->cap == 0 || key == NULL)
		return (false);
	if (h->len >= h->cap / 2)
		if (!hash_resize(h))
			return (false);
	buff = (t_hash_entry *)h->ctx;
	idx = hash_func(key, h->cap);
	i = 0;
	while (i < h->cap)
	{
		if (buff[idx].key == NULL)
		{
			buff[idx].key = (char *)key;
			buff[idx].value = value;
			h->len++;
			return (true);
		}
		if (strcmp(buff[idx].key, key) == 0)
		{
			buff[idx].value = value;
			return (true);
		}
		idx = (idx + 1) % h->cap;
		i++;
	}
	return (false);
}

/* Get value by key. Returns NULL if not found. */
void	*hash_get(t_hash *h, const char *key)
{
	int	idx = hash_find_idx(h, key);
	t_hash_entry *buff = (t_hash_entry *)h->ctx;

	if (idx >= 0)
		return (buff[idx].value);
	return (NULL);
}

/* Free hash table and reset struct. */
void	hash_destroy(t_hash *h, void (*free_val)(void *))
{
	size_t	i;
	t_hash_entry *buff;

	if (h == NULL)
		return ;
	buff = (t_hash_entry *)h->ctx;
	if (free_val != NULL && buff != NULL)
	{
		for (i = 0; i < h->cap; ++i)
		{
			if (buff[i].key != NULL)
				free_val(buff[i].value);
		}
	}
	free(h->ctx);
	*h = (t_hash){0};
}
