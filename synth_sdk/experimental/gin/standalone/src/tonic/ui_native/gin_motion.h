/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_MOTION_H
#define GIN_MOTION_H


#include <stdint.h>
typedef struct {
 uint32_t started,id,morph,last_tick,hit,start,until,leader,live,flags,pulse_start,pulse_seen;
 int32_t progress[2],ripple,shake,interaction_ripple,pulse;
} gin_motion_t;


static uint32_t gin_motion_alpha(uint32_t dt){
 if(dt>=1000)return 32768;
 uint32_t remaining=32768;while(dt>50){remaining=(remaining*(32768u-motion_alpha[50]))>>15;dt-=50;}
 return 32768u-((remaining*(32768u-motion_alpha[dt]))>>15);
}

static void gin_motion_rebase(gin_motion_t *s,uint32_t now,uint32_t hit){
 s->last_tick=now;s->hit=hit;s->live=s->flags=s->pulse_seen=0;
 s->ripple=s->shake=s->interaction_ripple=s->pulse=0;
}
static void gin_motion_interaction(gin_motion_t *s){s->interaction_ripple=s->ripple=32768;}
static void gin_motion_step(gin_motion_t *s,uint32_t id,uint32_t morph,uint32_t side,uint32_t now,uint32_t hit,uint32_t decay){
 uint32_t dt=now-s->last_tick,i;int changed=0;
 if(!s->started){s->started=1;s->id=id;s->morph=morph;s->hit=hit;dt=0;}
 if(s->id!=id){gin_motion_rebase(s,now,hit);s->id=id;s->morph=morph;}
 if(s->morph!=morph){
  changed=1;s->morph=morph;
  if(!s->live||(int32_t)(now-s->until)>=190){s->start=now;s->leader=side;}
  s->until=now+950;s->live=1;
 }
 uint32_t alpha=gin_motion_alpha(dt);s->last_tick=now;s->flags=0;
 for(i=0;i<2;i++){
  uint32_t delay=i==s->leader?0:180,withdraw=i==s->leader?0:190;
  int on=s->live&&(uint32_t)(now-s->start)>=delay&&(int32_t)(now-s->until)<(int32_t)withdraw;
  int32_t target=on&&(i?morph>0:morph<1000)?32768:0;
  if(on)s->flags|=1u<<i;
  s->progress[i]+=(int32_t)(((int64_t)(target-s->progress[i])*alpha)>>15);
  if(target-s->progress[i]<16&&target-s->progress[i]>-16)s->progress[i]=target;
 }
 if(s->live&&(int32_t)(now-s->until)>=190)s->live=0;


 uint32_t age=now-s->pulse_start;
 if(hit!=s->hit&&(!s->pulse_seen||age>=240u)){s->pulse_start=now;s->pulse_seen=1;age=0;}
 s->pulse=s->pulse_seen&&age<180u?(int32_t)(8192u*(180u-age)/180u):0;
 s->shake=0;
 if(changed||s->flags)s->interaction_ripple=32768;
 else{s->interaction_ripple-=(int32_t)(((uint64_t)s->interaction_ripple*dt)/((uint64_t)decay+dt+(decay==0&&dt==0)));if(s->interaction_ripple<262)s->interaction_ripple=0;}
 s->ripple=s->interaction_ripple>s->pulse?s->interaction_ripple:s->pulse;
 s->hit=hit;
}
#endif
