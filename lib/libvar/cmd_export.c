/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cmd_export.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 19:35:32 by alcacere          #+#    #+#             */
/*   Updated: 2025/11/04 21:09:22 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "var.h"

void	export_names(char **aptr, int flag)
{
    char		*name;
    const char	*p;
    t_var	*vp;

    while ((name = *aptr++) != NULL)
    {
        p = ft_strchr(name, '=');
        if (p)
            var_set(name, p + 1, flag);
        else {
            vp = *var_fin(name);
            if (vp)
                vp->flags |= flag;
            else
                var_set(name, NULL, flag);
        }
    }
}

int	exportcmd(int argc, char **argv)
{
    int		flag;
    int		notp;
    char	**aptr;

    if (argv[0][0] == 'r')
        flag = VREADONLY;
    else
        flag = VEXPORT;
    notp = nextopt("p") - 'p';
    if (notp && (aptr = argptr) && *aptr)
        export_names(aptr, flag);
    else
        var_show(argv[0], flag, 0);
    return (0);
}
