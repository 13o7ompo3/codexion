/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_compare.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 21:22:37 by obahya            #+#    #+#             */
/*   Updated: 2026/04/24 23:06:31 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	compare_edf(t_coder *a, t_coder *b)
{
	if (a->deadline < b->deadline)
		return (1);
	if (a->deadline > b->deadline)
		return (0);
	if (a->compiles_done < b->compiles_done)
		return (1);
	if (a->compiles_done > b->compiles_done)
		return (0);
	if (a->id % 2 != 0 && b->id % 2 == 0)
		return (1);
	if (a->id % 2 == 0 && b->id % 2 != 0)
		return (0);
	if (a->id < b->id)
		return (1);
	return (0);
}

int	compare_fifo(t_coder *a, t_coder *b)
{
	if (a->deadline < b->deadline)
		return (1);
	if (a->deadline > b->deadline)
		return (0);
	if (a->compiles_done < b->compiles_done)
		return (1);
	if (a->compiles_done > b->compiles_done)
		return (0);
	if (a->id % 2 != 0 && b->id % 2 == 0)
		return (1);
	if (a->id % 2 == 0 && b->id % 2 != 0)
		return (0);
	if (a->id < b->id)
		return (1);
	return (0);
}

int	compare_sleep(t_coder *a, t_coder *b)
{
	if (a->wake_up_time < b->wake_up_time)
		return (1);
	if (a->wake_up_time > b->wake_up_time)
		return (0);
	return (0);
}
