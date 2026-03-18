#include "codexion.h"
#include <unistd.h>


static void	wait_starting_gun(t_coder *coder, t_sim *sim)
{
	pthread_mutex_lock(&sim->state_mutex);
	while (sim->threads_ready == 0)
		pthread_cond_wait(&sim->start_cond, &sim->state_mutex);
	coder->last_compile_start = sim->start_time;
	pthread_mutex_unlock(&sim->state_mutex);
}

static int	check_if_finished(t_coder *coder, t_sim *sim)
{
	int	finished;

	finished = 0;
	pthread_mutex_lock(&sim->state_mutex);
	if (sim->required_compiles != -1)
	{
		if (coder->compiles_done >= sim->required_compiles)
		{
			coder->is_finished = 1;
			finished = 1;
		}
	}
	pthread_mutex_unlock(&sim->state_mutex);
	if (finished)
		release_both_dongles(coder);
	return (finished);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;
	t_sim	*sim;

	coder = (t_coder *)arg;
	sim = coder->sim;
	wait_starting_gun(coder, sim);
	while (1)
	{
		if (take_both_dongles(coder) != 0)
			break ;
		print_compiling_sequence(coder);
		precise_sleep(sim->time_to_compile, sim);
		if (check_if_finished(coder, sim))
			break ;
		release_both_dongles(coder);
		print_action(coder, "is debugging");
		precise_sleep(sim->time_to_debug, sim);
		print_action(coder, "is refactoring");
		precise_sleep(sim->time_to_refactor, sim);
	}
	return (NULL);
}

