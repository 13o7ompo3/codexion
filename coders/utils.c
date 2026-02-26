#include "codexion.h"
#include <stdio.h>
#include <sys/time.h>
#include <stddef.h>

long long	get_current_time_ms(void)
{
	struct timeval  tv;
	long long       time_in_ms;

	if (gettimeofday(&tv, NULL) == -1)
		return (-1); // Handle potential errors gracefully
	
	time_in_ms = (long long)tv.tv_sec * 1000 + (long long)tv.tv_usec / 1000;
	return (time_in_ms);
}

void	print_action(t_coder *coder, char *action)
{
	long long current_time;

	pthread_mutex_lock(&coder->sim->write_mutex);

	pthread_mutex_lock(&coder->sim->state_mutex);
	if (coder->sim->is_active == 1 || strcmp(action, "burned out") == 0)
	{
		current_time = get_current_time_ms() - coder->sim->start_time;
		printf("%lld %d %s\n", current_time, coder->id, action);
	}

	pthread_mutex_unlock(&coder->sim->state_mutex);
	pthread_mutex_unlock(&coder->sim->write_mutex);
}
