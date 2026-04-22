#include "codexion.h"
#include <string.h>

static int	try_wake_coder(t_sim *sim, int i, long long now)
static int	try_wake_coder(t_sim *sim, int i, long long now, int *reserved)
{
	t_coder	*coder;
	int		left_avail;
	int		right_avail;

	coder = sim->queue->array[i];
	debug_log(sim, "Waiter checking heap index", i, coder->left_dongle->available_at, now);
	if (coder->left_dongle->available_at <= now && !coder->left_dongle->in_use
		&& coder->right_dongle->available_at <= now && !coder->right_dongle->in_use)
	left_avail = (coder->left_dongle->available_at <= now && !coder->left_dongle->in_use);
	right_avail = (coder->right_dongle->available_at <= now && !coder->right_dongle->in_use);
	if (left_avail && right_avail)
	{
		debug_log(sim, "WAITER APPROVED", coder->id, 0, 0);
		coder->left_dongle->in_use = 1;
		coder->right_dongle->in_use = 1;
		heap_remove_at(sim->queue, i);
		coder->owns_hardware = 1;
		pthread_cond_broadcast(&coder->wakeup_cond);
		// coder->left_dongle->available_at = now + coder->sim->dongle_cooldown + coder->sim->time_to_compile;
		// coder->right_dongle->available_at = coder->left_dongle->available_at;
		
		return (1);
		if (!reserved[coder->left_dongle->id] && !reserved[coder->right_dongle->id])
		{
			coder->left_dongle->in_use = 1;
			coder->right_dongle->in_use = 1;
			heap_remove_at(sim->queue, i);
			coder->owns_hardware = 1;
			pthread_cond_broadcast(&coder->wakeup_cond);
			return (1);
		}
	}
	else
	{
		reserved[coder->left_dongle->id] = 1;
		reserved[coder->right_dongle->id] = 1;
	}
	return (0);
}

void	*waiter_routine(void *arg)
{
	t_sim		*sim = (t_sim *)arg;
	int			woke_someone;
	int			i;
	long long	frozen_now;
	int			*reserved;

	reserved = calloc(sim->num_coders, sizeof(int));
	if (!reserved)
		return (NULL);

	pthread_mutex_lock(&sim->queue_mutex);
	while (sim->is_active)
	{
		// 1. If the queue is empty, go to sleep until someone rings the bell.
		if (sim->queue->size == 0)
		{
			pthread_cond_wait(&sim->waiter_cond, &sim->queue_mutex);
			continue;
		}
		memset(reserved, 0, sim->num_coders * sizeof(int));
		i = 0;
		woke_someone = 0;
		frozen_now = get_current_time_ms();
		while (i < sim->queue->size)
		{
			/* Process each coder in the queue */
			if (try_wake_coder(sim, i, frozen_now))
			if (try_wake_coder(sim, i, frozen_now, reserved))
			{
				woke_someone = 1;
				// i = 1;
				break;
			}
			i++;
		}
		if (!woke_someone)
		{
			pthread_mutex_unlock(&sim->queue_mutex);
			usleep(500); // Prevent 100% CPU spin-locking
			pthread_mutex_lock(&sim->queue_mutex);
		}
	}
	pthread_mutex_unlock(&sim->queue_mutex);
	free(reserved);
	return (NULL);
}