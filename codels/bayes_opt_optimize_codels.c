#include "acbayes_opt.h"
#include "bayes_opt_c_types.h"

#include <arpa/inet.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>


#define DAEMON_HOST "127.0.0.1"
#define DAEMON_PORT 5055

/*
 * Default command used by boInit if the daemon is not already running.
 * Adjust this path if needed.
 */
#define DEFAULT_DAEMON_CMD \
  "/home/lichenjiang/Skygrip_Project/bayes_env/bin/python3 " \
  "/home/lichenjiang/src/bayes_opt-genom3/python/optimiser_daemon.py " \
  ">/tmp/bayes_opt_daemon.log 2>&1 &"


/*------- Daemon interacting -----------------------------------------*/
static int
daemon_send_recv(const char *msg, char *reply, size_t reply_size)
{
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_port = htons(DAEMON_PORT);

  if (inet_pton(AF_INET, DAEMON_HOST, &addr.sin_addr) != 1) {
    close(fd);
    return -1;
  }

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(fd);
    return -1;
  }

  char line[2048];
  snprintf(line, sizeof(line), "%s\n", msg);

  ssize_t sent = send(fd, line, strlen(line), 0);
  if (sent < 0) {
    perror("send");
    close(fd);
    return -1;
  }

  ssize_t n = recv(fd, reply, reply_size - 1, 0);
  if (n < 0) {
    perror("recv");
    close(fd);
    return -1;
  }

  reply[n] = '\0';

  /* strip trailing newline */
  size_t len = strlen(reply);
  while (len > 0 && (reply[len - 1] == '\n' || reply[len - 1] == '\r')) {
    reply[len - 1] = '\0';
    len--;
  }

  close(fd);
  return 0;
}

static int
daemon_start_if_needed(void)
{
  char reply[512];

  if (daemon_send_recv("PING", reply, sizeof(reply)) == 0) {
    if (strncmp(reply, "OK", 2) == 0) {
      fprintf(stderr, ">>> optimizer daemon already running\n");
      return 0;
    }
  }

  const char *cmd = getenv("BAYES_OPT_DAEMON_CMD");
  if (!cmd || strlen(cmd) == 0)
    cmd = DEFAULT_DAEMON_CMD;

  fprintf(stderr, ">>> starting optimizer daemon with command:\n%s\n", cmd);
  fflush(stderr);

  int ret = system(cmd);
  if (ret == -1) {
    fprintf(stderr, ">>> failed to launch optimizer daemon\n");
    return -1;
  }

  for (int i = 0; i < 20; i++) {
    usleep(100000);

    if (daemon_send_recv("PING", reply, sizeof(reply)) == 0) {
      if (strncmp(reply, "OK", 2) == 0) {
        fprintf(stderr, ">>> optimizer daemon started\n");
        return 0;
      }
    }
  }

  fprintf(stderr, ">>> optimizer daemon did not respond after launch\n");
  return -1;
}

static int
daemon_init_bounds(const double lower_bounds[5],
                   const double upper_bounds[5])
{
  char cmd[1024];
  char reply[512];

  snprintf(cmd, sizeof(cmd),
           "INIT %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g",
           lower_bounds[0], upper_bounds[0],
           lower_bounds[1], upper_bounds[1],
           lower_bounds[2], upper_bounds[2],
           lower_bounds[3], upper_bounds[3],
           lower_bounds[4], upper_bounds[4]);

  fprintf(stderr, ">>> daemon init cmd: %s\n", cmd);
  fflush(stderr);

  if (daemon_send_recv(cmd, reply, sizeof(reply)) != 0)
    return -1;

  fprintf(stderr, ">>> daemon init reply: %s\n", reply);
  fflush(stderr);

  if (strncmp(reply, "OK", 2) != 0)
    return -1;

  return 0;
}


static int
daemon_ask(bayes_opt_suggestion *s)
{
  char reply[1024];

  if (daemon_send_recv("ASK", reply, sizeof(reply)) != 0)
    return -1;

  fprintf(stderr, ">>> daemon ask reply: %s\n", reply);
  fflush(stderr);

  double cf, ct, kpz, kvz, kiz;

  int parsed = sscanf(reply,
                      "PARAMS %lf %lf %lf %lf %lf",
                      &cf, &ct, &kpz, &kvz, &kiz);

  if (parsed != 5) {
    fprintf(stderr, ">>> failed to parse daemon PARAMS reply\n");
    return -1;
  }

  s->params[0] = cf;
  s->params[1] = ct;
  s->params[2] = kpz;
  s->params[3] = kvz;
  s->params[4] = kiz;

  fprintf(stderr,
          ">>> daemon params parsed: cf=%g ct=%g kpz=%g kvz=%g kiz=%g\n",
          cf, ct, kpz, kvz, kiz);
  fflush(stderr);

  return 0;
}

