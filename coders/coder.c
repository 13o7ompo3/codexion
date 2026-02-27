#include "codexion.h"
#include <unistd.h>

void	precise_sleep(long long time_in_ms, t_sim *sim)
{
	long long start_time;
	long long current_time;

	start_time = get_current_time_ms();
	while (1)
	{
		pthread_mutex_lock(&sim->state_mutex);
		if (!sim->is_active)
		{
			pthread_mutex_unlock(&sim->state_mutex);
			break ;
		}
		pthread_mutex_unlock(&sim->state_mutex);

		current_time = get_current_time_ms();
		if ((current_time - start_time) >= time_in_ms)
			break ;
	}
}

int	take_both_dongles(t_coder *coder)
{
	t_node *my_node = malloc(sizeof(t_node));
	if (!my_node) return (1);
	my_node->coder = coder;
	pthread_mutex_lock(&coder->sim->state_mutex);
	enqueue(coder->sim, my_node, coder->sim->scheduler_type);
	while (1)
	{
		if (!coder->sim->is_active)
		{
			remove_node(coder->sim, my_node);
			free(my_node);
			pthread_mutex_unlock(&coder->sim->state_mutex);
			return (1);
		}
		int conflict = 0;
		t_node *curr = (t_node *)coder->sim->queue;
		if (curr)
		{
			while (curr != my_node)
			{
				if (curr->coder->left_dongle == coder->left_dongle || 
					curr->coder->left_dongle == coder->right_dongle ||
					curr->coder->right_dongle == coder->left_dongle ||
					curr->coder->right_dongle == coder->right_dongle)
				{
					conflict = 1;
					break;
				}
				curr = curr->next;
			}
		}
		int on_cooldown = 0;
		if (!conflict)
		{
			t_dongle *first = coder->left_dongle < coder->right_dongle ? coder->left_dongle : coder->right_dongle;
			t_dongle *second = coder->left_dongle < coder->right_dongle ? coder->right_dongle : coder->left_dongle;
			
			pthread_mutex_lock(&first->mutex);
			pthread_mutex_lock(&second->mutex);
			
			if (!first->is_held && !second->is_held)
			{
				if (get_current_time_ms() >= first->available_at &&
					get_current_time_ms() >= second->available_at)
				{
					first->is_held = 1;
					second->is_held = 1;
					pthread_mutex_unlock(&second->mutex);
					pthread_mutex_unlock(&first->mutex);
					break; 
				}
				else
					on_cooldown = 1; 

			}
			pthread_mutex_unlock(&second->mutex);
			pthread_mutex_unlock(&first->mutex);
		}
		if (on_cooldown)
		{
			pthread_mutex_unlock(&coder->sim->state_mutex);
			usleep(500);
			pthread_mutex_lock(&coder->sim->state_mutex);
		}
		else
			pthread_cond_wait(&coder->sim->arbiter_cond, &coder->sim->state_mutex);
	}
	remove_node(coder->sim, my_node);
	free(my_node);
	
	long long current_time = get_current_time_ms() - coder->sim->start_time;
	printf("%lld %d has taken a dongle\n", current_time, coder->id);
	printf("%lld %d has taken a dongle\n", current_time, coder->id);
	
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (0);
}

void release_both_dongles(t_coder *coder)
{
	long long current = get_current_time_ms();
	long long next_avail = current + coder->sim->dongle_cooldown;
	
	t_dongle *first = coder->left_dongle < coder->right_dongle ? coder->left_dongle : coder->right_dongle;
	t_dongle *second = coder->left_dongle < coder->right_dongle ? coder->right_dongle : coder->left_dongle;
	
	pthread_mutex_lock(&first->mutex);
	pthread_mutex_lock(&second->mutex);
	
	first->is_held = 0;
	first->available_at = next_avail;
	second->is_held = 0;
	second->available_at = next_avail;
	
	pthread_mutex_unlock(&second->mutex);
	pthread_mutex_unlock(&first->mutex);
	
	pthread_mutex_lock(&coder->sim->state_mutex);
	pthread_cond_broadcast(&coder->sim->arbiter_cond);
	pthread_mutex_unlock(&coder->sim->state_mutex);
}

void	*coder_routine(void *arg)
{
	t_coder		*coder = (t_coder *)arg;
	t_sim		*sim = coder->sim;


	pthread_mutex_lock(&sim->state_mutex);
	while (sim->threads_ready == 0)
		pthread_cond_wait(&sim->start_cond, &sim->state_mutex);
	pthread_mutex_unlock(&sim->state_mutex);

	if (sim->num_coders == 1)
	{
		precise_sleep(sim->time_to_burnout + 10, sim); 
		return (NULL);
	}

	while (1)
	{
		pthread_mutex_lock(&sim->state_mutex);
		if (!sim->is_active)
		{
			pthread_mutex_unlock(&sim->state_mutex);
			break ;
		}
		pthread_mutex_unlock(&sim->state_mutex);

		if (take_both_dongles(coder) != 0) break;

		pthread_mutex_lock(&sim->state_mutex);
		coder->last_compile_start = get_current_time_ms();
		coder->compiles_done++;
		pthread_mutex_unlock(&sim->state_mutex);

		print_action(coder, "is compiling");
		precise_sleep(sim->time_to_compile, sim);

		release_both_dongles(coder);

		print_action(coder, "is debugging");
		precise_sleep(sim->time_to_debug, sim);

		print_action(coder, "is refactoring");
		precise_sleep(sim->time_to_refactor, sim);
	}
	return (NULL);
}

