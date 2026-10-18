/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */

#ifndef GIN_EVENT_QUEUE_H
#define GIN_EVENT_QUEUE_H
#include <stdint.h>
#define GIN_EVENT_CAPACITY 16u
typedef struct {uint32_t slot,revision;int32_t kind,value;uint32_t shift,sound_id,mode,page,side,name_epoch;} event_t;


static inline int gin_event_push(event_t *events,volatile uint32_t *head,
                                 uint32_t tail,event_t event){
 uint32_t h=*head;
 if(h!=tail&&event.kind==5){
  event_t *last=&events[(h-1u)&(GIN_EVENT_CAPACITY-1u)];
  if(last->kind==5&&last->slot==event.slot&&last->revision==event.revision){*last=event;return 0;}
 }
 if(h-tail>=GIN_EVENT_CAPACITY)return -1;
 events[h&(GIN_EVENT_CAPACITY-1u)]=event;*head=h+1u;return 0;
}
#endif
