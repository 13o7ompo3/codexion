#include "codexion.h"
#include <unistd.h>

static int	is_sim_active(t_sim *sim)
{
	int	active;

	pthread_mutex_lock(&sim->queue_mutex);
	active = sim->is_active;
	pthread_mutex_unlock(&sim->queue_mutex);
	return (active);
}

// void	precise_sleep(long long time_in_ms, t_sim *sim)
// {
// 	long long	start;

// 	start = get_current_time_ms();
// 	while (is_sim_active(sim))
// 	{
// 		if (get_current_time_ms() - start >= time_in_ms)
// 			break ;
// 		usleep(500);
// 	}
// }

// void precise_sleep(long long time_in_ms, t_coder *coder)
// {
// 	pthread_mutex_lock(&coder->coder_mutex);
// 	pthread_cond_timedwait(&coder->wakeup_cond, &coder->coder_mutex, &(struct timespec){
// 		.tv_sec = time_in_ms / 1000,
// 		.tv_nsec = (time_in_ms % 1000) * 1000000
// 	});
// 	pthread_mutex_unlock(&coder->coder_mutex);
// 	return;
// }

int	take_both_dongles(t_coder *coder)
{
	pthread_mutex_lock(&coder->sim->queue_mutex);


	pthread_mutex_lock(&coder->coder_mutex);
	if (coder->sim->scheduler_type == 0)
		coder->deadline = coder->last_compile_start + coder->sim->time_to_burnout;
	else
		coder->deadline = get_current_time_ms();
	pthread_mutex_unlock(&coder->coder_mutex);
	debug_log(coder->sim, "joining queue. Deadline:", coder->id, coder->deadline, coder->compiles_done);

	heap_insert(coder->sim->queue, coder);

	pthread_cond_broadcast(&coder->sim->waiter_cond);

	while (coder->sim->is_active && !coder->owns_hardware)
		pthread_cond_wait(&coder->wakeup_cond, &coder->sim->queue_mutex);

	pthread_mutex_unlock(&coder->sim->queue_mutex);

	if (!is_sim_active(coder->sim))
		return (1);
	return (0);
}

void	release_both_dongles(t_coder *coder)
{
	long long	now;

	pthread_mutex_lock(&coder->sim->queue_mutex);

	coder->owns_hardware = 0;
	now = get_current_time_ms();
	coder->left_dongle->available_at = now + coder->sim->dongle_cooldown;
	coder->right_dongle->available_at = coder->left_dongle->available_at;
	
	// Physically release the hardware
	coder->left_dongle->in_use = 0;
	coder->right_dongle->in_use = 0;

	pthread_cond_broadcast(&coder->sim->waiter_cond);

	pthread_mutex_unlock(&coder->sim->queue_mutex);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder = (t_coder *)arg;
	t_sim	*sim = coder->sim;

	pthread_mutex_lock(&sim->queue_mutex);
	while (sim->threads_ready == 0)
		pthread_cond_wait(&sim->start_cond, &sim->queue_mutex);
	pthread_mutex_lock(&coder->coder_mutex);

	coder->last_compile_start = sim->start_time;
	pthread_mutex_unlock(&coder->coder_mutex);
	pthread_mutex_unlock(&sim->queue_mutex);

	if (sim->num_coders == 1)
	{
		print_action(coder, "has taken a dongle");
		request_sleep(coder, sim->time_to_burnout + 10);
		return (NULL);
	}
	if (coder->id % 2 == 0)
		request_sleep(coder, (sim->time_to_compile + sim->time_to_debug + sim->time_to_refactor + sim->dongle_cooldown) / 2);

	while (is_sim_active(sim))
	{
		if (take_both_dongles(coder) != 0)
			break;

		pthread_mutex_lock(&coder->coder_mutex);
		coder->last_compile_start = get_current_time_ms();
		coder->compiles_done++;
		pthread_mutex_unlock(&coder->coder_mutex);

		print_compiling_sequence(coder);
		request_sleep(coder, sim->time_to_compile);

		release_both_dongles(coder);
        if (sim->required_compiles != -1 && coder->compiles_done >= sim->required_compiles)
            break;

		print_action(coder, "is debugging");
		request_sleep(coder, sim->time_to_debug);

		print_action(coder, "is refactoring");
		request_sleep(coder, sim->time_to_refactor);
	}
	return (NULL);
}
