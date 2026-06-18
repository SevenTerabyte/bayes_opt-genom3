/*
 * Copyright (c) 2012-2014,2018-2019,2023-2024 LAAS/CNRS
 * All rights reserved.
 *
 * Redistribution and use  in source  and binary  forms,  with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   1. Redistributions of  source  code must retain the  above copyright
 *      notice and this list of conditions.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice and  this list of  conditions in the  documentation and/or
 *      other materials provided with the distribution.
 *
 *                                      Anthony Mallet on Thu Mar 15 2012
 */
#ifndef H_CCLIENT_PRIVATE
#define H_CCLIENT_PRIVATE

#include <stddef.h>
#include <stdint.h>

#include "csLib.h"

#include "genom3/c/client.h"

struct pocolibs_options {
  size_t varmsg_maxsize;
  char *instance;
};

struct genom_context_data {
  pthread_key_t key;
};

struct genom_exdata {
  genom_event ex;
  void *exdetail;
  size_t exsize;
};

struct genom_client_s {
  struct genom_context_iface context;
  struct genom_context_data context_data;

  char *instance;
  CLIENT_ID csid;
  pthread_mutex_t cmutex;
  pthread_cond_t cevent;

  struct genom_client_request {
    const struct genom_service_info *info;
    int rid, aid;
    int status, csstatus;
    genom_event report;
    void *output;
    void *exdetail;
    void (*exfini)(void *);
    int	(*decode)(char *, int, char *, int);
    genom_request_cb sentcb, donecb;
    void *cb_data;
  } *active_rqst;
  int nactive, nextrqst;

  struct genom_client_ports *ports;

  int evpipe[2], evpending;
  genom_client next;
};

extern const struct genom_client_option genom_pocolibs_options[];

genom_client	pocolibs_newclient(void);
void		pocolibs_delclient(genom_client h);
genom_event	pocolibs_init(genom_client h, size_t mboxsize);
void		pocolibs_fini(void);
genom_event	pocolibs_getopt(genom_client h, int argc, char *argv[],
                        struct pocolibs_options *opts);
int		pocolibs_eventfd(genom_client h);

const struct genom_service_info *
		pocolibs_service_info(genom_client h, int rqstid);
genom_event	pocolibs_wait(genom_client h, int rqstid);
int		pocolibs_done(genom_client h, int rqstid);
genom_event	pocolibs_result(genom_client h, int rqstid,
                        genom_event *report, void **output, void **exdetail);
genom_event	pocolibs_clean(genom_client h, int rqstid);
genom_event	pocolibs_doevents(genom_client h);

genom_event	pocolibs_sendrqst(genom_client h, int rqst,
                        const struct genom_service_info *info, const void *in,
                        int (*encode)(char *, int, char *, int), int ack,
                        void (*outini)(void *),
                        int (*decode)(char *, int, char *, int), size_t outsz,
                        genom_request_cb sentcb, genom_request_cb donecb,
                        void *cb_data, int *rqstid);

genom_event	genom_pocolibs_client_raise(genom_event ex, const void *detail,
                        size_t size, genom_context self);
const void *	genom_pocolibs_client_raised(genom_event *ex,
                        genom_context self);

#endif /* H_CCLIENT_PRIVATE */
