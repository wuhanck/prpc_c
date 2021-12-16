#include "prpc_c.h"
#include "msgpack.h"

int main(void)
{
	struct prpc_chan_st *chan = prpc_connect_chan("test", "test-services");
	int64_t iret = 0;
	double dret = 0.0;

	prpc_connected_chan_init_call(chan, "test1", 2, 0);
	prpc_chan_call_args_add_int64(chan, 3);
	prpc_chan_call_args_add_int64(chan, 4);
	prpc_chan_call_exec(chan);
	iret = -1;
	prpc_chan_call_ret_ith_int64(chan, 0, &iret);
	printf("%ld\n", iret);
	prpc_chan_fini_call(chan);

	prpc_connected_chan_init_call(chan, "test1", 2, 0);
	prpc_chan_call_args_add_double(chan, 7);
	prpc_chan_call_args_add_int64(chan, 8);
	prpc_chan_call_exec(chan);
	iret = -1;
	prpc_chan_call_ret_ith_int64(chan, 0, &iret);
	printf("%ld\n", iret);
	dret = -1.0;
	prpc_chan_call_ret_ith_double(chan, 0, &dret);
	printf("%f\n", dret);
	prpc_chan_fini_call(chan);

	prpc_connected_chan_init_call(chan, "test1", 2, 0);
	prpc_chan_call_args_add_str(chan, "hello", strlen("hello"));
	prpc_chan_call_args_add_str(chan, "world", strlen("world"));
	prpc_chan_call_exec(chan);
	char bf[64];
	size_t bs = sizeof(bf);
	prpc_chan_call_ret_ith_str(chan, 0, bf, &bs);
	bf[bs] = 0;
	printf("%s\n", bf);
	prpc_chan_fini_call(chan);

	prpc_connected_chan_init_call(chan, "test1", 2, 0);
	prpc_chan_call_args_add_bin(chan, "t", 1);
	prpc_chan_call_args_add_str(chan, "test", strlen("test"));
	int exec_ret;
	exec_ret = prpc_chan_call_exec(chan);
	printf("%s\n", exec_ret == PRPC_EXEC_DONE_WITH_ERR ? "true" : "false");
	char ef[64];
	size_t es = sizeof(ef);
	prpc_chan_call_ret_ith_str(chan, 0, ef, &es);
	printf("%s\n", ef);
	prpc_chan_fini_call(chan);

	prpc_close_chan(chan);
	return 0;
}
