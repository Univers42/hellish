/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tgoto.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/11 00:50:35 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/11 00:50:38 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"

/*
 * termcap convention: parameter string usually wants row (y) first then column (x).
 * Public API: tgoto(cm, hpos, vpos) like classic termcap.
 * We pass (row=vpos, col=hpos) to tparam.
 */
char *tgoto(const char *cm, int hpos, int vpos)
{
    static char buf[128];

    if (!cm)
        return (NULL);
    return (tparam(cm, buf, (int)sizeof(buf), vpos, hpos, 0, 0));
}
