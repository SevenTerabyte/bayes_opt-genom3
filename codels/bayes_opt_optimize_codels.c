#include "acbayes_opt.h"

#include "bayes_opt_c_types.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>

/* --- Task optimize ---------------------------------------------------- */

static void
fill_status(bayes_opt_status_struct *s,
            const bayes_opt_state *state,
            const char *text)
{
  memset(s, 0, sizeof(*s));
  snprintf(s->text, sizeof(s->text), "%s", text);
  s->iteration = state->current_iteration;
  s->running = state->running;
  s->initialized = state->initialized;
}

/* --- Activity Init ---------------------------------------------------- */

/** Codel boInit of activity Init.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_INVALID_BOUNDS, bayes_opt_e_sys.
 */
genom_event
boInit(const double lower_bounds[5], const double upper_bounds[5],
       int32_t max_iterations, double reference_x, double reference_y,
       double reference_z, bayes_opt_state *state,
       const bayes_opt_status *status, const genom_context self)
{
  fprintf(stderr, ">>> boInit called\n");
  fflush(stderr);

  if (max_iterations <= 0)
    return bayes_opt_INVALID_BOUNDS(self);

  for (int i = 0; i < 5; i++) {
    if (lower_bounds[i] >= upper_bounds[i])
      return bayes_opt_INVALID_BOUNDS(self);

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

  state->reference_x = reference_x;
  state->reference_y = reference_y;
  state->reference_z = reference_z;
  state->sample_count = 0;

  srand((unsigned int)time(NULL));

  bayes_opt_status_struct s;
  fill_status(&s, state, "initialized");

  /* If this function name differs, grep generated files for status_write */
  //bayes_opt_status_write(status, &s);

  fprintf(stderr, ">>> boInit returning\n");
  fflush(stderr);

  return bayes_opt_ether;
}


/* --- Activity AskNext ------------------------------------------------- */

/** Codel boProposeParams of activity AskNext.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_NOT_INITIALIZED, bayes_opt_OPTIMIZATION_FAILED.
 */
genom_event
boProposeParams(bayes_opt_state *state,
                bayes_opt_suggestion *params_out,
                const bayes_opt_params *params,
                const bayes_opt_status *status,
                const genom_context self)
{
  fprintf(stderr, ">>> boProposeParams called\n");
  fflush(stderr);

  if (!state->initialized)
    return bayes_opt_NOT_INITIALIZED(self);

  if (state->current_iteration >= state->max_iterations)
    return bayes_opt_OPTIMIZATION_FAILED(self);

  bayes_opt_suggestion s;
  memset(&s, 0, sizeof(s));

  for (int i = 0; i < 5; i++) {
    double r = (double)rand() / (double)RAND_MAX;
    s.params[i] =
      state->lower_bounds[i] +
      r * (state->upper_bounds[i] - state->lower_bounds[i]);
  }

  s.iteration = state->current_iteration;

  state->current_params = s;
  state->running = true;

  *params_out = s;

  bayes_opt_suggestion *poster = params->data(self);
  *poster = s;

  genom_event ev = params->write(self);
  if (ev != genom_ok)
    return ev;

  fprintf(stderr, ">>> params poster written\n");
  fprintf(stderr, ">>> boProposeParams returning\n");
  fflush(stderr);

  return bayes_opt_ether;
}


/* --- Activity UpdateFromMeasure --------------------------------------- */

/** Codel boUpdateFromMeasure of activity UpdateFromMeasure.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_NOT_INITIALIZED, bayes_opt_NO_MEASUREMENT,
 *        bayes_opt_OPTIMIZATION_FAILED.
 */
genom_event
boUpdateFromMeasure(const bayes_opt_measure *measure,
                    const bayes_opt_allow *allow,
                    bayes_opt_state *state,
                    const bayes_opt_best_result *best_result,
                    const bayes_opt_status *status,
                    const genom_context self)
{
  fprintf(stderr, ">>> boUpdateFromMeasure called\n");
  fflush(stderr);

  if (!state->initialized)
    return bayes_opt_NOT_INITIALIZED(self);

  genom_event ev = measure->read(self);
  if (ev != genom_ok) {
    fprintf(stderr, ">>> measure read failed\n");
    fflush(stderr);
    return bayes_opt_NO_MEASUREMENT(self);
  }

  or_pose_estimator_state *m = measure->data(self);

  if (!m->pos._present) {
    fprintf(stderr, ">>> measure has no position\n");
    fflush(stderr);
    return bayes_opt_NO_MEASUREMENT(self);
  }

  double x = m->pos._value.x;
  double y = m->pos._value.y;
  double z = m->pos._value.z;

  double vx = 0.0;
  double vy = 0.0;
  double vz = 0.0;

  if (m->vel._present) {
    vx = m->vel._value.vx;
    vy = m->vel._value.vy;
    vz = m->vel._value.vz;
  }

  bool allow_update = true;

  ev = allow->read(self);
  if (ev == genom_ok) {
    bayes_opt_control *a = allow->data(self);
    allow_update = a->allow;
    fprintf(stderr, ">>> allow read OK: allow=%d\n", allow_update);
  } else {
    fprintf(stderr, ">>> allow read failed/empty, defaulting allow=true\n");
  }

  if (!allow_update) {
    fprintf(stderr, ">>> update skipped: allow=false\n");
    fflush(stderr);
    return bayes_opt_ether;
  }

  double dz = z - state->reference_z;
  double vel_penalty = fabs(vz);

  double score = fabs(dz) + 0.2 * vel_penalty;

  state->current_score = score;
  state->sample_count++;

  if (score < state->best_value) {
    state->best_value = score;
    state->best_params.value = score;

    for (int i = 0; i < 5; i++)
      state->best_params.params[i] = state->current_params.params[i];

    bayes_opt_best *bposter = best_result->data(self);
    *bposter = state->best_params;

    ev = best_result->write(self);
    if (ev != genom_ok)
      return ev;

    fprintf(stderr, ">>> new best result stored and published\n");
  }

  state->current_iteration++;
  state->running = false;

  bayes_opt_status_struct st;
  fill_status(&st, state, "measurement updated");

  bayes_opt_status_struct *sposter = status->data(self);
  *sposter = st;

  ev = status->write(self);
  if (ev != genom_ok)
    return ev;

  fprintf(stderr,
          ">>> measure: x=%f y=%f z=%f vx=%f vy=%f vz=%f\n",
          x, y, z, vx, vy, vz);

  fprintf(stderr,
          ">>> score=%f, best=%f, iteration=%d, samples=%d\n",
          state->current_score,
          state->best_value,
          state->current_iteration,
          state->sample_count);

  fflush(stderr);

  return bayes_opt_ether;
}


/* --- Activity GetBest ------------------------------------------------- */

/** Codel boGetBest of activity GetBest.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_NOT_INITIALIZED, bayes_opt_NO_BEST_RESULT.
 */
genom_event
boGetBest(const bayes_opt_state *state,
          bayes_opt_best *best_result_out,
          const bayes_opt_best_result *best_result,
          const bayes_opt_status *status,
          const genom_context self)
{
  fprintf(stderr, ">>> boGetBest called\n");
  fflush(stderr);

  if (!state->initialized)
    return bayes_opt_NOT_INITIALIZED(self);

  if (state->best_value == DBL_MAX)
    return bayes_opt_NO_BEST_RESULT(self);

  *best_result_out = state->best_params;

  bayes_opt_best *bposter = best_result->data(self);
  *bposter = state->best_params;

  genom_event ev = best_result->write(self);
  if (ev != genom_ok)
    return ev;

  bayes_opt_status_struct st;
  fill_status(&st, state, "best result returned");

  bayes_opt_status_struct *sposter = status->data(self);
  *sposter = st;

  ev = status->write(self);
  if (ev != genom_ok)
    return ev;

  fprintf(stderr, ">>> best_result poster written\n");
  fprintf(stderr, ">>> boGetBest returning\n");
  fflush(stderr);

  return bayes_opt_ether;
}

/* --- Activity Reset --------------------------------------------------- */

/** Codel boReset of activity Reset.
 *
 * Triggered by bayes_opt_start.
 * Yields to bayes_opt_ether.
 * Throws bayes_opt_e_sys.
 */
genom_event
boReset(bayes_opt_state *state,
        const bayes_opt_status *status,
        const genom_context self)
{
  fprintf(stderr, ">>> boReset called\n");
  fflush(stderr);

  memset(state, 0, sizeof(*state));
  state->best_value = DBL_MAX;
  state->current_score = DBL_MAX;

  bayes_opt_status_struct st;
  fill_status(&st, state, "reset");

  bayes_opt_status_struct *sposter = status->data(self);
  *sposter = st;

  genom_event ev = status->write(self);
  if (ev != genom_ok)
    return ev;

  fprintf(stderr, ">>> boReset returning\n");
  fflush(stderr);

  return bayes_opt_ether;
}