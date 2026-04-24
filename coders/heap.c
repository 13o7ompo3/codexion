/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 18:42:56 by obahya            #+#    #+#             */
/*   Updated: 2026/04/24 18:43:04 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"


int compare_edf(t_coder *a, t_coder *b)
{
    if (a->deadline < b->deadline)
        return (1);
    if (a->deadline > b->deadline)
        return (0);
    // Tie breaker
    if (a->compiles_done < b->compiles_done)
        return (1);
    if (a->compiles_done > b->compiles_done)
        return (0);
    if (a->id % 2 != 0 && b->id % 2 == 0)
        return (1);
    return (0);
}

int compare_fifo(t_coder *a, t_coder *b)
{
    if (a->deadline < b->deadline)
        return (1);
    if (a->deadline > b->deadline)
        return (0);
    if (a->compiles_done < b->compiles_done)
        return (1);
    if (a->compiles_done > b->compiles_done)
        return (0);
    // Tie breaker
    if (a->id % 2 != 0 && b->id % 2 == 0)
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

static void	swap_coders(t_coder **a, t_coder **b)
{
	t_coder	*tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

static void	heapify_up(t_heap *heap, int idx)
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

void	heap_insert(t_heap *heap, t_coder *coder)
{
	if (heap->size >= heap->capacity)
		return ;
	heap->array[heap->size] = coder;
	heapify_up(heap, heap->size);
	heap->size++;
}

static void	heapify_down(t_heap *heap, int idx)
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

void	heap_remove_at(t_heap *heap, int idx)
{
	int	parent;

	if (idx < 0 || idx >= heap->size)
		return ;
	heap->size--;
	if (idx == heap->size)
		return ; // We just removed the last element, no sorting needed

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
	heap->array = calloc(capacity, sizeof(t_coder *));
	if (!heap->array)
	{
		free(heap);
		return (NULL);
	}
	heap->size = 0;
	heap->capacity = capacity;

	if (scheduler_type == 0) // EDF
		heap->compare = compare_edf;
	else if (scheduler_type == 1) // FIFO
		heap->compare = compare_fifo;
	else				// Sleep Room
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
