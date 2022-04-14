#include <bbs.h>
#include <iostream>
#include <map>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define THREAD_POOL_PARAM_T dispatch_context_t
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <devctl.h>
#include <string.h>
#include <sys/neutrino.h>
#include <mutex>

std::mutex mut;
std::unique_lock<std::mutex> unique_mut(mut, std::defer_lock);

static resmgr_connect_funcs_t    connect_funcs;
static resmgr_io_funcs_t         io_funcs;
static iofunc_attr_t             attr;

struct Params
{
	std::uint32_t LastElement = 0;
	bbs::BBSParams *param;

};

std::map <std::int32_t, Params*> contexts;

#define count_bits sizeof(std::uint32_t) * CHAR_BIT // == 32

std::uint32_t getElement(std::uint32_t client_id){

	std::uint32_t CyphElement = 0,NewElement = 0;
	bool bit = false;
	std::uint32_t place= 1;
	//std:: cout << "client_id = \t" << client_id << "\ncontexts[client_id]->param->p = \t" << contexts[client_id]->param->p  << "\t" << contexts[client_id]->param << std::endl;
	unique_mut.lock();
	for(unsigned int i = 0; i < count_bits; i++)
	{
		NewElement = contexts[client_id]->LastElement * contexts[client_id]->LastElement % (contexts[client_id]->param->p * contexts[client_id]->param->q);
		bit = NewElement % 2;
		CyphElement +=  place * bit;
		contexts[client_id]->LastElement = NewElement;
		place = place * 2;
	}
	unique_mut.unlock();
	return CyphElement;
}

int io_open (resmgr_context_t * ctp , io_open_t * msg , RESMGR_HANDLE_T * handle , void * extra )
{
	unique_mut.lock();
	contexts[ctp->info.scoid] = new Params();
	contexts[ctp->info.scoid]->param = new bbs::BBSParams();
	unique_mut.unlock();
	std::cout << "CLIENT:\t" << ctp->info.scoid << "\tCONNECTED\n";
	return (iofunc_open_default (ctp, msg, handle, extra));
}

int io_close(resmgr_context_t *ctp, io_close_t *msg, iofunc_ocb_t *ocb)
{
	unique_mut.lock();
	std::map <std::int32_t, Params*> :: iterator _cell;
	_cell = contexts.find(ctp->info.scoid);
	if (contexts.count(ctp->info.scoid))
	{
		delete contexts[ctp->info.scoid]->param;
		delete contexts[ctp->info.scoid];
		contexts.erase(_cell);
		std::cout << "CLIENT:\t" << ctp->info.scoid << "\tCLOSE\n";
	}
	else
		std::cout << "CLIENT = \t" << ctp->info.scoid << " not found" << std::endl;
	unique_mut.unlock();
	return (iofunc_close_dup_default(ctp, msg, ocb));
}

int io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, iofunc_ocb_t *ocb) {
	int  status;
	if ((status = iofunc_devctl_default(ctp, msg, ocb)) != _RESMGR_DEFAULT)
		return (status);

	void *rx_data;
	int  nbytes = 0;

	std::int32_t client_id = ctp->info.scoid;
	rx_data = _DEVCTL_DATA(msg->i);

	switch (msg->i.dcmd)
	{
		case SET_GEN_PARAMS:
		{
			unique_mut.lock();
			std::cout << "SET_GEN_PARAMS:\tclient_id = \t" << client_id << std::endl;
			bbs::BBSParams* temp_param = reinterpret_cast<bbs::BBSParams*> (rx_data);
			contexts[client_id]->param->p = temp_param->p;
			contexts[client_id]->param->q = temp_param->q;
			contexts[client_id]->param->seed = temp_param->seed;
			contexts[client_id]->LastElement= contexts[client_id]->param->seed;
			unique_mut.unlock();
			break;
		}
		case GET_ELEMENT:
		{
			std:: cout << "GET_ELEMENT:\tclient_id = \t" << client_id << std::endl;
			*(std::uint32_t*)rx_data = getElement(client_id);
			nbytes = sizeof(std::uint32_t);
			break;
		}
		default:
			return EINVAL;
 };

	memset(&(msg->o), 0, sizeof(msg->o));
	msg->o.nbytes = nbytes;
	SETIOV(ctp->iov, &msg->o, sizeof(msg->o) + nbytes);

	return (_RESMGR_NPARTS(1));

}



int main(int argc, char **argv)
{
    /* declare variables we'll be using */
	thread_pool_attr_t   pool_attr;
	resmgr_attr_t        resmgr_attr;
	dispatch_t           *dpp;
	thread_pool_t        *tpp;
	dispatch_context_t   *ctp;
	int                  id;

    /* initialize dispatch interface */
    if((dpp = dispatch_create()) == NULL)
    {
        fprintf(stderr,
                "%s: Unable to allocate dispatch handle.\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    /* initialize resource manager attributes */
	memset(&resmgr_attr, 0, sizeof resmgr_attr);
	resmgr_attr.nparts_max = 1;
	resmgr_attr.msg_max_size = 2048;

    /* initialize functions for handling messages */
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                     _RESMGR_IO_NFUNCS, &io_funcs);
    io_funcs.devctl = io_devctl;
	connect_funcs.open 	= io_open;
	io_funcs.close_dup = io_close;
    /* initialize attribute structure used by the device */
    iofunc_attr_init(&attr, S_IFNAM | 0666, 0, 0);

    /* attach our device name */
    id = resmgr_attach(
            dpp,            /* dispatch handle        */
            &resmgr_attr,   /* resource manager attrs */
            "/dev/cryptobbs",  /* device name            */
            _FTYPE_ANY,     /* open type              */
            0,              /* flags                  */
            &connect_funcs, /* connect routines       */
            &io_funcs,      /* I/O routines           */
            &attr);         /* handle                 */
    if(id == -1) {
        fprintf(stderr, "%s: Unable to attach name.\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* инициализация атрибутов пула потоков */
	memset(&pool_attr, 0, sizeof pool_attr);
	pool_attr.handle = dpp;
	pool_attr.context_alloc = dispatch_context_alloc;
	pool_attr.block_func = dispatch_block;
	pool_attr.unblock_func = dispatch_unblock;
	pool_attr.handler_func = dispatch_handler;
	pool_attr.context_free = dispatch_context_free;
	pool_attr.lo_water = 2;
	pool_attr.hi_water = 4;
	pool_attr.increment = 1;
	pool_attr.maximum = 50;

	/* инициализация пула потоков */
	if((tpp = thread_pool_create(&pool_attr,
								 POOL_FLAG_EXIT_SELF)) == NULL) {
		fprintf(stderr, "%s: Unable to initialize thread pool.\n",
				argv[0]);
		return EXIT_FAILURE;
	}

	/* запустить потоки, блокирующая функция */
	thread_pool_start(tpp);

	std::cout << "ok";
    return EXIT_SUCCESS; // never go here
}
