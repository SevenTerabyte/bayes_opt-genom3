#include "acbayes_opt.h"

#include "bayes_opt_c_types.h"

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <time.h>
#include <math.h>
#include <stdio.h>


/* --- Activity AskNext ------------------------------------------------- */

/** Validation codel checkInitialized of activity AskNext.
 *
 * Returns genom_ok.
 * Throws bayes_opt_OPTIMIZATION_FAILED.
 */
genom_event
checkInitialized(const bayes_opt_state *state,
                 const genom_context self)
{
  if (!state->initialized)
  {
    fprintf(stderr, ">>> Optimization has failed\n");
    fflush(stderr);
    return bayes_opt_OPTIMIZATION_FAILED(self);
  }

  fprintf(stderr, ">>> checkInitialized called\n");
  fflush(stderr);
  return genom_ok;
}


/* --- Activity SubmitResult -------------------------------------------- */

/** Validation codel checkInitialized of activity SubmitResult.
 *
 * Returns genom_ok.
 * Throws bayes_opt_INVALID_PARAMETER, bayes_opt_EVALUATION_FAILED,
 * bayes_opt_NO_SCORE_AVAILABLE.
 */
/* already defined in service AskNext validation */



/* --- Activity GetBest ------------------------------------------------- */

/** Validation codel checkInitialized of activity GetBest.
 *
 * Returns genom_ok.
 * Throws bayes_opt_NO_SCORE_AVAILABLE.
 */
/* already defined in service AskNext validation */

