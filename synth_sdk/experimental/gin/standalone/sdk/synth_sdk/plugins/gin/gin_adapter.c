/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#include "gin_adapter.h"
#define RUNTIME_COOKIE 0x47445231u
#define RETRIGGER_FADE_SAMPLES 64u
static gin_sdk_runtime_t *current;
static void reset_reconstruction(gin_sdk_binding_t *b){
#if GIN_RATE_2X
 gin_resample_2x_reset(&b->reconstruction);b->resample_left=0;
#else
 (void)b;
#endif
}
static int residual_reconstruction(const gin_sdk_binding_t *b){
#if GIN_RATE_2X
 return b->resample_left!=0;
#else
 (void)b;return 0;
#endif
}



#if defined(__GNUC__)
typedef uint32_t gin_copy_word_t __attribute__((__may_alias__));
#endif
static void copy_bytes(void *d,const void *s,uint32_t n){
 volatile unsigned char *a=d;const volatile unsigned char *b=s;
#if defined(__GNUC__)
 if(!(((uintptr_t)d|(uintptr_t)s)&3u)){
  volatile gin_copy_word_t *wa=d;const volatile gin_copy_word_t *wb=s;
  while(n>=4u){*wa++=*wb++;n-=4u;}a=(volatile unsigned char *)wa;b=(const volatile unsigned char *)wb;
 }
#endif
 while(n--)*a++=*b++;
}
static void zero_bytes(void *d,uint32_t n){
 volatile unsigned char *a=d;
#if defined(__GNUC__)
 if(!((uintptr_t)d&3u)){
  volatile gin_copy_word_t *wa=d;while(n>=4u){*wa++=0;n-=4u;}a=(volatile unsigned char *)wa;
 }
#endif
 while(n--)*a++=0;
}
static uint32_t word(const void *p){const unsigned char *b=p;return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
static int unit_word(uint32_t w){return w<=0x3f800000u||w==0x80000000u;}
static int valid_runtime(const gin_sdk_runtime_t *r){return r&&r->cookie==RUNTIME_COOKIE;}
static int sound_index(const gin_sdk_slot_t *s,uint32_t id){uint32_t i;for(i=0;i<s->kit.count;++i)if(s->kit.sounds[i].id==id)return (int)i;return -1;}
static gin_sdk_binding_t *binding(const void *receiver){uint32_t i;if(!valid_runtime(current)||!receiver)return 0;for(i=0;i<12u;++i){gin_sdk_binding_t *b=&current->binding[i];if(b->receiver==receiver&&word(receiver)==b->vptr_word)return b;}return 0;}
static uint32_t revision(gin_sdk_runtime_t *r){if(r->serial==UINT32_MAX)return 0;return ++r->serial;}
void gin_sdk_init(gin_sdk_runtime_t *r){if(r){zero_bytes(r,(uint32_t)sizeof(*r));r->cookie=RUNTIME_COOKIE;r->core_slot=r->selected_slot=UINT32_MAX;}}
uint32_t gin_sdk_activate(gin_sdk_runtime_t *r){if(!valid_runtime(r))return OP1_DRUM_IDENTITY;current=r;return OP1_DRUM_OK;}
void gin_sdk_retire_voice(gin_sdk_binding_t *b){if(b){gin_integer_release(&b->owned);b->last_sample=b->tail_sample=0;b->tail_left=0;reset_reconstruction(b);}}
void gin_sdk_deactivate(gin_sdk_runtime_t *r){uint32_t i;if(!valid_runtime(r))return;for(i=0;i<12u;++i)gin_sdk_retire_voice(&r->binding[i]);if(current==r)current=0;}
uint32_t gin_sdk_bind(gin_sdk_runtime_t *r,uint32_t v,const void *receiver,uint32_t bytes,uint32_t vptr,const op1_drum_engine_t *original,const int32_t *adjust){uint32_t i,g;gin_sdk_binding_t *b;
 if(!valid_runtime(r)||v>=12u||!receiver||(bytes!=20u&&bytes!=72u)||!original||!adjust||!original->trigger||!original->render||!original->status||!original->release||word(receiver)!=vptr)return OP1_DRUM_IDENTITY;
 for(i=0;i<12u;++i)if(i!=v&&r->binding[i].receiver==receiver)return OP1_DRUM_IDENTITY;
 g=revision(r);if(!g)return OP1_DRUM_STALE;b=&r->binding[v];gin_sdk_retire_voice(b);zero_bytes(b,(uint32_t)sizeof(*b));b->receiver=receiver;b->record_bytes=bytes;b->vptr_word=vptr;b->voice=v;b->generation=g;copy_bytes(&b->original,original,sizeof(*original));copy_bytes(b->original_adjust,adjust,4u*sizeof(int32_t));return OP1_DRUM_OK;}
uint32_t gin_sdk_select_engine(gin_sdk_runtime_t *r,uint32_t v,const char *name){gin_sdk_binding_t *b;const op1_drum_engine_t *d;uint32_t g;if(!valid_runtime(r)||v>=12u||!r->binding[v].receiver)return OP1_DRUM_IDENTITY;d=name?op1_drum_find(name):0;if(name&&!d)return OP1_DRUM_SELECTION;g=revision(r);if(!g)return OP1_DRUM_STALE;b=&r->binding[v];gin_sdk_retire_voice(b);b->selected=d;b->generation=g;b->view_generation=0;b->block_age=0;return OP1_DRUM_OK;}
uint32_t gin_sdk_selection(gin_sdk_runtime_t *r,uint32_t core,uint32_t selected){if(!valid_runtime(r)||core>=8u||selected>=8u)return OP1_DRUM_RANGE;r->core_slot=core;r->selected_slot=selected;return core==selected?OP1_DRUM_OK:OP1_DRUM_SELECTION;}
uint32_t gin_sdk_load_kit(gin_sdk_runtime_t *r,uint32_t slot,const int16_t *base,const gin_preset_owned_t *kit,gin_slot_ticket_t *out){uint32_t i,j,k,g;gin_sdk_slot_t *s;
 if(!valid_runtime(r)||slot>=8u||!base||!kit||!out||!kit->count||kit->count>GIN_PRESET_MAX_SOUNDS)return OP1_DRUM_RANGE;
 for(i=0;i<kit->count;++i){const gin_preset_sound_t *a=&kit->sounds[i];if(!a->id||a->morph>1000u||a->edit_side>1u||!a->name[0])return OP1_DRUM_SNAPSHOT;
  for(j=0;j<i;++j)if(kit->sounds[j].id==a->id)return OP1_DRUM_SNAPSHOT;
  for(j=0;j<24u&&a->name[j];++j)if((unsigned char)a->name[j]<32u||(unsigned char)a->name[j]>126u)return OP1_DRUM_SNAPSHOT;
  if(j==24u)return OP1_DRUM_SNAPSHOT;
  for(j=0;j<2u;++j)for(k=0;k<21u;++k)if(!unit_word(word(&a->endpoint[j][k])))return OP1_DRUM_SNAPSHOT;}
 for(i=0;i<24u;++i)if(kit->key_map[i]){for(j=0;j<kit->count;++j)if(kit->sounds[j].id==kit->key_map[i])break;if(j==kit->count)return OP1_DRUM_SNAPSHOT;}
 g=revision(r);if(!g)return OP1_DRUM_STALE;s=&r->slot[slot];zero_bytes(s,sizeof(*s));s->revision=g;s->canonical_base=base;copy_bytes(&s->kit,kit,sizeof(*kit));copy_bytes(s->base_words,base,sizeof(s->base_words));out->slot=slot;out->revision=g;return OP1_DRUM_OK;}
uint32_t gin_sdk_publish_prepared(gin_sdk_runtime_t *r,gin_slot_ticket_t t,uint32_t id,const uint32_t *patch,const gin_prepared_t *p,uint32_t velocity){int index;uint32_t i;gin_sdk_slot_t *s;if(!valid_runtime(r)||t.slot>=8u||!patch||!p||velocity>127u)return OP1_DRUM_SNAPSHOT;s=&r->slot[t.slot];if(!t.revision||s->revision!=t.revision)return OP1_DRUM_STALE;index=sound_index(s,id);if(index<0||p->magic!=GIN_INTEGER_MAGIC||!p->total||p->total>441000u)return OP1_DRUM_SNAPSHOT;for(i=0;i<21u;++i)if(!unit_word(patch[i]))return OP1_DRUM_SNAPSHOT;copy_bytes(&s->prepared[index],p,sizeof(*p));copy_bytes(s->patch_words[index],patch,21u*sizeof(uint32_t));s->velocity[index]=velocity;s->prepared_revision[index]=t.revision;return OP1_DRUM_OK;}
uint32_t gin_sdk_publish_hit(gin_sdk_runtime_t *r,uint32_t v,gin_slot_ticket_t t,uint32_t key,uint32_t velocity){
 gin_sdk_binding_t *b;gin_sdk_slot_t *s;int index;
 if(!valid_runtime(r)||v>=12u||key>=24u||t.slot>=8u||velocity>127u)return OP1_DRUM_RANGE;
 b=&r->binding[v];s=&r->slot[t.slot];
 if(!b->receiver||b->selected!=&DRUM_ENGINE_gin)return OP1_DRUM_SELECTION;
 if(r->core_slot!=t.slot||r->selected_slot!=t.slot)return OP1_DRUM_SELECTION;
 if(!t.revision||s->revision!=t.revision)return OP1_DRUM_STALE;
 index=sound_index(s,s->kit.key_map[key]);
 if(index<0||s->prepared_revision[index]!=t.revision||s->velocity[index]!=velocity)return OP1_DRUM_SNAPSHOT;
 copy_bytes(&b->view_prepared,&s->prepared[index],sizeof(b->view_prepared));
 copy_bytes(b->view_patch_words,s->patch_words[index],sizeof(b->view_patch_words));
 b->view_sound_id=s->kit.sounds[index].id;
 b->view_slot=t.slot;b->view_revision=t.revision;b->key=key;b->velocity=velocity;b->view_generation=b->generation;
 return OP1_DRUM_OK;
}
static int coordinate(const gin_sdk_slot_t *s,uint32_t key,const int16_t *control){
 uintptr_t delta;
 if(!control||key>=24u||(uintptr_t)control<(uintptr_t)s->canonical_base)return 0;
 delta=(uintptr_t)control-(uintptr_t)s->canonical_base;
 return delta<384u&&!(delta&15u)&&(delta>>4)==key;
}
static int canonical_unchanged(const gin_sdk_slot_t *s,uint32_t key){
 const int16_t *p=s->canonical_base+8u*key;
 for(unsigned i=0;i<8u;i++)if(p[i]!=s->base_words[key][i])return 0;
 return 1;
}
static int resolve_hit(gin_sdk_binding_t *b,const int16_t *control){
 gin_sdk_slot_t *s;
 if(!b||b->view_generation!=b->generation||b->view_slot>=8u||current->core_slot!=b->view_slot||current->selected_slot!=b->view_slot)return 0;
 s=&current->slot[b->view_slot];
 return s->revision==b->view_revision&&coordinate(s,b->key,control)&&canonical_unchanged(s,b->key)&&s->kit.key_map[b->key]==b->view_sound_id;
}
static int current_hit(const gin_sdk_binding_t *b){
 return valid_runtime(current)&&b&&b->hit_generation==b->generation&&b->hit_slot<8u&&b->hit_key<24u&&b->hit_slot==current->core_slot&&b->hit_slot==current->selected_slot&&b->hit_revision==current->slot[b->hit_slot].revision;
}
static void choke(gin_sdk_binding_t *b){
 gin_integer_release(&b->owned);reset_reconstruction(b);b->tail_sample=b->last_sample;
 b->tail_left=b->last_sample?RETRIGGER_FADE_SAMPLES:0;
}
static void limit_synthesis(gin_sdk_binding_t *newest){
 uint32_t i,active=0,pass;
 for(i=0;i<12u;++i){gin_sdk_binding_t *b=&current->binding[i];
  if(b->selected==&DRUM_ENGINE_gin&&current_hit(b)&&b->owned.active)++active;
 }



 for(pass=0;pass<12u&&active>GIN_SYNTH_VOICE_LIMIT;++pass){
  gin_sdk_binding_t *oldest=0;
  for(i=0;i<12u;++i){gin_sdk_binding_t *b=&current->binding[i];
   if(b!=newest&&b->selected==&DRUM_ENGINE_gin&&current_hit(b)&&b->owned.active&&
      (!oldest||b->block_age>oldest->block_age))oldest=b;
  }
  if(!oldest)break;
  choke(oldest);--active;
 }
}
uint32_t gin_sdk_trigger_bound(gin_sdk_binding_t *b,const int16_t *control){
 uint32_t i;int32_t previous;
 if(!valid_runtime(current)||!resolve_hit(b,control))return OP1_DRUM_IDENTITY;
 previous=current_hit(b)?b->last_sample:0;
 if(gin_integer_trigger(&b->owned,&b->view_prepared))return OP1_DRUM_SNAPSHOT;
 b->hit_slot=b->view_slot;b->hit_key=b->key;b->hit_revision=b->view_revision;b->hit_generation=b->generation;
 b->hit_sound_id=b->view_sound_id;b->hit_velocity=b->velocity;
 copy_bytes(b->hit_patch_words,b->view_patch_words,sizeof(b->hit_patch_words));
 reset_reconstruction(b);
 b->tail_sample=previous;b->tail_left=previous?RETRIGGER_FADE_SAMPLES:0;


 for(i=0;i<12u;++i){gin_sdk_binding_t *old=&current->binding[i];
  if(old!=b&&old->selected==&DRUM_ENGINE_gin&&current_hit(old)&&(old->owned.active||residual_reconstruction(old))&&old->hit_key==b->hit_key){
   choke(old);
  }
 }
 b->block_age=0;limit_synthesis(b);return OP1_DRUM_OK;
}




uint32_t gin_sdk_render_bound(gin_sdk_binding_t *b,int16_t *out,const int16_t *working){
 uint32_t i,error=OP1_DRUM_OK;
 if(!out)return OP1_DRUM_RANGE;
 if(!current_hit(b))error=OP1_DRUM_STALE;
 else if(!working||!canonical_unchanged(&current->slot[b->hit_slot],b->hit_key))error=OP1_DRUM_WORKING;
 if(error){gin_sdk_retire_voice(b);for(i=0;i<128u;++i)out[i]=0;return error;}

#if GIN_RATE_2X



 if(b->owned.active){
  int16_t core[GIN_RESAMPLE_CORE_BLOCK];uint32_t start=b->owned.n;
  if(gin_integer_render64(&b->owned,core)){gin_sdk_retire_voice(b);for(i=0;i<128u;++i)out[i]=0;return OP1_DRUM_SNAPSHOT;}
  gin_resample_2x_process64(&b->reconstruction,core,out);
  if(!b->owned.active){
   uint32_t valid=b->owned.p.total>start?b->owned.p.total-start:0;
   uint32_t zeros=valid<GIN_RESAMPLE_CORE_BLOCK?GIN_RESAMPLE_CORE_BLOCK-valid:0;
   b->resample_left=zeros<GIN_RESAMPLE_DRAIN_CORE?GIN_RESAMPLE_DRAIN_CORE-zeros:0;
  }
 }else if(b->resample_left){
  gin_resample_2x_drain(&b->reconstruction,out,GIN_RESAMPLE_CORE_BLOCK);b->resample_left=0;
 }else for(i=0;i<128u;++i)out[i]=0;
#else
 if(b->owned.active){if(gin_integer_render(&b->owned,out)){gin_sdk_retire_voice(b);return OP1_DRUM_SNAPSHOT;}}
 else for(i=0;i<128u;++i)out[i]=0;
#endif
 for(i=0;b->tail_left&&i<128u;++i){
  uint32_t left=b->tail_left--;

  out[i]=(int16_t)(((int32_t)out[i]*(int32_t)(RETRIGGER_FADE_SAMPLES-left)+b->tail_sample*(int32_t)left)/(int32_t)RETRIGGER_FADE_SAMPLES);
 }
 b->last_sample=out[127];++b->block_age;return OP1_DRUM_OK;
}
uint32_t gin_sdk_status_bound(gin_sdk_binding_t *b){if(!current_hit(b)){gin_sdk_retire_voice(b);return 0;}return b->owned.active||b->tail_left||residual_reconstruction(b)?b->block_age:0;}


uint32_t gin_sdk_release_bound(gin_sdk_binding_t *b,const int16_t *control){if(!current_hit(b)||!coordinate(&current->slot[b->hit_slot],b->hit_key,control))return OP1_DRUM_IDENTITY;return OP1_DRUM_OK;}


static uint32_t trigger(void *self,const int16_t *p){return gin_sdk_trigger_bound(binding(self),p);}
static uint32_t render(void *self,int16_t *out,const int16_t *p){return gin_sdk_render_bound(binding(self),out,p);}
static uint32_t status(const void *self){return gin_sdk_status_bound(binding(self));}
static uint32_t release(void *self,const int16_t *p){return gin_sdk_release_bound(binding(self),p);}
const op1_drum_engine_t DRUM_ENGINE_gin={trigger,render,status,release};


static void *adjusted(const void *self,int32_t a){uintptr_t p=(uintptr_t)self;return(void *)(a<0?p-(uintptr_t)(0u-(uint32_t)a):p+(uintptr_t)a);}
static uint32_t original_trigger(void *s,const int16_t *p){gin_sdk_binding_t *b=binding(s);return b?b->original.trigger(adjusted(s,b->original_adjust[1]),p):OP1_DRUM_UNBOUND;}
static uint32_t original_render(void *s,int16_t *o,const int16_t *p){gin_sdk_binding_t *b=binding(s);return b?b->original.render(adjusted(s,b->original_adjust[0]),o,p):OP1_DRUM_UNBOUND;}
static uint32_t original_status(const void *s){gin_sdk_binding_t *b=binding(s);return b?b->original.status(adjusted(s,b->original_adjust[2])):0;}
static uint32_t original_release(void *s,const int16_t *p){gin_sdk_binding_t *b=binding(s);return b?b->original.release(adjusted(s,b->original_adjust[3]),p):OP1_DRUM_UNBOUND;}
static const op1_drum_engine_t stock={original_trigger,original_render,original_status,original_release};
const op1_drum_engine_t *op1_drum_resolve_receiver(const void *s){gin_sdk_binding_t *b=binding(s);return b?(b->selected?b->selected:&stock):0;}
