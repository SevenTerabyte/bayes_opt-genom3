#include "acbayes_opt.h"

#include "bayes_opt_c_types.h"

#include <stdio.h>

/* --- Activity AskNext ------------------------------------------------- */

/** Validation codel checkInitialized of activity AskNext.
 *
 * Returns genom_ok.
 * Throws bayes_opt_NOT_INITIALIZED, bayes_opt_OPTIMIZATION_FAILED.
 */
genom_event
checkInitialized(const bayes_opt_state *state,
                 const genom_context self)
{
  fprintf(stderr, ">>> checkInitialized called\n");
  fflush(stderr);

  if (!state->initialized) {
    fprintf(stderr, ">>> checkInitialized failed: not initialized\n");
    fflush(stderr);
    return bayes_opt_NOT_INITIALIZED(self);
  }

  fprintf(stderr, ">>> checkInitialized OK\n");
  fflush(stderr);
  return genom_ok;
}


/* --- Activity UpdateFromMeasure --------------------------------------- */

/** Validation codel checkInitialized of activity UpdateFromMeasure.
 *
 * Returns genom_ok.
 * Throws bayes_opt_NOT_INITIALIZED, bayes_opt_NO_MEASUREMENT,
 * bayes_opt_OPTIMIZATION_FAILED.
 */
/* already defined in service AskNext validation */



/* --- Activity GetBest ------------------------------------------------- */

/** Validation codel checkInitialized of activity GetBest.
 *
 * Returns genom_ok.
 * Throws bayes_opt_NOT_INITIALIZED, bayes_opt_NO_BEST_RESULT.
 */
/* already defined in service AskNext validation */