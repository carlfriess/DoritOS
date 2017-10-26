#ifndef _INIT_LMP_HANDLERS_H_
#define _INIT_LMP_HANDLERS_H_

#include <aos/aos.h>

enum lmp_messagetype;

void lmp_handler_dispatcher(void *);
void lmp_handler_register(struct lmp_chan *, struct capref *);
void lmp_handler_memory(struct lmp_chan *, struct capref *, size_t, size_t);
void lmp_handler_spawn(struct lmp_chan *, struct capref *);
void lmp_handler_terminal(struct lmp_chan *, struct capref *);

#endif