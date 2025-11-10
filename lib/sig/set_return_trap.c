/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_return_trap.c                                  :+:      :+:    :+:   */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void set_return_trap(const char *command)
{
	set_signal(RETURN_TRAP, command);
}
