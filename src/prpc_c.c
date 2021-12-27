#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "msgpack.h"

#include "prpc_c.h"

#define MAX_SELF_NAME 32
#define MAX_REPLY 256

struct prpc_chan_st {
	uint64_t tag_;
	msgpack_sbuffer* buf_;
	msgpack_packer* pk_;
	msgpack_unpacked msg_;
	msgpack_object *pret_;
	int ret_cnt_;
	int s_;
	int args_;
	int ia_;
	int kwargs_;
	int ikwa_;
	int self_len_;
	char self_[MAX_SELF_NAME];
	char reply_[MAX_REPLY];
};

const uint64_t _MAX_SERV_ID = (((uint64_t)0x1) << 48);
const int _REQT_CALL = 0;
const int _REQT_RET_DONE = 1;
const int _REQT_RET_ERR = 2;
const int _REQT_NOTIFY = -1;

static void gen_tag(struct prpc_chan_st *chan) {
	chan->tag_ += 1;
	if (chan->tag_ == _MAX_SERV_ID)
		chan->tag_ = 0;
}

static void gen_self_name(struct prpc_chan_st *chan) {
	chan->self_len_ = snprintf(chan->self_, MAX_SELF_NAME, "%d-%p", getpid(), (void *)chan);
	assert(chan->self_len_ >= 0);
	assert(chan->self_len_ < MAX_SELF_NAME);
}

struct prpc_chan_st *prpc_connect_chan(const char *bus, const char *peer)
{
	const char *pre = "local";
	const unsigned int pre_len = strlen(pre);
	const unsigned int bus_len = strlen(bus);
	const unsigned int peer_len = strlen(peer);
	const unsigned int sn_len = 1+pre_len+1+bus_len+1+peer_len;
	struct sockaddr_un addr;
	const unsigned int addr_len = sizeof(addr.sun_family)+sn_len;
	struct prpc_chan_st *chan;
	int ret;

	chan = malloc(sizeof(*chan));
	if (!chan)
		return 0;

	if (addr_len > sizeof(addr))
		goto err_addr_len;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	memcpy(&addr.sun_path[1], pre, pre_len);
	memcpy(&addr.sun_path[1+pre_len+1], bus, bus_len);
	memcpy(&addr.sun_path[1+pre_len+1+bus_len+1], peer, peer_len);

	chan->s_ = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (chan->s_ == -1)
		goto err_sock_create;

	ret = connect(chan->s_, (const struct sockaddr *) &addr, addr_len);
	if (ret == -1)
		goto err_sock_connect;

	gen_self_name(chan);
	ret = write(chan->s_, chan->self_, chan->self_len_);
	if (ret != chan->self_len_)
		goto err_sock_write;

	chan->tag_ = 0;
	chan->buf_ = msgpack_sbuffer_new();
	chan->pk_ = msgpack_packer_new(chan->buf_, msgpack_sbuffer_write);
	chan->args_ = 0;
	chan->ia_ = 0;
	chan->kwargs_ = 0;
	chan->ikwa_ = 0;
	return chan;

err_sock_write:
err_sock_connect:
	close(chan->s_);
err_sock_create:
err_addr_len:
	free(chan);
	return 0;
}

void prpc_connected_chan_init_call(struct prpc_chan_st *chan, const char *name, int args, int kwargs)
{
	assert(args >= 0);
	assert(kwargs >= 0);
	chan->args_ = args;
	chan->ia_ = 0;
	chan->kwargs_ = kwargs;
	chan->ikwa_ = 0;
	chan->ret_cnt_ = 0;
	msgpack_unpacked_init(&chan->msg_);

	gen_tag(chan);
	msgpack_pack_array(chan->pk_, 5);
	msgpack_pack_int(chan->pk_, _REQT_CALL);
	msgpack_pack_uint64(chan->pk_, chan->tag_);
	msgpack_pack_str(chan->pk_, strlen(name));
	msgpack_pack_str_body(chan->pk_, name, strlen(name));
	msgpack_pack_array(chan->pk_, args);
}

void prpc_chan_call_args_add_int64(struct prpc_chan_st *chan, int64_t arg)
{
	assert(chan->args_ != chan->ia_);
	++(chan->ia_);
	msgpack_pack_int64(chan->pk_, arg);
}

