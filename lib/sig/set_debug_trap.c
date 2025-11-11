/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_debug_trap.c                                   :+:      :+:    :+:   */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void set_debug_trap(const char *command)
{
	set_signal(DEBUG_TRAP, command);
}
