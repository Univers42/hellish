/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   export.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:29:51 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:29:51 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

void parse_export_arg(char *str, char **ident, char **val)
{
	char *eq;

	eq = ft_strchr(str, '=');
	if (eq)
	{
		*ident = ft_strndup(str, eq - str);
		*val = ft_strdup(eq + 1);
	}
	else
	{
		*ident = ft_strdup(str);
		*val = NULL;
	}
}

static bool is_valid_ident(char *id)
{
	int i;

	i = 0;
	if (!is_var_name_p1(id[0]))
		return (false);
	while (id[i] && is_var_name_p2(id[i]))
		i++;
	return (!id[i]);
}

static char	*expand_export_value(t_shell *st, char *val, bool allow_expand)
{
	t_string	out;
	size_t		i;
	size_t		start;

	if (!val || !allow_expand)
		return (val);
	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	while (val[i])
	{
		if (val[i] == '$' && is_var_name_p1(val[i + 1]))
		{
			start = ++i;
			while (is_var_name_p2(val[i]))
				i++;
			{
				char	*rep = env_expand_n(st, val + start, i - start);
				if (rep)
					vec_push_str(&out, rep);
			}
			continue ;
		}
		vec_push(&out, &val[i]);
		i++;
	}
	free(val);
	if (!vec_ensure_space_n(&out, 1))
		return (NULL);
	((char *)out.ctx)[out.len] = '\0';
	return ((char *)out.ctx);
}

char	strip_surrounding_quotes(char **val)
{
	size_t	vlen;
	char	*clean;
	char	f;

	if (!val || !*val)
		return (0);
	vlen = ft_strlen(*val);
	if (vlen >= 2)
	{
		f = (*val)[0];
		if ((f == '"' || f == '\'') && (*val)[vlen - 1] == f)
		{
			clean = ft_strndup(*val + 1, vlen - 2);
			free(*val);
			*val = clean;
			return (f);
		}
	}
	return (0);
}

void consume_following_value(t_vec av, int *i, char **val)
{
	int idx;
	char *next;

	idx = *i;
	if ((!*val || (*val)[0] == '\0') && idx + 1 < (int)av.len)
	{
		next = ((char **)av.ctx)[idx + 1];
		if (next && !ft_strchr(next, '='))
		{
			*val = ft_strdup(next);
			*i = idx + 1;
		}
	}
}

/* handle identifier: set env or mark exported or print error */
int handle_identifier(t_shell *st, char *id, char *val, const char *argv0, const char *orig_arg)
{
	t_env *e;

	if (is_valid_ident(id))
	{
		if (!val)
		{
			e = env_get(&st->env, id);
			if (e)
				e->exported = true;
			free(id);
		}
		else
			env_set(&st->env, (t_env){true, id, val});
		return (0);
	}
	else
		return (ft_eprintf("%s: %s: `%s' not valid identifier\n", st->context,
						   argv0, orig_arg),
				free(id), free(val), 1);
}

/* reduced process_arg using helpers */
int process_arg(t_shell *st, t_vec av, int *ip)
{
	char *id;
	char *val;
	int i;
	char *arg0;
	char *cur;
	char quote;

	i = *ip;
	arg0 = ((char **)av.ctx)[0];
	cur = ((char **)av.ctx)[i];
	if (!av.ctx || !cur)
		return (ft_eprintf("[DEBUG export] missing argv element at index %d\n", i), 1);
	parse_export_arg(cur, &id, &val);
#ifdef DEBUG_EXPORT
	ft_eprintf("[DEBUG export] arg='%s' -> id='%s' val='%s'\n", cur, id ? id : "(null)", val ? val : "(null)");
#endif
	quote = strip_surrounding_quotes(&val);
	consume_following_value(av, &i, &val);
	*ip = i;
	if (val)
		val = expand_export_value(st, val, quote != '\'');
	return (handle_identifier(st, id, val, arg0, cur));
}

/* collect, sort and print exported variables (extracted from builtin_export) */
void collect_and_print_exported(t_shell *st)
{
	t_vec list;
	size_t j;
	t_env *e;
	char *s;

	vec_init(&list);
	list.elem_size = sizeof(char *);
	j = 0;
	while (j < st->env.len)
	{
		e = &((t_env *)st->env.ctx)[j];
		if (e->exported)
		{
			s = ft_asprintf("export %s=\"%s\"", e->key, e->value ? e->value : "");
			vec_push(&list, &s);
		}
		j++;
	}
	if (list.len > 1)
		ft_quicksort(&list);
	j = 0;
	while (j < list.len)
	{
		s = ((char **)list.ctx)[j];
		ft_printf("%s\n", s);
		free(s);
		j++;
	}
	free(list.ctx);
}

/* replace builtin_export loop with calls to helpers */
int builtin_export(t_shell *st, t_vec av)
{
	size_t i;
	int status;
	int idx;

#ifdef DEBUG_EXPORT
	size_t di;
	char *s;
	ft_eprintf("[DEBUG builtin_export] argv.len=%d\n", (int)av.len);
	if (!av.ctx)
		ft_eprintf("  argv.ctx = NULL\n");
	else
	{
		di = -1;
		while (++di < av.len)
		{
			s = ((char **)av.ctx)[di];
			ft_eprintf("  argv[%d] = '%s'\n", (int)di, s ? s : "(null)");
		}
	}
#endif

	i = 1;
	status = 0;
	if (av.len == 1)
		return (collect_and_print_exported(st), 0);
	while (i < av.len)
	{
		idx = (int)i;
		status = process_arg(st, av, &idx) || status;
		i = (size_t)idx + 1;
	}
	if (status)
		return (1);
	return (0);
}
