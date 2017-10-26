#ifndef _AOS_LMP_H_
#define _AOS_LMP_H_

#include <aos/aos.h>

enum lmp_request_type;
enum lmp_response_type;

// Server side
void lmp_server_dispatcher(void *);
void lmp_server_register(struct lmp_chan *, struct capref);
void lmp_server_memory(struct lmp_chan *, struct capref, size_t, size_t);
void lmp_server_spawn(struct lmp_chan *, struct capref);
void lmp_server_terminal(struct lmp_chan *, struct capref);

// Client side
void lmp_client_recv(void *);
void lmp_client_wait(void *);
//void lmp_client_dispatcher(void *);
//void lmp_client_register(struct lmp_chan *, struct capref);
//void lmp_client_memory(struct lmp_chan *, struct capref, size_t, size_t);
//void lmp_client_spawn(struct lmp_chan *, struct capref);
//void lmp_client_terminal(struct lmp_chan *, struct capref);void lmp_handler_dispatcher(void *);

#endif