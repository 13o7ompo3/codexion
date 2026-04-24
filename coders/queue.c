/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 18:43:36 by obahya            #+#    #+#             */
/*   Updated: 2026/04/24 18:43:38 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

t_node	*create_node(t_coder *coder)
{
	t_node	*node;

	node = malloc(sizeof(t_node));
	if (!node)
		return (NULL);
	node->coder = coder;
	node->next = node;
	node->prev = node;
	return (node);
}

t_node	*append_node(t_node *head, t_node *new_node, int (*compare)(t_coder *, t_coder *))
{
	t_node	*current;

	if (!head)
	{
		new_node->next = new_node;
		new_node->prev = new_node;
		return (new_node);
	}
	current = head;
	do {
		if (compare(new_node->coder, current->coder))
			break;
		current = current->next;
	} while (current != head);

	new_node->prev = current->prev;
	new_node->next = current;
	current->prev->next = new_node;
	current->prev = new_node;

	if (current == head && compare(new_node->coder, head->coder))
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
