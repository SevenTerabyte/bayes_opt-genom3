#include "acbayes_opt.h"

#include "bayes_opt_c_types.h"


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
  /* skeleton sample: insert your code */
  /* skeleton sample */ return bayes_opt_wait_result;
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
            const bayes_opt_allow *allow, bayes_opt_state *state,
            const genom_context self)
{
  /* skeleton sample: insert your code */
  /* skeleton sample */ return bayes_opt_compute;
}


/** Codel compute of task optimize.
 *
 * Triggered by bayes_opt_compute.
 * Yields to bayes_opt_publish, bayes_opt_wait_result.
 * Throws bayes_opt_e_sys.
 */
genom_event
compute(const bayes_opt_result *result, const bayes_opt_allow *allow,
        bayes_opt_state *state, const bayes_opt_params *params,
        const genom_context self)
{
  /* skeleton sample: insert your code */
  /* skeleton sample */ return bayes_opt_publish;
}


/** Codel publish of task optimize.
 *
 * Triggered by bayes_opt_publish.
 * Yields to bayes_opt_wait_result.
 * Throws bayes_opt_e_sys.
 */
genom_event
publish(bayes_opt_state *state, const bayes_opt_params *params,
        const bayes_opt_best_value *best_value,
        const bayes_opt_best_params *best_params,
        const bayes_opt_status *status, const genom_context self)
{
  /* skeleton sample: insert your code */
  /* skeleton sample */ return bayes_opt_wait_result;
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
  /* skeleton sample: insert your code */
  /* skeleton sample */ return bayes_opt_ether;
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
boProposeParams(bayes_opt_state *state, const bayes_opt_params *params,
                const genom_context self)
{
  /* skeleton sample: insert your code */
  /* skeleton sample */ return bayes_opt_end;
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
boUpdateOptimizer(double score, bayes_opt_state *state,
                  const genom_context self)
{
  /* skeleton sample: insert your code */
  /* skeleton sample */ return bayes_opt_end;
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