void prpc_chan_call_args_add_double(struct prpc_chan_st *chan, double arg)
{
	assert(chan->args_ != chan->ia_);
	++(chan->ia_);
	msgpack_pack_double(chan->pk_, arg);
}

void prpc_chan_call_args_add_str(struct prpc_chan_st *chan, const void *arg, size_t len)
{
	assert(chan->args_ != chan->ia_);
	++(chan->ia_);
	msgpack_pack_str(chan->pk_, len);
	msgpack_pack_str_body(chan->pk_, arg, len);
}

void prpc_chan_call_args_add_bin(struct prpc_chan_st *chan, const void *arg, size_t len)
{
	assert(chan->args_ != chan->ia_);
	++(chan->ia_);
	msgpack_pack_bin(chan->pk_, len);
	msgpack_pack_bin_body(chan->pk_, arg, len);
}

void prpc_chan_call_kwargs_add_int64(struct prpc_chan_st *chan, const char *key, int64_t val)
{
	assert(chan->args_ == chan->ia_);
	if (chan->ikwa_ == 0)
		msgpack_pack_map(chan->pk_, chan->kwargs_);

	assert(chan->kwargs_ != chan->ikwa_);
	++(chan->ikwa_);
	msgpack_pack_str(chan->pk_, strlen(key));
	msgpack_pack_str_body(chan->pk_, key, strlen(key));
	msgpack_pack_int64(chan->pk_, val);
}

void prpc_chan_call_kwargs_add_double(struct prpc_chan_st *chan, const char *key, double val)
{
	assert(chan->args_ == chan->ia_);
	if (chan->ikwa_ == 0)
		msgpack_pack_map(chan->pk_, chan->kwargs_);

	assert(chan->kwargs_ != chan->ikwa_);
	++(chan->ikwa_);
	msgpack_pack_str(chan->pk_, strlen(key));
	msgpack_pack_str_body(chan->pk_, key, strlen(key));
	msgpack_pack_double(chan->pk_, val);
}

void prpc_chan_call_kwargs_add_str(struct prpc_chan_st *chan, const char *key, const void *val, size_t len)
{
	assert(chan->args_ == chan->ia_);
	if (chan->ikwa_ == 0)
		msgpack_pack_map(chan->pk_, chan->kwargs_);

	assert(chan->kwargs_ != chan->ikwa_);
	++(chan->ikwa_);
	msgpack_pack_str(chan->pk_, strlen(key));
	msgpack_pack_str_body(chan->pk_, key, strlen(key));
	msgpack_pack_str(chan->pk_, len);
	msgpack_pack_str_body(chan->pk_, val, len);
}

void prpc_chan_call_kwargs_add_bin(struct prpc_chan_st *chan, const char *key, const void *val, size_t len)
{
	assert(chan->args_ == chan->ia_);
	if (chan->ikwa_ == 0)
		msgpack_pack_map(chan->pk_, chan->kwargs_);

	assert(chan->kwargs_ != chan->ikwa_);
	++(chan->ikwa_);
	msgpack_pack_str(chan->pk_, strlen(key));
	msgpack_pack_str_body(chan->pk_, key, strlen(key));
	msgpack_pack_bin(chan->pk_, len);
	msgpack_pack_bin_body(chan->pk_, val, len);
}

