#include "bayes_opt.h"

// These would be generated / provided by GenoM3 runtime
extern void write_params_out(double *params);
extern void write_status_out(const char *msg);
extern void write_best_value_out(double v);
extern void write_best_params_out(double *params);

void optimize_step(bayes_opt_state_t *st)
{
    if (!st->running)
        return;

    if (!st->has_result || !st->allow_flag)
        return;

    double y = st->last_value;

    if (st->iteration == 0 || y > st->best_value)
    {
        st->best_value = y;

        for (int i = 0; i < st->dim; i++)
            st->best_params[i] = st->current_params[i];
    }

    propose_next_params(st);

    write_params_out(st->current_params);

    st->iteration++;
    st->has_result = false;
    st->allow_flag = false;

    write_status_out("iteration updated");
}