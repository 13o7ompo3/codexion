#include "codexion.h"

t_node *dequeue(t_dongle *dongle)
{
	t_node *head;
	t_node *tail;

	if (!dongle->queue)
		return (NULL);
	head = (t_node *)dongle->queue;
	if (head->next == head)
		dongle->queue = NULL;
	else
	{
		tail = head->prev;
		dongle->queue = head->next;
		tail->next = dongle->queue;
		((t_node *)dongle->queue)->prev = tail;
	}
	head->next = NULL;
	head->prev = NULL;
	return (head);
}

long long get_deadline(t_coder *coder)
{
	return (coder->last_compile_start + coder->sim->time_to_burnout);
}

void enqueue(t_dongle *dongle, t_node *new_node, int scheduler_type)
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

            // Standard EDF condition
            if (d_new < d_curr)
                break ;
            
            // THE TIE-BREAKER: If deadlines are exactly equal, prioritize the coder with fewer compiles
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
        {
            dongle->queue = new_node; // We are the new head!
        }
    }
}