int prpc_chan_call_exec(struct prpc_chan_st *chan)
{
	int ret;
	assert(chan->args_ == chan->ia_);
	assert(chan->kwargs_ == chan->ikwa_);
	if (chan->kwargs_ == 0)
		msgpack_pack_map(chan->pk_, 0);

	ret = write(chan->s_, chan->buf_->data, chan->buf_->size);
	if (ret != chan->buf_->size)
		goto err_sock_write;

	ret = read(chan->s_, chan->reply_, MAX_REPLY);
	if (ret <= 0)
		goto err_sock_read;

	msgpack_unpack_return unpack_ret = msgpack_unpack_next(&chan->msg_, chan->reply_, ret, NULL);
	if (unpack_ret != MSGPACK_UNPACK_SUCCESS)
		goto err_unpack;

	msgpack_object obj = chan->msg_.data;
	if (obj.type != MSGPACK_OBJECT_ARRAY
	    || obj.via.array.size != 3)
		goto err_ret_type;
	msgpack_object *p = obj.via.array.ptr;
	if (p->type != MSGPACK_OBJECT_POSITIVE_INTEGER)
		goto err_ret_oret_type;
	if (p->via.u64 != 1 && p->via.u64 != 2)
		goto err_ret_oret_val;
	ret = 0;
	if (p->via.u64 == 2)
		ret = PRPC_EXEC_DONE_WITH_ERR;
	/*we ignore tag*/
	msgpack_object *pret = obj.via.array.ptr + 2;
	if (pret->type == MSGPACK_OBJECT_NIL) {
		chan->ret_cnt_ = 0;
	} else if (pret->type == MSGPACK_OBJECT_ARRAY) {
		chan->pret_ = pret->via.array.ptr;
		chan->ret_cnt_ = pret->via.array.size;
	} else {
		chan->pret_ = pret;
		chan->ret_cnt_ = 1;
	}
	return ret;

err_ret_oret_val:
err_ret_oret_type:
err_ret_type:
err_unpack:
err_sock_read:
err_sock_write:
	return -1;
}

int prpc_chan_call_ret_ith_int64(struct prpc_chan_st *chan, unsigned i, int64_t *val)
{
	if (i >= chan->ret_cnt_)
		return PRPC_RET_ITH_ERR;
	msgpack_object *pret = chan->pret_ + i;
	if (pret->type == MSGPACK_OBJECT_NEGATIVE_INTEGER) {
		*val = pret->via.i64;
	} else if (pret->type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
		*val = pret->via.u64;
	} else
		return PRPC_RET_TYPE_ERR;
	return 0;
}

int prpc_chan_call_ret_ith_double(struct prpc_chan_st *chan, unsigned i, double *val)
{
	if (i >= chan->ret_cnt_)
		return PRPC_RET_ITH_ERR;
	msgpack_object *pret = chan->pret_ + i;
	if (pret->type == MSGPACK_OBJECT_FLOAT) {
		*val = pret->via.f64;
	} else if (pret->type == MSGPACK_OBJECT_FLOAT32) {
		*val = pret->via.f64;
	} else if (pret->type == MSGPACK_OBJECT_FLOAT64) {
		*val = pret->via.f64;
	} else
		return PRPC_RET_TYPE_ERR;
	return 0;
}

int prpc_chan_call_ret_ith_str(struct prpc_chan_st *chan, unsigned i, void *val, size_t *len)
{
	if (i >= chan->ret_cnt_)
		return PRPC_RET_ITH_ERR;
	msgpack_object *pret = chan->pret_ + i;
	if (pret->type != MSGPACK_OBJECT_STR)
		return PRPC_RET_TYPE_ERR;
	if (pret->via.str.size > *len) {
		*len = pret->via.str.size;
		return PRPC_RET_LEN_ERR;
	}
	*len = pret->via.str.size;
	memcpy(val, pret->via.str.ptr, pret->via.str.size);
	return 0;
}

int prpc_chan_call_ret_ith_bin(struct prpc_chan_st *chan, unsigned i, void *val, size_t *len)
{
	if (i >= chan->ret_cnt_)
		return PRPC_RET_ITH_ERR;
	msgpack_object *pret = chan->pret_ + i;
	if (pret->type != MSGPACK_OBJECT_BIN)
		return PRPC_RET_TYPE_ERR;
	if (pret->via.bin.size > *len) {
		*len = pret->via.bin.size;
		return PRPC_RET_LEN_ERR;
	}
	*len = pret->via.bin.size;
	memcpy(val, pret->via.bin.ptr, pret->via.bin.size);
	return 0;
}

void prpc_chan_fini_call(struct prpc_chan_st *chan)
{
	msgpack_sbuffer_clear(chan->buf_);
	msgpack_unpacked_destroy(&chan->msg_);
}

void prpc_close_chan(struct prpc_chan_st *chan)
{
	close(chan->s_);
	msgpack_sbuffer_free(chan->buf_);
	msgpack_packer_free(chan->pk_);
	free(chan);
}
