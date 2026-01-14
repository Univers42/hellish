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


#include "builtins.h"
# include "env.h"

void	parse_export_arg(char *str, char **ident, char **val)
{
	char	*eq;

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

static bool	is_valid_ident(char *id)
{
	int	i;

	i = 0;
	if (!is_var_name_p1(id[0]))
		return (false);
	while (id[i] && is_var_name_p2(id[i]))
		i++;
	return (!id[i]);
}

static int	process_arg(t_shell *st, t_vec av, int *ip)
{
	char	*id;
	t_env	*e;
	char	*val;
	int     i = *ip;

	if (!av.ctx || !((char **)av.ctx)[i])
	{
		ft_eprintf("[DEBUG export] missing argv element at index %d\n", i);
		return (1);
	}
	parse_export_arg(((char **)av.ctx)[i], &id, &val);
#	ifdef DEBUG_EXPORT
	ft_eprintf("[DEBUG export] arg='%s' -> id='%s' val='%s'\n", ((char **)av.ctx)[i], id ? id : "(null)", val ? val : "(null)");
#	endif
	/* If the parsed value is quoted ("..." or '...'), remove surrounding quotes */
	if (val && val[0] && val[1])
	{
		size_t vlen = ft_strlen(val);
		if ((val[0] == '"' && val[vlen - 1] == '"') || (val[0] == '\'' && val[vlen - 1] == '\''))
		{
			char *clean = ft_strndup(val + 1, vlen - 2);
			free(val);
			val = clean;
		}
	}
	/* If value missing (argument ended with '='), and next argv exists, consume it as value */
	if ((!val || val[0] == '\0') && i + 1 < (int)av.len)
	{
		char *next = ((char **)av.ctx)[i + 1];
		if (next && !ft_strchr(next, '='))
		{
			val = ft_strdup(next);
			/* advance caller index to skip consumed value */
			*ip = i + 1;
		}
	}
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
	}
	else
	{
		ft_eprintf("%s: %s: `%s' not valid identifier\n", st->context,
			((char **)av.ctx)[0], ((char **)av.ctx)[i]);
		free(id);
		return (free(val), 1);
	}
	return (0);
}

int	builtin_export(t_shell *st, t_vec av)
{
    size_t	i;
    int		status;

    /* Debug: print argv elements (use %d and avoid NULL %s) */
#	ifdef DEBUG_EXPORT
    size_t	di;
    ft_eprintf("[DEBUG builtin_export] argv.len=%d\n", (int)av.len);
    if (!av.ctx)
    {
        ft_eprintf("  argv.ctx = NULL\n");
    }
    else
    {
        for (di = 0; di < av.len; ++di)
        {
            char *s = ((char **)av.ctx)[di];
            ft_eprintf("  argv[%d] = '%s'\n", (int)di, s ? s : "(null)");
        }
    }
#	endif

    i = 1;
    status = 0;
    if (av.len == 1)
    {
        /* Collect exported entries into a vector of strings, sort and print */
        t_vec list;
        size_t j;

        vec_init(&list);
        list.elem_size = sizeof(char *);
        j = 0;
        while (j < st->env.len)
        {
            t_env *e = &((t_env *)st->env.ctx)[j];
            if (e->exported)
            {
                char *s = ft_asprintf("declare -x %s=\"%s\"", e->key, e->value ? e->value : "");
                vec_push(&list, &s);
            }
            j++;
        }
        if (list.len > 1)
            ft_quicksort(&list);
        j = 0;
        while (j < list.len)
        {
            char *s = ((char **)list.ctx)[j];
            ft_printf("%s\n", s);
            free(s);
            j++;
        }
        free(list.ctx);
        return (0);
    }
    while (i < av.len)
    {
        int idx = (int)i;
        status = process_arg(st, av, &idx) || status;
        i = (size_t)idx + 1;
    }
    return (status ? 1 : 0);
}
