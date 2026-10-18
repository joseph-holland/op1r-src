/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_NAME_H
#define GIN_NAME_H
#include <stdint.h>
#define GIN_NAME_BYTES 24u



enum {GIN_NAME_SOUND,GIN_NAME_KIT};
enum {GIN_NAME_DONE,GIN_NAME_CANCEL,GIN_NAME_SPACE,GIN_NAME_DELETE};
enum {GIN_NAME_OK,GIN_NAME_INVALID,GIN_NAME_FULL,GIN_NAME_STALE,GIN_NAME_APPLY,GIN_NAME_CANCELLED};

typedef struct {char text[GIN_NAME_BYTES];uint32_t active,target,slot,revision,id,cursor,bank,action;} gin_name_t;
extern const char gin_name_characters[];
extern const char *const gin_name_actions[4];
int gin_name_begin(gin_name_t *,uint32_t target,uint32_t slot,uint32_t revision,uint32_t id,const char *);
int gin_name_turn(gin_name_t *,unsigned encoder,int detents);



int gin_name_execute(gin_name_t *,uint32_t slot,uint32_t revision,char out[GIN_NAME_BYTES]);
void gin_name_cancel(gin_name_t *);
#endif
