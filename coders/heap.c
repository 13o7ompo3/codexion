/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 18:42:56 by obahya            #+#    #+#             */
/*   Updated: 2026/05/02 11:21:48 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	heap_insert(t_heap *heap, t_coder *coder)
{
	if (heap->size >= heap->capacity)
		return ;
	heap->array[heap->size] = coder;
	heapify_up(heap, heap->size);
	heap->size++;
}

void	heap_remove_at(t_heap *heap, int idx)
{
	int	parent;

	if (idx < 0 || idx >= heap->size)
		return ;
	heap->size--;
	if (idx == heap->size)
		return ;
	heap->array[idx] = heap->array[heap->size];
	parent = (idx - 1) / 2;
	if (idx > 0 && heap->compare(heap->array[idx], heap->array[parent]))
		heapify_up(heap, idx);
	else
		heapify_down(heap, idx);
}

t_heap	*init_heap(int capacity, int scheduler_type)
{
	t_heap	*heap;

	heap = malloc(sizeof(t_heap));
	if (!heap)
		return (NULL);
	heap->array = ft_calloc(capacity, sizeof(t_coder *));
	if (!heap->array)
	{
		free(heap);
		return (NULL);
	}
	heap->size = 0;
	heap->capacity = capacity;
	if (scheduler_type == 0)
		heap->compare = compare_edf;
	else if (scheduler_type == 1)
		heap->compare = compare_fifo;
	else
		heap->compare = compare_sleep;
	return (heap);
}

void	free_heap(t_heap *heap)
{
	free(heap->array);
	heap->array = NULL;
	free(heap);
	heap = NULL;
}
