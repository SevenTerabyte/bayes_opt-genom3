#ifndef BAYES_OPT_H
#define BAYES_OPT_H

#include <stdbool.h>

#define MAX_DIM 32

typedef struct
{
    int iteration;
    int max_iterations;

    bool has_result;
    bool allow_flag;
    bool running;

    double last_value;

    double best_value;
    double best_params[MAX_DIM];
    double current_params[MAX_DIM];

    int dim;
} bayes_opt_state_t;

#endif