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

static long long	get_deadline(t_coder *coder)
{
	return coder->last_compile_start + coder->sim->time_to_burnout;
}

static int cleanup_and_abort(t_sim *sim, t_node *node)
{
	remove_node(sim, node);
	free(node);
	pthread_mutex_unlock(&sim->state_mutex);
	return (1);
}

static int finish_take_dongles(t_coder *coder, t_node *node)
{
	remove_node(coder->sim, node);
	free(node);
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (0);
}

static void handle_cooldown_sleep(t_sim *sim)
{
	pthread_mutex_unlock(&sim->state_mutex);
	usleep(500);
	pthread_mutex_lock(&sim->state_mutex);
}

static int check_conflict(t_coder *coder, t_node *my_node)
{
	t_node      *curr;
	long long   my_dl;
	long long   curr_dl;

	curr = (t_node *)coder->sim->queue;
	my_dl = get_deadline(coder);
	while (curr && curr != my_node)
	{
		if (curr->coder->left_dongle == coder->left_dongle || 
			curr->coder->left_dongle == coder->right_dongle ||
			curr->coder->right_dongle == coder->left_dongle ||
			curr->coder->right_dongle == coder->right_dongle)
		{
			curr_dl = get_deadline(curr->coder);
			if (curr_dl < my_dl || (curr_dl == my_dl && 
				curr->coder->compiles_done < coder->compiles_done))
				return (1);
		}
		curr = curr->next;
	}
	return (0);
}

static int try_grab_dongles(t_coder *coder)
{
	t_dongle    *first;
	t_dongle    *second;
	long long   now;

	first = coder->left_dongle < coder->right_dongle ? 
			coder->left_dongle : coder->right_dongle;
	second = coder->left_dongle < coder->right_dongle ? 
			coder->right_dongle : coder->left_dongle;
	pthread_mutex_lock(&first->mutex);
	pthread_mutex_lock(&second->mutex);
	if (!first->is_held && !second->is_held)
	{
		now = get_current_time_ms();
		if (now >= first->available_at && now >= second->available_at)
		{
			first->is_held = 1;
			second->is_held = 1;
			pthread_mutex_unlock(&second->mutex);
			pthread_mutex_unlock(&first->mutex);
			return (0);
		}
		pthread_mutex_unlock(&second->mutex);
		pthread_mutex_unlock(&first->mutex);
		return (1);
	}
	pthread_mutex_unlock(&second->mutex);
	pthread_mutex_unlock(&first->mutex);
	return (2);
}

int	take_both_dongles(t_coder *coder)
{
	t_node  *n;
	int     st;

	n = malloc(sizeof(t_node));
	if (!n)
		return (1);
	n->coder = coder;
	pthread_mutex_lock(&coder->sim->state_mutex);
	enqueue(coder->sim, n, coder->sim->scheduler_type);
	while (1)
	{
		if (!coder->sim->is_active)
			return (cleanup_and_abort(coder->sim, n));
		if (!check_conflict(coder, n))
		{
			st = try_grab_dongles(coder);
			if (st == 0)
				break ;
			if (st == 1)
			{
				handle_cooldown_sleep(coder->sim);
				continue ;
			}
		}
		pthread_cond_wait(&coder->sim->arbiter_cond, &coder->sim->state_mutex);
	}
	return (finish_take_dongles(coder, n));
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

		print_compiling_sequence(coder);
		precise_sleep(sim->time_to_compile, sim);

		release_both_dongles(coder);
		if (coder->compiles_done == coder->sim->required_compiles)
			break;

		print_action(coder, "is debugging");
		precise_sleep(sim->time_to_debug, sim);

		print_action(coder, "is refactoring");
		precise_sleep(sim->time_to_refactor, sim);
	}
	return (NULL);
}

