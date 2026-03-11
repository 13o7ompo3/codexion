#include "codexion.h"

long long get_deadline(t_coder *coder)
{
	return (coder->last_compile_start + coder->sim->time_to_burnout);
}

void enqueue(t_sim *sim, t_coder *new_coder, int scheduler_type)
{
	t_coder		*curr;
	long long	d_new;
	long long	d_curr;

	if (!sim->queue)
	{
		sim->queue = new_coder;
		new_coder->next = new_coder;
		new_coder->prev = new_coder;
		return ;
	}
	curr = sim->queue;
	if (scheduler_type == 1)
	{
			d_new = get_deadline(new_coder);
			do {
				d_curr = get_deadline(curr);

				if (d_new < d_curr)
					break ;

				if (d_new == d_curr && new_coder->compiles_done < curr->compiles_done)
					break ;
					
				curr = curr->next;
			} while (curr != sim->queue);
		}
		new_coder->next = curr;
		new_coder->prev = curr->prev;
		curr->prev->next = new_coder;
		curr->prev = new_coder;
		if (scheduler_type == 1 && curr == sim->queue)
		{
			d_new = get_deadline(new_coder);
			d_curr = get_deadline(curr);
			if (d_new < d_curr || (d_new == d_curr && new_coder->compiles_done < curr->compiles_done))
				sim->queue = new_coder;
		}
	}

void	remove_coder(t_sim *sim, t_coder *coder)
{
	if (!coder->next || !coder->prev)
		return;
	if (coder->next == coder)
		sim->queue = NULL;
	else
	{
		coder->prev->next = coder->next;
		coder->next->prev = coder->prev;
		if (sim->queue == coder)
			sim->queue = coder->next;
	}
	coder->next = NULL;
	coder->prev = NULL;
}