static int
daemon_tell(const bayes_opt_suggestion *s, double score)
{
  char cmd[1024];
  char reply[1024];

  snprintf(cmd, sizeof(cmd),
           "TELL %.17g %.17g %.17g %.17g %.17g %.17g",
           s->params[0],
           s->params[1],
           s->params[2],
           s->params[3],
           s->params[4],
           score);

  fprintf(stderr, ">>> daemon tell cmd: %s\n", cmd);
  fflush(stderr);

  if (daemon_send_recv(cmd, reply, sizeof(reply)) != 0)
    return -1;

  fprintf(stderr, ">>> daemon tell reply: %s\n", reply);
  fflush(stderr);

  if (strncmp(reply, "BEST", 4) != 0 && strncmp(reply, "OK", 2) != 0)
    return -1;

  return 0;
}


static int
daemon_reset(void)
{
  char reply[512];

  if (daemon_send_recv("RESET", reply, sizeof(reply)) != 0)
    return -1;

  fprintf(stderr, ">>> daemon reset reply: %s\n", reply);
  fflush(stderr);

  return strncmp(reply, "OK", 2) == 0 ? 0 : -1;
}


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
       double reference_z, double reference_qw, double reference_qx,
       double reference_qy, double reference_qz,
       bayes_opt_state *state,
       const bayes_opt_status *status,
       const genom_context self)
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

  state->reference_qw = reference_qw;
  state->reference_qx = reference_qx;
  state->reference_qy = reference_qy;
  state->reference_qz = reference_qz;

  state->sample_count = 0;

  if (daemon_start_if_needed() != 0) {
    fprintf(stderr, ">>> warning: optimizer daemon not available at Init\n");
  } else {
    if (daemon_init_bounds(lower_bounds, upper_bounds) != 0)
      fprintf(stderr, ">>> warning: daemon INIT failed\n");
  }

  bayes_opt_status_struct st;
  fill_status(&st, state, "initialized");

  bayes_opt_status_struct *sposter = status->data(self);
  *sposter = st;
  status->write(self);

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

  if (daemon_ask(&s) != 0) {
    fprintf(stderr, ">>> daemon ASK failed\n");
    fflush(stderr);
    return bayes_opt_OPTIMIZATION_FAILED(self);
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

  bayes_opt_status_struct st;
  fill_status(&st, state, "new parameters proposed");

  bayes_opt_status_struct *sposter = status->data(self);
  *sposter = st;

  ev = status->write(self);
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

  if (!m->pos._present || !m->att._present) {
    fprintf(stderr, ">>> measure missing pos or att\n");
    fflush(stderr);
    return bayes_opt_NO_MEASUREMENT(self);
  }

  double x = m->pos._value.x;
  double y = m->pos._value.y;
  double z = m->pos._value.z;

  double qw = m->att._value.qw;
  double qx = m->att._value.qx;
  double qy = m->att._value.qy;
  double qz = m->att._value.qz;

  bool allow_update = true;

  ev = allow->read(self);
  if (ev == genom_ok) {
    bayes_opt_control *a = allow->data(self);
    allow_update = a->allow;
  }

  if (!allow_update) {
    fprintf(stderr, ">>> update skipped: allow=false\n");
    fflush(stderr);
    return bayes_opt_ether;
  }

  double dx = x - state->reference_x;
  double dy = y - state->reference_y;
  double dz = z - state->reference_z;

  double pos_error = sqrt(dx * dx + dy * dy + dz * dz);

  double dot =
    qw * state->reference_qw +
    qx * state->reference_qx +
    qy * state->reference_qy +
    qz * state->reference_qz;

  dot = fabs(dot);

  if (dot > 1.0)
    dot = 1.0;

  double att_error = 2.0 * acos(dot);

  double score = pos_error + 0.5 * att_error;

  state->current_score = score;
  state->sample_count++;

  if (daemon_tell(&state->current_params, score) != 0) {
    fprintf(stderr, ">>> warning: daemon TELL failed\n");
  }

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
          ">>> measure: x=%f y=%f z=%f qw=%f qx=%f qy=%f qz=%f\n",
          x, y, z, qw, qx, qy, qz);

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

  daemon_reset();

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
