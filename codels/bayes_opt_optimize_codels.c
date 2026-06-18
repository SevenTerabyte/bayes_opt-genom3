#include "acbayes_opt.h"

#include "bayes_opt_c_types.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_PARAMS 32

/* --- Task optimize ---------------------------------------------------- */


/** Codel init of task optimize.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_wait_result.
 * Throws bayes_opt_e_sys.
 */
genom_event
init(bayes_opt_state *state, const genom_context self)
{
  state->running = true;
  state->initialized = true;

  state->current_iteration = 0;
  state->current_score = 0.0;
  state->best_value = -1e100;

  return bayes_opt_wait_result;
}


/** Codel wait_result of task optimize.
 *
 * Triggered by bayes_opt_wait_result.
 * Yields to bayes_opt_compute, bayes_opt_wait_result,
 *           bayes_opt_failed.
 * Throws bayes_opt_e_sys.
 */
genom_event
wait_result(const bayes_opt_result *result,
            const bayes_opt_allow *allow,
            bayes_opt_state *state,
            const genom_context self)
{
  /* no result yet */
  if (result == NULL) {
    return bayes_opt_wait_result;
  }

  /* wait for explicit allow flag */
  if (!(*(allow->data(self)))) {
    return bayes_opt_wait_result;
  }

  /* store incoming score */
  state->current_score = *(result->data(self));

  return bayes_opt_compute;
}


/** Codel compute of task optimize.
 *
 * Triggered by bayes_opt_compute.
 * Yields to bayes_opt_publish, bayes_opt_wait_result.
 * Throws bayes_opt_e_sys.
 */
genom_event
compute(const bayes_opt_result *result,
        const bayes_opt_allow *allow,
        bayes_opt_state *state,
        const bayes_opt_params *params,
        const genom_context self)
{
  long i;

  /* copy current params */
  for (i = 0; i < MAX_PARAMS; i++) {
    state->current_params[i] = *(params->data(self) + i);
  }

  /* update iteration */
  state->current_iteration++;

  /* update best */
  if (state->current_score > state->best_value) {
    state->best_value = state->current_score;

    for (i = 0; i < MAX_PARAMS; i++) {
      state->best_params[i] = state->current_params[i];
    }
  }

  return bayes_opt_publish;
}


/** Codel publish of task optimize.
 *
 * Triggered by bayes_opt_publish.
 * Yields to bayes_opt_wait_result.
 * Throws bayes_opt_e_sys.
 */
genom_event
publish(bayes_opt_state *state,
        const bayes_opt_params *params,
        const bayes_opt_best_value *best_value,
        const bayes_opt_best_params *best_params,
        const bayes_opt_status *status,
        const genom_context self)
{
  long i;

  /* best value output */
  *(best_value->data(self)) = state->best_value;

  /* best params output */
  for (i = 0; i < MAX_PARAMS; i++) {
    *(best_params->data(self) + i) = state->best_params[i];
  }

  /* status string */
  snprintf(*(status->data(self)), 128,
          "iter=%ld best=%.6f",
          state->current_iteration,
          state->best_value);

  return bayes_opt_wait_result;
}


/** Codel failed of task optimize.
 *
 * Triggered by bayes_opt_failed.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_e_sys.
 */
genom_event
failed(bayes_opt_state *state, const genom_context self)
{
  state->running = false;
  return bayes_opt_ether;
}


/* --- Activity Init ---------------------------------------------------- */

/** Codel fake_start of activity Init.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_e_sys.
 */
genom_event
fake_start(const genom_context self)
{
  /* skeleton sample: insert your code */
  /* skeleton sample */ return bayes_opt_ether;
}

/** Codel fake_stop of activity Init.
 *
 * Triggered by bayes_opt_stop.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_e_sys.
 */
genom_event
fake_stop(const genom_context self)
{
  /* skeleton sample: insert your code */
  /* skeleton sample */ return bayes_opt_ether;
}


/* --- Activity AskNext ------------------------------------------------- */

/** Codel boProposeParams of activity AskNext.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_end, bayes_opt_ether.
 * Throws bayes_opt_e_sys, bayes_opt_OPTIMIZATION_FAILED.
 */
genom_event
boProposeParams(bayes_opt_state *state,
                const bayes_opt_params *params,
                const genom_context self)
{
  long i;

  /* simple exploration strategy (placeholder BO) */
  for (i = 0; i < MAX_PARAMS; i++) {

    double low = state->lower_bounds[i % 5];
    double high = state->upper_bounds[i % 5];

    /* pseudo-random / exploration step */
    double r = ((double) rand() / RAND_MAX);

    state->current_params[i] = low + r * (high - low);
    *(params->data(self) + i) = state->current_params[i];
  }

  return bayes_opt_end;
}

/** Codel fake_end of activity AskNext.
 *
 * Triggered by bayes_opt_end.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_e_sys, bayes_opt_OPTIMIZATION_FAILED.
 */
genom_event
fake_end(const genom_context self)
{
  /* skeleton sample: insert your code */
  /* skeleton sample */ return bayes_opt_ether;
}


/* --- Activity SubmitResult -------------------------------------------- */

/** Codel boUpdateOptimizer of activity SubmitResult.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_end, bayes_opt_ether.
 * Throws bayes_opt_e_sys, bayes_opt_INVALID_PARAMETER,
 *        bayes_opt_EVALUATION_FAILED.
 */
genom_event
boUpdateOptimizer(double score,
                  bayes_opt_state *state,
                  const genom_context self)
{
  state->current_score = score;

  if (score > state->best_value) {
    state->best_value = score;
  }

  return bayes_opt_end;
}

/** Codel fake_end of activity SubmitResult.
 *
 * Triggered by bayes_opt_end.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_e_sys, bayes_opt_INVALID_PARAMETER,
 *        bayes_opt_EVALUATION_FAILED.
 */
/* already defined in service AskNext */



/* --- Activity GetBest ------------------------------------------------- */

/** Codel fake_start of activity GetBest.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_e_sys.
 */
/* already defined in service Init */


/** Codel fake_stop of activity GetBest.
 *
 * Triggered by bayes_opt_stop.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_e_sys.
 */
/* already defined in service Init */

