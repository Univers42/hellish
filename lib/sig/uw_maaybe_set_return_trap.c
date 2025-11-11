/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   uw_maaybe_set_return_trap.c                        :+:      :+:    :+:   */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void uw_maybe_set_return_trap(void *command)
{
	trap_if_untrapped(RETURN_TRAP, command);
}
