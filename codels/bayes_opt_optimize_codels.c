#include "acbayes_opt.h"

#include "bayes_opt_c_types.h"

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <time.h>
#include <math.h>
#include <stdio.h>



/* --- Task optimize ---------------------------------------------------- */


/* --- Activity Init ---------------------------------------------------- */

/** Codel boInit of activity Init.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_INVALID_BOUNDS, bayes_opt_e_sys.
 */
genom_event
boInit(const double lower_bounds[5], const double upper_bounds[5],
       int32_t max_iterations, bayes_opt_state *state,
       const genom_context self)
{
  for (int i = 0; i < 5; i++) {
    if (lower_bounds[i] >= upper_bounds[i])
    {
      fprintf(stderr, ">>> Lower bounds can't be higher than upper bounds!\n");
      fflush(stderr);
      return bayes_opt_INVALID_BOUNDS(self);
    }
      

    state->lower_bounds[i] = lower_bounds[i];
    state->upper_bounds[i] = upper_bounds[i];
  }

  state->running = false;
  state->initialized = true;
  state->current_iteration = 0;
  state->max_iterations = max_iterations;
  state->current_score = DBL_MAX;
  state->best_value = DBL_MAX;

  memset(&state->current_params, 0, sizeof(state->current_params));
  memset(&state->best_params, 0, sizeof(state->best_params));

  srand((unsigned int)time(NULL));

  fprintf(stderr, "boInit called\n"); 
  fflush(stderr);

  return bayes_opt_ether;
}


/* --- Activity AskNext ------------------------------------------------- */

/** Codel boProposeParams of activity AskNext.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_OPTIMIZATION_FAILED.
 */
genom_event
boProposeParams(bayes_opt_state *state,
                bayes_opt_suggestion *params_out,
                const genom_context self)
{
  if (!state->initialized)
  {
    fprintf(stderr, ">>> Optimization has failed\n");
    fflush(stderr);   
    return bayes_opt_OPTIMIZATION_FAILED(self); 
  }
    

  if (state->current_iteration >= state->max_iterations)
  {
    fprintf(stderr, ">>> Optimization has failed\n");
    fflush(stderr);
    return bayes_opt_OPTIMIZATION_FAILED(self);
  }

  for (int i = 0; i < 5; i++) {
    double r = (double)rand() / (double)RAND_MAX;
    params_out->params[i] =
      state->lower_bounds[i] +
      r * (state->upper_bounds[i] - state->lower_bounds[i]);
  }

  params_out->iteration = state->current_iteration;

  state->current_params = *params_out;
  state->running = true;

  fprintf(stderr, "boProposeParams called\n");
  fflush(stderr);
  return bayes_opt_ether;
}


/* --- Activity SubmitResult -------------------------------------------- */

/** Codel boUpdateOptimizer of activity SubmitResult.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_INVALID_PARAMETER, bayes_opt_EVALUATION_FAILED,
 *        bayes_opt_NO_SCORE_AVAILABLE.
 */
genom_event
boUpdateOptimizer(double score, bayes_opt_state *state,
                  const genom_context self)
{
  if (!state->initialized)
  {
    fprintf(stderr, ">>> Evaluation has failed\n");
    fflush(stderr);
    return bayes_opt_EVALUATION_FAILED(self);
  }
    

  if (!isfinite(score))
  {
    fprintf(stderr, ">>> Evaluation has failed\n");
    fflush(stderr);
    return bayes_opt_INVALID_PARAMETER(self);
  }

  state->current_score = score;

  /* Assumption: lower score is better */
  if (score < state->best_value) {
    state->best_value = score;

    for (int i = 0; i < 5; i++)
      state->best_params.params[i] = state->current_params.params[i];

    state->best_params.value = score;
  }

  state->current_iteration++;
  state->running = false;

  fprintf(stderr, ">>> boUpdateOptimizer called, score=%f\n", score);
  fflush(stderr);
  return bayes_opt_ether;
}


/* --- Activity GetBest ------------------------------------------------- */

/** Codel boGetBest of activity GetBest.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_NO_SCORE_AVAILABLE.
 */
genom_event
boGetBest(const bayes_opt_state *state,
          bayes_opt_best *best_result_out,
          const genom_context self)
{
  if (!state->initialized || state->best_value == DBL_MAX)
  {
    fprintf(stderr, ">>> No avaliable score yet\n");
    fflush(stderr);    
    return bayes_opt_NO_SCORE_AVAILABLE(self);
  }
    

  *best_result_out = state->best_params;

  fprintf(stderr, ">>> boGetBest called\n");
  fflush(stderr);
  return bayes_opt_ether;
}
