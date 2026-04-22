#include "codexion.h"

void	request_sleep(t_coder *coder, long long duration_ms)
{
	if (duration_ms == 0)
	{
		// usleep(100);
		return;
	}
	pthread_mutex_lock(&coder->sim->sleep_mutex);
	// 1. Calculate our alarm clock time
	coder->wake_up_time = get_current_time_ms() + duration_ms;
	coder->is_sleeping = 1;
	// 2. Put ourselves in the Sleep Heap
	heap_insert(coder->sim->sleep_heap, coder);
	// 3. Ring the bell in case the Sleep Room staff was asleep
	pthread_cond_signal(&coder->sim->sleep_room_cond);
	// 4. Wait for the Sleep Room to page us when our time is up
	while (coder->sim->is_active && coder->is_sleeping)
		pthread_cond_wait(&coder->wakeup_cond, &coder->sim->sleep_mutex);	
	pthread_mutex_unlock(&coder->sim->sleep_mutex);
}

static int	process_awakenings(t_sim *sim, long long now)
{
	t_coder	*top;
	int		woke_someone;

	woke_someone = 0;
	while (sim->sleep_heap->size > 0)
	{
		top = sim->sleep_heap->array[0];
		if (top->wake_up_time > now)
			break; // if the first one isn't ready, nobody is ready.
		heap_remove_at(sim->sleep_heap, 0);
		top->is_sleeping = 0;
		pthread_cond_broadcast(&top->wakeup_cond);
		woke_someone = 1;
	}
	return (woke_someone);
}

#include <sys/time.h>

void	set_timespec_timeout(struct timespec *ts, long long delay_in_ms)
{
	struct timeval	tv;
	long long		absolute_usec;

	gettimeofday(&tv, NULL);
	absolute_usec = (tv.tv_sec * 1000000LL) + tv.tv_usec + (delay_in_ms * 1000LL);
	
	ts->tv_sec = absolute_usec / 1000000LL;
	ts->tv_nsec = (absolute_usec % 1000000LL) * 1000LL;
}

void	*sleep_room_routine(void *arg)
{
	t_sim			*sim;
	long long		now;
	struct timespec	ts;
	long long		time_until_next;

	sim = (t_sim *)arg;
	pthread_mutex_lock(&sim->sleep_mutex);
	while (sim->is_active)
	{
		// 1. If nobody is sleeping, the Sleep Room staff goes to sleep
		if (sim->sleep_heap->size == 0)
		{
			pthread_cond_wait(&sim->sleep_room_cond, &sim->sleep_mutex);
			continue;
		}
		// 2. Freeze time and process anyone whose alarm is ringing
		now = get_current_time_ms();
		if (!process_awakenings(sim, now))
		{
			// 3. Nobody is ready to wake up yet. 
			time_until_next = sim->sleep_heap->array[0]->wake_up_time - now;
			if (time_until_next < 1)
				time_until_next = 0;
			set_timespec_timeout(&ts, time_until_next);
			pthread_cond_timedwait(&sim->sleep_room_cond, &sim->sleep_mutex, &ts);
		}
	}
	pthread_mutex_unlock(&sim->sleep_mutex);
	return (NULL);
}
