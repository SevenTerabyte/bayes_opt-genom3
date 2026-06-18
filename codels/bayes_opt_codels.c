#include "acbayes_opt.h"

#include "bayes_opt_c_types.h"
#include <stdio.h>
#include <stdlib.h>

/* --- Activity AskNext ------------------------------------------------- */

/** Validation codel checkInitialized of activity AskNext.
 *
 * Returns genom_ok.
 * Throws bayes_opt_e_sys, bayes_opt_OPTIMIZATION_FAILED.
 */
genom_event
checkInitialized(const bayes_opt_state *state,
                 const genom_context self)
{
  if (!state->initialized || !state->running) {
    return bayes_opt_OPTIMIZATION_FAILED;
  }

  return genom_ok;
}


/* --- Activity SubmitResult -------------------------------------------- */

/** Validation codel checkInitialized of activity SubmitResult.
 *
 * Returns genom_ok.
 * Throws bayes_opt_e_sys, bayes_opt_INVALID_PARAMETER,
 * bayes_opt_EVALUATION_FAILED.
 */
/* already defined in service AskNext validation */

