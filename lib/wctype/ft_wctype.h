/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_wctype.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 16:24:11 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 16:52:26 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_WCTYPE_H
# define FT_WCTYPE_H

# include <wchar.h>

typedef enum e_wctype
{
	FT_WCTYPE_ALNUM,
	FT_WCTYPE_ALPHA,
	FT_WCTYPE_BLANK,
	FT_WCTYPE_CNTRL,
	FT_WCTYPE_DIGIT,
	FT_WCTYPE_GRAPH,
	FT_WCTYPE_LOWER,
	FT_WCTYPE_PRINT,
	FT_WCTYPE_PUNCT,
	FT_WCTYPE_SPACE,
	FT_WCTYPE_UPPER,
	FT_WCTYPE_XDIGIT,
	FT_WCTYPE_NONE
}	t_wctype;

/* classification prototypes */
int			ft_iswalnum(wint_t wc);
int			ft_iswalpha(wint_t wc);
int			ft_iswblank(wint_t wc);
int			ft_iswcntrl(wint_t wc);
int			ft_iswdigit(wint_t wc);
int			ft_iswgraph(wint_t wc);
int			ft_iswlower(wint_t wc);
int			ft_iswprint(wint_t wc);
int			ft_iswpunct(wint_t wc);
int			ft_iswspace(wint_t wc);
int			ft_iswupper(wint_t wc);
int			ft_iswxdigit(wint_t wc);

/* generic helpers */
int			ft_towlower(wint_t wc);
int			ft_towupper(wint_t wc);

/* meta helpers */
int			ft_iswctype(wint_t wc, t_wctype desc);
t_wctype	ft_wctype(const wchar_t *name);

#endif
