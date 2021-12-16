#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct prpc_chan_st;
struct prpc_chan_st *prpc_connect_chan(const char *bus, const char *peer);
void prpc_connected_chan_init_call(struct prpc_chan_st *chan, const char *name, int args, int kwargs);

/*args*/
void prpc_chan_call_args_add_int64(struct prpc_chan_st *chan, int64_t arg);
void prpc_chan_call_args_add_double(struct prpc_chan_st *chan, double arg);
void prpc_chan_call_args_add_str(struct prpc_chan_st *chan, const void *arg, size_t len);
void prpc_chan_call_args_add_bin(struct prpc_chan_st *chan, const void *arg, size_t len);

/*kwargs*/
void prpc_chan_call_kwargs_add_int64(struct prpc_chan_st *chan, const char *key, int64_t val);
void prpc_chan_call_kwargs_add_double(struct prpc_chan_st *chan, const char *key, double val);
void prpc_chan_call_kwargs_add_str(struct prpc_chan_st *chan, const char *key, const void *val, size_t len);
void prpc_chan_call_kwargs_add_bin(struct prpc_chan_st *chan, const char *key, const void *val, size_t len);

/*sync exec call*/
#define PRPC_EXEC_DONE_WITH_ERR 1
int prpc_chan_call_exec(struct prpc_chan_st *chan);

#define PRPC_RET_ITH_ERR -1
#define PRPC_RET_TYPE_ERR -2
#define PRPC_RET_LEN_ERR -3

int prpc_chan_call_ret_ith_int64(struct prpc_chan_st *chan, unsigned i, int64_t *val);
int prpc_chan_call_ret_ith_double(struct prpc_chan_st *chan, unsigned i, double *val);
int prpc_chan_call_ret_ith_str(struct prpc_chan_st *chan, unsigned i, void *val, size_t *len);
int prpc_chan_call_ret_ith_bin(struct prpc_chan_st *chan, unsigned i, void *val, size_t *len);

void prpc_chan_fini_call(struct prpc_chan_st *chan);
void prpc_close_chan(struct prpc_chan_st *chan);
