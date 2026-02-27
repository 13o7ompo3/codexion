#include "codexion.h"

long long get_deadline(t_coder *coder)
{
	return (coder->last_compile_start + coder->sim->time_to_burnout);
}

void enqueue(t_sim *dongle, t_node *new_node, int scheduler_type)
{
	t_node *curr;
	long long	d_new;
	long long	d_curr;

	if (!dongle->queue)
	{
		dongle->queue = new_node;
		new_node->next = new_node;
		new_node->prev = new_node;
		return ;
	}
	curr = (t_node *)dongle->queue;
	if (scheduler_type == 1)
	{
			d_new = get_deadline(new_node->coder);
			do {
				d_curr = get_deadline(curr->coder);

				if (d_new < d_curr)
					break ;

				if (d_new == d_curr && new_node->coder->compiles_done < curr->coder->compiles_done)
					break ;
					
				curr = curr->next;
			} while (curr != (t_node *)dongle->queue);
		}
		new_node->next = curr;
		new_node->prev = curr->prev;
		curr->prev->next = new_node;
		curr->prev = new_node;
		if (scheduler_type == 1 && curr == (t_node *)dongle->queue)
		{
			d_curr = get_deadline(curr->coder);
			if (d_new < d_curr || (d_new == d_curr && new_node->coder->compiles_done < curr->coder->compiles_done))
				dongle->queue = new_node;
		}
	}

void	remove_node(t_sim *sim, t_node *node)
{
	if (node->next == node)
	{
		sim->queue = NULL;
	}
	else
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
		if (sim->queue == node)
			sim->queue = node->next;
	}
	node->next = NULL;
	node->prev = NULL;
}

