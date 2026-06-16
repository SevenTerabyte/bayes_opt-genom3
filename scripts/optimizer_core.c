#include "bayes_opt.h"
#include <stdlib.h>
#include <math.h>

static double rand01()
{
    return (double)rand() / (double)RAND_MAX;
}

void propose_next_params(bayes_opt_state_t *st)
{
    for (int i = 0; i < st->dim; i++)
    {
        // placeholder exploration strategy
        st->current_params[i] = rand01();
    }
}
