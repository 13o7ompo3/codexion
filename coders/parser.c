#include "codexion.h"
#include <stdlib.h>
#include <string.h>

int	init_simulation(t_sim *sim)
{
	int	i;

	sim->dongles = malloc(sizeof(t_dongle) * sim->num_coders);
	sim->coders = malloc(sizeof(t_coder) * sim->num_coders);
	if (!sim->dongles || !sim->coders)
		return (1);
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_mutex_init(&sim->dongles[i].mutex, NULL);
		pthread_cond_init(&sim->dongles[i].cond, NULL);
		sim->dongles[i].available_at = 0;
		sim->dongles[i].queue = NULL;
		sim->coders[i].id = i + 1;
		sim->coders[i].compiles_done = 0;
		sim->coders[i].sim = sim;
		sim->coders[i].left_dongle = &sim->dongles[i];
		sim->coders[i].right_dongle = &sim->dongles[(i + 1) % sim->num_coders];
		i++;
	}
	return (0);
}

int	validat_args(int argc, char **argv)
{
	int	i;
	int	j;

	if (argc != 9 || (strcmp(argv[8], "edf") && strcmp(argv[8], "fifo")))
		return (1);
	i = 1;
	while (i < argc - 1)
	{
		if ((strlen(argv[i]) > 10 + (argv[i][0] == '+')) ||
		((strlen(argv[i]) == 10 + (argv[i][0] == '+'))
			&& argv[i][(argv[i][0] == '+')] > '2') ||
		(atoi(argv[i]) < 0))
			return (1);
		j = 0;
		if (argv[i][j] == '+')
			j++;
		while (argv[i][j])
		{
			if (argv[i][j] < '0' || argv[i][j] > '9')
				return (1);
			j++;
		}
		i++;
	}
	return (0);
}

int	parse_args(int argc, char **argv, t_sim *sim)
{
	sim->coders = NULL;
	sim->dongles = NULL;
	if (validat_args(argc, argv))
		return (1);
	sim->num_coders = atoi(argv[1]);
	sim->time_to_burnout = atoi(argv[2]);
	sim->time_to_compile = atoi(argv[3]);
	sim->time_to_debug = atoi(argv[4]);
	sim->time_to_refactor = atoi(argv[5]);
	sim->required_compiles = atoi(argv[6]);
	sim->dongle_cooldown = atoi(argv[7]);
	sim->scheduler_type = 1;
	if (!strcmp(argv[8], "edf"))
		sim->scheduler_type = 0;
	sim->is_active = 1;
	sim->threads_ready = 0;
	pthread_cond_init(&sim->start_cond, NULL);
	pthread_mutex_init(&sim->state_mutex, NULL);
	pthread_mutex_init(&sim->write_mutex, NULL);
	if (init_simulation(sim))
		return (2);
	return (0);
}