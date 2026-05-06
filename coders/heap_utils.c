/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 21:31:27 by obahya            #+#    #+#             */
/*   Updated: 2026/04/24 21:40:07 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	swap_coders(t_coder **a, t_coder **b)
{
	t_coder	*tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

void	heapify_up(t_heap *heap, int idx)
{
	int	parent;

	while (idx > 0)
	{
		parent = (idx - 1) / 2;
		if (heap->compare(heap->array[idx], heap->array[parent]))
		{
			swap_coders(&heap->array[idx], &heap->array[parent]);
			idx = parent;
		}
		else
			break ;
	}
}

void	heapify_down(t_heap *heap, int idx)
{
	int	left;
	int	right;
	int	best;

	while (1)
	{
		left = 2 * idx + 1;
		right = 2 * idx + 2;
		best = idx;
		if (left < heap->size
			&& heap->compare(heap->array[left], heap->array[best]))
			best = left;
		if (right < heap->size
			&& heap->compare(heap->array[right], heap->array[best]))
			best = right;
		if (best != idx)
		{
			swap_coders(&heap->array[idx], &heap->array[best]);
			idx = best;
		}
		else
			break ;
	}
}
