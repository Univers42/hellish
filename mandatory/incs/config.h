/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/31 16:23:28 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 15:56:15 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* config.h -- Configuration file for bash. */

#ifndef CONFIG_H
#define CONFIG_H

/* Basic features - always available */
#define FEATURE_CORE 1

/* Exclusive mode flags take precedence */
#ifdef FEATURE_HELP_ONLY
/* Only help enabled */
#define FEATURE_HELP 1
#elif defined(FEATURE_DEBUG_ONLY)
/* Only debug + help enabled */
#define FEATURE_DEBUG 1
#define FEATURE_HELP 1
#else
/* DEFAULT: ALL features enabled (no exclusive flags set) */
#define FEATURE_HELP 1
#define FEATURE_DEBUG 1
#define FEATURE_LOGIN 1
#define FEATURE_POSIX 1
#define FEATURE_PRETTY_PRINT 1
#endif

#endif /* CONFIG_H */
