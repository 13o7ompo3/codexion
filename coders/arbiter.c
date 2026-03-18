#include "codexion.h"

void	wait_for_hardware(t_coder *coder)
{
	long long	now;
	long long	wait_time;
	long long	left_avail;
	long long	right_avail;
	long long	max_avail;

	wait_time = 0;
	pthread_mutex_lock(&coder->sim->state_mutex);
	left_avail = coder->left_dongle->available_at;
	right_avail = coder->right_dongle->available_at;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	if (left_avail > right_avail)
		max_avail = left_avail;
	else
		max_avail = right_avail;
	now = get_current_time_ms();
	if (max_avail > now)
	{
		wait_time = max_avail - now;
		precise_sleep(wait_time, coder->sim);
	}
}

static int	has_priority(t_coder *me, t_coder *other)
{
	long long	my_dl;
	long long	other_dl;

	if (other->state != HUNGRY)
		return (1);
	if (me->sim->scheduler_type == 1)
	{
		my_dl = get_deadline(me);
		other_dl = get_deadline(other);
		if (my_dl < other_dl)
			return (1);
		if (my_dl > other_dl)
			return (0);
	}
	if (me->compiles_done < other->compiles_done)
		return (1);
	if (me->compiles_done > other->compiles_done)
		return (0);
	if (me->id % 2 != 0 && other->id % 2 == 0)
		return (1);
	return (0);
}

static int	can_i_compile(t_coder *coder)
{
	int		i;
	int		left_idx;
	int		right_idx;
	t_coder	*left;
	t_coder	*right;

	i = coder->id - 1;
	left_idx = (i - 1 + coder->sim->num_coders) % coder->sim->num_coders;
	right_idx = (i + 1) % coder->sim->num_coders;
	left = &coder->sim->coders[left_idx];
	right = &coder->sim->coders[right_idx];
	if (left->state == COMPILING || right->state == COMPILING)
		return (0);
	if (!has_priority(coder, left))
		return (0);
	if (!has_priority(coder, right))
		return (0);
	return (1);
}

static int	evaluate_and_take(t_coder *coder)
{
	long long	now;

	if (!coder->sim->is_active)
		return (2);
	now = get_current_time_ms();
	if (coder->left_dongle->available_at > now
		|| coder->right_dongle->available_at > now)
		return (1);
	if (can_i_compile(coder))
	{
		coder->state = COMPILING;
		coder->last_compile_start = get_current_time_ms();
		coder->compiles_done++;
		return (0);
	}
	return (3);
}

int	take_both_dongles(t_coder *coder)
{
	int	status;

	pthread_mutex_lock(&coder->sim->state_mutex);
	coder->state = HUNGRY;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	while (1)
	{
		wait_for_hardware(coder);
		pthread_mutex_lock(&coder->sim->state_mutex);
		status = evaluate_and_take(coder);
		if (status == 2)
		{
			pthread_mutex_unlock(&coder->sim->state_mutex);
			return (1);
		}
		if (status == 0)
			break ;
		if (status == 3)
			pthread_cond_wait(&coder->wakeup_cond, &coder->sim->state_mutex);
		pthread_mutex_unlock(&coder->sim->state_mutex);
	}
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (0);
}

void	release_both_dongles(t_coder *coder)
{
	int			i;
	int			left_idx;
	int			right_idx;
	long long	next_avail;

	i = coder->id - 1;
	left_idx = (i - 1 + coder->sim->num_coders) % coder->sim->num_coders;
	right_idx = (i + 1) % coder->sim->num_coders;
	pthread_mutex_lock(&coder->sim->state_mutex);
	coder->state = THINKING;
	next_avail = get_current_time_ms() + coder->sim->dongle_cooldown;
	coder->left_dongle->available_at = next_avail;
	coder->right_dongle->available_at = next_avail;
	pthread_cond_signal(&coder->sim->coders[left_idx].wakeup_cond);
	pthread_cond_signal(&coder->sim->coders[right_idx].wakeup_cond);
	pthread_mutex_unlock(&coder->sim->state_mutex);
}
