/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exe_path.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:21 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:21 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

static void	reset_string(t_string *temp, const char *dir, const char *exe_name)
{
	char	*joined;

	joined = ft_strnjoin(dir, "/", exe_name, NULL);
	vec_clear(temp);
	vec_push_str(temp, joined);
	free(joined);
}

char	*exe_path(char **path_dirs, char *exe_name, int *perm_denied)
{
	t_string	temp;
	int			i;
	struct stat	st;

	*perm_denied = 0;
	if (ft_strchr(exe_name, '/'))
		return (ft_strdup(exe_name));
	if (ft_strlen(exe_name) == 0)
		return (0);
	vec_init(&temp);
	i = -1;
	while (path_dirs && path_dirs[++i])
	{
		reset_string(&temp, path_dirs[i], exe_name);
		if (access((char *)temp.ctx, F_OK) != 0)
			continue ;
		if (stat((char *)temp.ctx, &st) == -1)
			continue ;
		if (S_ISDIR(st.st_mode))
			continue ;
		if (access((char *)temp.ctx, X_OK) == 0)
			return ((char *)temp.ctx);
		*perm_denied = 1;
	}
	return (free(temp.ctx), NULL);
}
