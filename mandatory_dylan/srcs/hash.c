/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   hash.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 16:01:54 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 16:01:54 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../incs/dsa.h"


/* Initialize hash table with given capacity. */
bool	hash_init(t_hash *h, size_t capacity)
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
size_t	hash_func(const char *key, size_t cap)
{
	size_t	hash;

	hash = 5381;
	while (*key)
		hash = ((hash << 5) + hash) + (unsigned char)*key++;
	return (hash % cap);
}

/* Find entry by key. Returns index or -1 if not found. */
int	hash_find_idx(t_hash *h, const char *key)
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


/* Resize hash table and rehash entries. */
bool	hash_resize(t_hash *h)
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
bool	hash_set(t_hash *h, const char *key, void *value)
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
void	*hash_get(t_hash *h, const char *key)
{
	int	idx;

	idx = hash_find_idx(h, key);
	if (idx >= 0)
		return (h->buff[idx].value);
	return (NULL);
}

/* Free hash table and reset struct. */
void	hash_destroy(t_hash *h, void (*free_val)(void *))
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
