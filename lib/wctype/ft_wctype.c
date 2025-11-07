/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_wctype.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 16:35:00 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 16:54:17 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_wctype.h"

t_wctype	ft_wctype(const wchar_t *name)
{
	if (wcscmp(name, L"alnum") == 0)
		return (FT_WCTYPE_ALNUM);
	if (wcscmp(name, L"alpha") == 0)
		return (FT_WCTYPE_ALPHA);
	if (wcscmp(name, L"blank") == 0)
		return (FT_WCTYPE_BLANK);
	if (wcscmp(name, L"cntrl") == 0)
		return (FT_WCTYPE_CNTRL);
	if (wcscmp(name, L"digit") == 0)
		return (FT_WCTYPE_DIGIT);
	if (wcscmp(name, L"graph") == 0)
		return (FT_WCTYPE_GRAPH);
	if (wcscmp(name, L"lower") == 0)
		return (FT_WCTYPE_LOWER);
	if (wcscmp(name, L"print") == 0)
		return (FT_WCTYPE_PRINT);
	if (wcscmp(name, L"punct") == 0)
		return (FT_WCTYPE_PUNCT);
	if (wcscmp(name, L"space") == 0)
		return (FT_WCTYPE_SPACE);
	if (wcscmp(name, L"upper") == 0)
		return (FT_WCTYPE_UPPER);
	if (wcscmp(name, L"xdigit") == 0)
		return (FT_WCTYPE_XDIGIT);
	return (FT_WCTYPE_NONE);
}

int	ft_iswctype(wint_t wc, t_wctype desc)
{
	if (desc == FT_WCTYPE_ALNUM)
		return (ft_iswalnum(wc));
	if (desc == FT_WCTYPE_ALPHA)
		return (ft_iswalpha(wc));
	if (desc == FT_WCTYPE_BLANK)
		return (ft_iswblank(wc));
	if (desc == FT_WCTYPE_CNTRL)
		return (ft_iswcntrl(wc));
	if (desc == FT_WCTYPE_DIGIT)
		return (ft_iswdigit(wc));
	if (desc == FT_WCTYPE_GRAPH)
		return (ft_iswgraph(wc));
	if (desc == FT_WCTYPE_LOWER)
		return (ft_iswlower(wc));
	if (desc == FT_WCTYPE_PRINT)
		return (ft_iswprint(wc));
	if (desc == FT_WCTYPE_PUNCT)
		return (ft_iswpunct(wc));
	if (desc == FT_WCTYPE_SPACE)
		return (ft_iswspace(wc));
	if (desc == FT_WCTYPE_UPPER)
		return (ft_iswupper(wc));
	if (desc == FT_WCTYPE_XDIGIT)
		return (ft_iswxdigit(wc));
	return (0);
}
