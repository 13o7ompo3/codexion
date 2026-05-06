/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 18:43:36 by obahya            #+#    #+#             */
/*   Updated: 2026/04/26 14:39:27 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

t_node	*append_node(t_node *head, t_node *new_node,
	int (*compare)(t_coder *, t_coder *))
{
	t_node	*current;

	if (!head)
	{
		new_node->next = new_node;
		new_node->prev = new_node;
		return (new_node);
	}
	current = head;
	while (1)
	{
		if (compare(new_node->coder, current->coder))
			break ;
		current = current->next;
		if (current == head)
			break ;
	}
	new_node->prev = current->prev;
	new_node->next = current;
	current->prev->next = new_node;
	current->prev = new_node;
	if (current == head && compare(new_node->coder, current->coder))
		return (new_node);
	return (head);
}

t_node	*remove_node(t_node **head, t_node *node)
{
	if (!node)
		return (*head);
	if (*head == node)
	{
		if (node->next == node)
			*head = NULL;
		else
			*head = node->next;
	}
	node->prev->next = node->next;
	node->next->prev = node->prev;
	return (*head);
}
