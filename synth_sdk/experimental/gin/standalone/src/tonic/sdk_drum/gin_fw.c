/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#include "gin_fw.h"
#include "gin_fw_level.h"
#include "gin_profile.h"
#ifdef GIN_NATIVE_SNAPSHOTS
#include "gin_fw_snapshot.h"
#include "gin_snapshot_text.h"
#include "gin_prepare.h"
static volatile uint32_t musical_epoch[8];
uint32_t gin_fw_snapshot_epoch(uint32_t slot){return slot<8?musical_epoch[slot]:0;}
uint32_t gin_fw_snapshot_capture(uint32_t slot,char *out,uint32_t capacity,uint32_t *epoch){
 uint32_t before,revision;int result;
 if(slot>=8||!epoch||!(gin_fw_selected_mask&(1u<<slot)))return OP1_DRUM_SELECTION;
 before=musical_epoch[slot];revision=gin_fw_runtime.slot[slot].revision;
 if(!before||(before&1u)||!revision)return OP1_DRUM_IDENTITY;
 __asm__ volatile("":::"memory");
 result=gin_snapshot_text_encode(&gin_fw_runtime.slot[slot].kit,out,capacity);
 __asm__ volatile("":::"memory");
 if(result||before!=musical_epoch[slot]||revision!=gin_fw_runtime.slot[slot].revision)return OP1_DRUM_SNAPSHOT;
 *epoch=before;return 0;
}
#endif
#ifdef GIN_FACTORY_BANK
#include "gin_factory.h"
static gin_preset_owned_t factory_kit;
static gin_prepared_t factory_prepared;
static uint32_t factory_words[21];
extern const uint32_t gin_factory_prepared_dictionary[];
#if GIN_FACTORY_PACKED_INDEX
extern const uint8_t gin_factory_prepared_index[];
static uint32_t prepared_index(uint32_t kit,uint32_t sound,uint32_t word_index){
 uint32_t n=(kit*24u+sound)*77u+word_index;
 const uint8_t *p=gin_factory_prepared_index+(n>>1)*3u;
 return n&1u?((uint32_t)p[1]>>4)|(uint32_t)p[2]<<4:(uint32_t)p[0]|((uint32_t)p[1]&15u)<<8;
}
#else
extern const uint16_t gin_factory_prepared_index[GIN_FACTORY_KIT_COUNT][24][77];
static uint32_t prepared_index(uint32_t kit,uint32_t sound,uint32_t word_index){return gin_factory_prepared_index[kit][sound][word_index];}
#endif
typedef char gin_factory_prepared_geometry[(sizeof(gin_prepared_t)==308)?1:-1];
static uint32_t preset_pending[8],preset_reload[8],preset_bank[8];
void gin_fw_preset_selected(uint32_t slot){if(slot<8)preset_reload[slot]=1;}
uint32_t gin_fw_factory_origin(uint32_t slot){
 return slot<8&&gin_fw_runtime.slot[slot].revision?preset_bank[slot]:0;
}
#endif



gin_sdk_runtime_t gin_fw_runtime;
volatile uint32_t gin_fw_fault,gin_fw_factory_count,gin_fw_hit_count,gin_fw_render_count;
static uint32_t initialized,selected_gin[8],factory_type;
static gin_slot_ticket_t ticket[8];
volatile uint32_t gin_fw_sql_result,gin_fw_selected_mask;
volatile char gin_fw_sql_error[160];
static uint32_t database_count;
#ifndef GIN_AUDIO_OBSERVATION
#define GIN_AUDIO_OBSERVATION 1
#endif
#ifdef OP1_HOST
const uint32_t gin_fw_audio_observation_enabled=GIN_AUDIO_OBSERVATION;
const uint32_t gin_fw_fast_fixed_enabled=GIN_FAST_FIXED;
volatile gin_fw_observation_t gin_fw_observation[12][3];
#else
const uint32_t gin_fw_audio_observation_enabled __attribute__((used,section(".rodata.gin_contract")))=GIN_AUDIO_OBSERVATION;
const uint32_t gin_fw_fast_fixed_enabled __attribute__((used,section(".rodata.gin_contract")))=GIN_FAST_FIXED;
volatile gin_fw_observation_t gin_fw_observation[12][3] __attribute__((section(".bss.gin_observation")));
#endif
typedef int (*gin_sql_callback_t)(void *,int,char **,char **);
extern int instr_name_eq(const char *,const gin_fw_string_t *);
#ifdef OP1_HOST
extern uint32_t gin_fw_stock_type(const gin_fw_string_t *);
extern uint32_t gin_fw_stock_factory(uint32_t,void **,uint32_t);
extern uint32_t gin_fw_native_core(void),gin_fw_native_selected(void);
extern const int16_t *gin_fw_native_base(uint32_t);
extern int gin_fw_native_receiver(uint32_t,const void *);
extern const int16_t *gin_fw_native_gain(void);
extern const op1_drum_engine_t gin_fw_stock_methods;
extern uint32_t gin_fw_stock_database_open(void);
extern uint32_t gin_fw_database_exec(const char *,gin_sql_callback_t,void *);
#else
extern uint32_t gin_fw_original_type(const gin_fw_string_t *);
extern uint32_t gin_fw_original_database_open(void);
#define gin_fw_stock_type(s) gin_fw_original_type(s)
#define gin_fw_stock_factory(t,p,s) (((uint32_t(*)(uint32_t,void **,uint32_t))0x01a1b9c0)(t,p,s))
static uint32_t gin_fw_native_core(void){return *(volatile uint32_t *)0x01aaac10;}
static uint32_t gin_fw_native_selected(void){return *(volatile uint32_t *)0xff8016d8;}
static const int16_t *gin_fw_native_base(uint32_t s){return(const int16_t *)(0x01aaadb4u+s*384u);}
static int gin_fw_native_receiver(uint32_t i,const void *p){return(uintptr_t)p==0xffb00000u+72u*i&&*(volatile const uint32_t *)p==0x0190f8a4u&&*(volatile const uint32_t *)((const unsigned char *)p+20)==i&&((void *volatile *)0x01aaad30)[i]==p;}
static const int16_t *gin_fw_native_gain(void){return(const int16_t *)0xff801d50;}
static const op1_drum_engine_t gin_fw_stock_methods={
 (op1_drum_trigger_fn)0x01970de6,(op1_drum_render_fn)0x01970ee8,
 (op1_drum_status_fn)0x01970e5c,(op1_drum_release_fn)0x01970e7e};
static uint32_t gin_fw_stock_database_open(void){return gin_fw_original_database_open();}
static uint32_t gin_fw_database_exec(const char *sql,gin_sql_callback_t callback,void *arg){


 void *connection=*(void *volatile *)0x01bab724;
 void *sem=*(void *volatile *)0x01bab718;uint32_t r;
 if(!connection)return OP1_DRUM_IDENTITY;




 if(!sem){
  ((void(*)(void *,const char *))0x0196f950)(connection,"SELECT 1 WHERE 0;");
  sem=*(void *volatile *)0x01bab718;
 }
 if(!sem)return OP1_DRUM_IDENTITY;
 ((void(*)(void *,int))0xffa0b6e8)(sem,0);
 r=((uint32_t(*)(void *,const char *,gin_sql_callback_t,void *,void *))0x019c9460)(*(void **)connection,sql,callback,arg,0);
 if(r){
  const char *message=((const char *(*)(void *))0x019d6c24)(*(void **)connection);
  uint32_t i=0;
  if(message)for(;i<sizeof(gin_fw_sql_error)-1&&message[i];++i)gin_fw_sql_error[i]=message[i];
  gin_fw_sql_error[i]=0;
 }
 ((void(*)(void *))0xffa0b96c)(sem);return r;
}
#endif
static int gin_catalog_found(void *arg,int columns,char **values,char **names){
 (void)columns;(void)values;(void)names;*(uint32_t *)arg=1u;return 0;
}
static int is_gin(const gin_fw_string_t *s){const char *p;if(!s||s->size!=3)return 0;p=s->cap>15?s->u.ptr:s->u.buf;return p&&p[0]=='g'&&p[1]=='i'&&p[2]=='n';}
int gin_fw_name_compare(const char *literal,const gin_fw_string_t *s){
 if(is_gin(s))return literal&&((literal[0]=='d'&&literal[1]=='b'&&literal[2]=='o'&&literal[3]=='x'&&!literal[4])||(literal[0]=='g'&&literal[1]=='i'&&literal[2]=='n'&&!literal[3]));
 return instr_name_eq(literal,s);
}
uint32_t gin_fw_database_open(void){
 uint32_t original=gin_fw_stock_database_open(),found=0;





 gin_fw_sql_result=gin_fw_database_exec("SELECT 1 FROM drum_types WHERE type='gin' LIMIT 1;",gin_catalog_found,&found);
 if(!gin_fw_sql_result&&!found)
  gin_fw_sql_result=gin_fw_database_exec("INSERT INTO drum_types(id,type,default_params) SELECT (SELECT COALESCE(MAX(id),-1)+1 FROM drum_types),'gin','[0,0,0,0,0,0,0,0]' WHERE NOT EXISTS(SELECT 1 FROM drum_types WHERE type='gin');",0,0);
 ++database_count;
 return original;
}
void gin_fw_shutdown(void){if(initialized)gin_sdk_deactivate(&gin_fw_runtime);factory_type=0;}
uint32_t gin_fw_activation_type(const gin_fw_string_t *s,uint32_t slot){
 uint32_t gin=is_gin(s),result;
 if(slot>=8u){gin_fw_fault=OP1_DRUM_RANGE;return gin?9u:gin_fw_stock_type(s);}
 selected_gin[slot]=is_gin(s)?1u:0u;
#ifdef GIN_FACTORY_BANK
 preset_pending[slot]=gin;
#endif
 if(selected_gin[slot])gin_fw_selected_mask|=1u<<slot;else gin_fw_selected_mask&=~(1u<<slot);


 result=gin?9u:gin_fw_stock_type(s);return result;
}
uint32_t gin_fw_type_entry(const gin_fw_string_t *s,uint32_t caller,const void *native_patch){
 if(caller==0x01a006f4u&&native_patch)return gin_fw_activation_type(s,*(const uint32_t *)native_patch);
 return is_gin(s)?9u:gin_fw_stock_type(s);
}
uint32_t gin_fw_factory_parent(uint32_t type){
#ifdef OP1_HOST
 extern void **gin_fw_native_publication(void);
 return gin_fw_factory(type,gin_fw_native_publication(),gin_fw_native_core());
 #else
 return gin_fw_factory(type,(void **)0x01aaad30,gin_fw_native_core());
 #endif
}
#ifdef GIN_FACTORY_BANK


static uint32_t load_factory_bank(uint32_t slot){
 const int16_t *base=gin_fw_native_base(slot);uint32_t i,j,index;
#ifdef GIN_NATIVE_SNAPSHOTS
 int restored;
#endif
 int marker=base[7];
 if(marker==0)index=0;
 else{
  if(marker<4096||marker>=4096+(int)GIN_FACTORY_KIT_COUNT)return OP1_DRUM_SNAPSHOT;
  index=(uint32_t)(marker-4096);
  for(i=0;i<24;i++)if(base[8*i+7]!=marker)return OP1_DRUM_SNAPSHOT;
 }
 if(gin_fw_runtime.slot[slot].revision&&preset_bank[slot]==index+1&&!preset_reload[slot])return 0;
#ifdef GIN_NATIVE_SNAPSHOTS
 restored=gin_fw_snapshot_restore(slot,&factory_kit);
 if(restored<0)return OP1_DRUM_SNAPSHOT;
 if(!restored&&gin_factory_load_index(index,&factory_kit))return OP1_DRUM_SNAPSHOT;
#else
 if(gin_factory_load_index(index,&factory_kit))return OP1_DRUM_SNAPSHOT;
#endif
 if(gin_sdk_load_kit(&gin_fw_runtime,slot,base,&factory_kit,&ticket[slot]))return OP1_DRUM_SNAPSHOT;
 for(i=0;i<factory_kit.count;i++){
  typedef uint32_t alias_word __attribute__((__may_alias__));
  alias_word *dst=(alias_word *)&factory_prepared;
#ifdef GIN_NATIVE_SNAPSHOTS
  if(restored){
   po32_patch_params_t patch;
   if(gin_integer_prepare_morph(&factory_prepared,&patch,factory_kit.sounds[i].endpoint,factory_kit.sounds[i].morph))return OP1_DRUM_SNAPSHOT;
   for(j=0;j<sizeof(factory_words);j++)((unsigned char *)factory_words)[j]=((const unsigned char *)&patch)[j];
  }else{
#endif
  for(j=0;j<77;j++)dst[j]=gin_factory_prepared_dictionary[prepared_index(index,i,j)];
  for(j=0;j<sizeof(factory_words);j++)((unsigned char *)factory_words)[j]=((const unsigned char *)factory_kit.sounds[i].endpoint[0])[j];
#ifdef GIN_NATIVE_SNAPSHOTS
  }
#endif
  if(gin_sdk_publish_prepared(&gin_fw_runtime,ticket[slot],factory_kit.sounds[i].id,factory_words,&factory_prepared,127))return OP1_DRUM_SNAPSHOT;
 }
 preset_bank[slot]=index+1;
#ifdef GIN_NATIVE_SNAPSHOTS
 musical_epoch[slot]+=2u;if(!musical_epoch[slot])musical_epoch[slot]=2;
 gin_fw_snapshot_loaded(slot);
#endif
 return 0;
}
#endif
uint32_t gin_fw_factory(uint32_t type,void **publication,uint32_t slot){
 uint32_t i,j,result;static const int32_t adjustment[4]={0,0,0,0};
 gin_fw_shutdown();
 result=gin_fw_stock_factory(type,publication,slot);
 ++gin_fw_factory_count;
 if(!initialized){gin_sdk_init(&gin_fw_runtime);initialized=1;}
 if(type!=9u)return result;
 if(slot>=8u||slot!=gin_fw_native_core()||!publication){gin_fw_fault=OP1_DRUM_IDENTITY;return result;}
 for(i=0;i<12u;++i)if(!gin_fw_native_receiver(i,publication[i])){gin_fw_fault=OP1_DRUM_IDENTITY;return result;}


 for(i=0;i<12u;++i)if(gin_sdk_bind(&gin_fw_runtime,i,publication[i],72,0x0190f8a4,&gin_fw_stock_methods,adjustment)){gin_fw_fault=OP1_DRUM_IDENTITY;return result;}
 for(i=0;i<8u;++i){
#ifdef GIN_FACTORY_BANK
  if(selected_gin[i]&&preset_pending[i]){
   if(load_factory_bank(i)){gin_fw_fault=OP1_DRUM_SNAPSHOT;return result;}
   preset_pending[i]=preset_reload[i]=0;
  }
#endif
  if(gin_fw_runtime.slot[i].revision){


   volatile unsigned char *dst=(volatile unsigned char *)gin_fw_runtime.slot[i].base_words;
   const volatile unsigned char *src=(const volatile unsigned char *)gin_fw_native_base(i);
   for(j=0;j<sizeof(gin_fw_runtime.slot[i].base_words);++j)dst[j]=src[j];
   gin_fw_runtime.slot[i].canonical_base=gin_fw_native_base(i);
   ticket[i].slot=i;ticket[i].revision=gin_fw_runtime.slot[i].revision;
  }else{
   if(gin_sdk_load_kit(&gin_fw_runtime,i,gin_fw_native_base(i),&gin_fw_builtin_kit,&ticket[i])){gin_fw_fault=OP1_DRUM_SNAPSHOT;return result;}
   for(j=0;j<gin_fw_builtin_kit.count;++j)if(gin_sdk_publish_prepared(&gin_fw_runtime,ticket[i],gin_fw_builtin_kit.sounds[j].id,gin_fw_builtin_patch[j],&gin_fw_builtin_prepared[j],127)){gin_fw_fault=OP1_DRUM_SNAPSHOT;return result;}
  }
 }
 for(i=0;i<12u;++i)if(gin_sdk_select_engine(&gin_fw_runtime,i,selected_gin[slot]?"gin":0)){gin_fw_fault=OP1_DRUM_SELECTION;return result;}
 if(gin_sdk_activate(&gin_fw_runtime)){gin_fw_fault=OP1_DRUM_IDENTITY;return result;}
 factory_type=type;gin_fw_fault=0;return result;
}
uint32_t gin_fw_update_sound(uint32_t slot,uint32_t sound,const gin_prepared_t *prepared,const uint32_t *patch,const void *endpoints,uint32_t morph,uint32_t side){
 uint32_t result,i;volatile unsigned char *dst;const volatile unsigned char *src;
 if(!initialized||slot>=8||sound>=gin_fw_runtime.slot[slot].kit.count||!endpoints||morph>1000||side>1)return OP1_DRUM_RANGE;
 if(factory_type!=9||gin_fw_native_core()!=slot||gin_fw_native_selected()!=slot||!selected_gin[slot])return OP1_DRUM_SELECTION;
 src=endpoints;
 for(i=0;i<42;++i){uint32_t w=(uint32_t)src[4*i]|(uint32_t)src[4*i+1]<<8|(uint32_t)src[4*i+2]<<16|(uint32_t)src[4*i+3]<<24;if(w>0x3f800000u&&w!=0x80000000u)return OP1_DRUM_SNAPSHOT;}
#ifdef GIN_NATIVE_SNAPSHOTS
 if(musical_epoch[slot]>=UINT32_MAX-1u)return OP1_DRUM_RANGE;
 ++musical_epoch[slot];
#endif
 result=gin_sdk_publish_prepared(&gin_fw_runtime,ticket[slot],gin_fw_runtime.slot[slot].kit.sounds[sound].id,patch,prepared,127);
#ifdef GIN_NATIVE_SNAPSHOTS
 if(result){++musical_epoch[slot];return result;}
#else
 if(result)return result;
#endif
 dst=(volatile unsigned char *)gin_fw_runtime.slot[slot].kit.sounds[sound].endpoint;src=endpoints;
 for(i=0;i<sizeof(gin_fw_runtime.slot[slot].kit.sounds[sound].endpoint);++i)dst[i]=src[i];
 gin_fw_runtime.slot[slot].kit.sounds[sound].morph=(uint16_t)morph;gin_fw_runtime.slot[slot].kit.sounds[sound].edit_side=(uint8_t)side;
#ifdef GIN_NATIVE_SNAPSHOTS
 ++musical_epoch[slot];
#endif
 return 0;
}
uint32_t gin_fw_rename(uint32_t slot,uint32_t revision,uint32_t id,const char name[24]){
 uint32_t i,len=0,ink=0;char *dst=0;
 if(!initialized||slot>=8||!name||!revision||gin_fw_runtime.slot[slot].revision!=revision)return OP1_DRUM_IDENTITY;
 if(factory_type!=9||gin_fw_native_core()!=slot||gin_fw_native_selected()!=slot||!selected_gin[slot])return OP1_DRUM_SELECTION;
 while(len<24&&name[len]){if((unsigned char)name[len]<32||(unsigned char)name[len]>126)return OP1_DRUM_RANGE;if(name[len]!=' ')ink=1;++len;}
 if(!len||len>=24||!ink)return OP1_DRUM_RANGE;
 if(!id)dst=gin_fw_runtime.slot[slot].kit.name;
 else for(i=0;i<gin_fw_runtime.slot[slot].kit.count;i++)if(gin_fw_runtime.slot[slot].kit.sounds[i].id==id){dst=gin_fw_runtime.slot[slot].kit.sounds[i].name;break;}
 if(!dst)return OP1_DRUM_IDENTITY;
#ifdef GIN_NATIVE_SNAPSHOTS
 if(musical_epoch[slot]>=UINT32_MAX-1u)return OP1_DRUM_RANGE;
 ++musical_epoch[slot];
#endif
 for(i=0;i<24;i++)dst[i]=i<len?name[i]:0;
#ifdef GIN_NATIVE_SNAPSHOTS
 ++musical_epoch[slot];
#endif
 return 0;
}
static int voice(const void *p){uint32_t i;for(i=0;i<12u;++i)if(gin_fw_runtime.binding[i].receiver==p&&gin_fw_native_receiver(i,p))return(int)i;return -1;}
static int route_ready(uint32_t slot){return initialized&&database_count&&!gin_fw_fault&&!gin_fw_sql_result&&factory_type==9u&&slot<8u&&selected_gin[slot];}
uint32_t gin_fw_route(const void *raw){return route_ready(gin_fw_native_core())&&voice(raw)>=0;}
static uint32_t selection(void){return gin_sdk_selection(&gin_fw_runtime,gin_fw_native_core(),gin_fw_native_selected());}
static int observation(uint32_t op,int v,void *p,uint32_t result,const int16_t *pcm){
#if !GIN_AUDIO_OBSERVATION


 (void)op;(void)v;(void)p;(void)result;(void)pcm;return 1;
#else
 gin_sdk_binding_t *b;volatile gin_fw_observation_t *o;uint32_t i,seq;
 if(v<0||v>=12||op>=3||result)return 0;b=&gin_fw_runtime.binding[v];o=&gin_fw_observation[v][op];seq=o->sequence;
 if(seq==UINT32_MAX){gin_fw_fault=OP1_DRUM_RANGE;gin_sdk_retire_voice(b);return 0;}
 o->factory=gin_fw_factory_count;o->generation=b->hit_generation;o->receiver=(uint32_t)(uintptr_t)p;o->voice=(uint32_t)v;o->slot=b->hit_slot;o->key=b->hit_key;o->sound_id=b->hit_sound_id;o->operation=op;o->result=result;
 for(i=0;i<128u;++i)o->pcm[i]=pcm?pcm[i]:0;
 o->sequence=seq+1u;return 1;
#endif
}
static uint32_t trigger_body(void *p,const int16_t *canonical){
 uint32_t slot=gin_fw_native_core(),key,r;uintptr_t delta;int v=voice(p);


 r=OP1_DRUM_IDENTITY;
 if(v<0||!route_ready(slot)||selection()||!canonical)goto rejected;
 if((uintptr_t)canonical<(uintptr_t)gin_fw_native_base(slot))goto rejected;
 delta=(uintptr_t)canonical-(uintptr_t)gin_fw_native_base(slot);
 if(delta>=384u||(delta&15u))goto rejected;key=(uint32_t)(delta>>4);




 r=gin_sdk_publish_hit(&gin_fw_runtime,(uint32_t)v,ticket[slot],key,127);
 if(!r)r=gin_sdk_trigger_bound(&gin_fw_runtime.binding[v],canonical);
 if(!r){if(!observation(0,v,p,r,0))return OP1_DRUM_RANGE;++gin_fw_hit_count;return 0;}
rejected:
 if(v>=0)gin_sdk_retire_voice(&gin_fw_runtime.binding[v]);return r;
}
uint32_t gin_fw_trigger(void *p,const int16_t *canonical){
 uint32_t t=gin_profile_cycles(),r=trigger_body(p,canonical);
 gin_profile_event_done(GIN_PROFILE_TRIGGER,t);gin_profile_result(GIN_PROFILE_TRIGGER,r);return r;
}
static uint32_t render_body(void *p,int16_t *out,const int16_t *working){
 uint32_t i,r;int v=voice(p);const int16_t *gain=gin_fw_native_gain();
 if(!out)return OP1_DRUM_RANGE;
 if(selection()||!working||!gain){if(v>=0)gin_sdk_retire_voice(&gin_fw_runtime.binding[v]);for(i=0;i<128u;++i)out[i]=0;return OP1_DRUM_WORKING;}
#if GIN_LEVEL_TRACE
 uint32_t gain_minimum=32767u,gain_maximum=0;
#endif
 for(i=0;i<128u;++i){int32_t value=gain[i];if(value<0){if(v>=0)gin_sdk_retire_voice(&gin_fw_runtime.binding[v]);for(i=0;i<128u;++i)out[i]=0;return OP1_DRUM_WORKING;}
#if GIN_LEVEL_TRACE
  if((uint32_t)value<gain_minimum)gain_minimum=(uint32_t)value;
  if((uint32_t)value>gain_maximum)gain_maximum=(uint32_t)value;
#endif
 }
 if(v<0||!route_ready(gin_fw_native_core())){if(v>=0)gin_sdk_retire_voice(&gin_fw_runtime.binding[v]);for(i=0;i<128u;++i)out[i]=0;return OP1_DRUM_SELECTION;}
 gin_profile_render_kind(gin_fw_runtime.binding[v].owned.active,gin_fw_runtime.binding[v].tail_left);
 r=gin_sdk_render_bound(&gin_fw_runtime.binding[v],out,working);
#if GIN_LEVEL_TRACE
 if(!r)gin_profile_gain_frame(gain_minimum,gain_maximum);
#endif
 if(!r){for(i=0;i<128u;++i)out[i]=gin_fw_output_sample(out[i],gain[i]);if(!observation(1,v,p,r,out)){for(i=0;i<128u;++i)out[i]=0;return OP1_DRUM_RANGE;}++gin_fw_render_count;}
 return r;
}
uint32_t gin_fw_render(void *p,int16_t *out,const int16_t *working){
 uint32_t t=gin_profile_cycles(),r=render_body(p,out,working);
 gin_profile_render_done(0,t);gin_profile_result(GIN_PROFILE_RENDER,r);return r;
}
uint32_t gin_fw_profile_stock_render(void *p,int16_t *out,const int16_t *working){
 uint32_t t=gin_profile_cycles(),r=gin_fw_stock_methods.render(p,out,working);
 gin_profile_render_done(1,t);return r;
}
uint32_t gin_fw_status(const void *p){uint32_t t=gin_profile_cycles(),r;int v=voice(p);selection();r=v>=0&&route_ready(gin_fw_native_core())?gin_sdk_status_bound(&gin_fw_runtime.binding[v]):op1_drum_native_status(p);gin_profile_event_done(GIN_PROFILE_STATUS,t);return r;}
static uint32_t release_body(void *p,const int16_t *canonical){uint32_t r;int v=voice(p);if(selection()||v<0||!route_ready(gin_fw_native_core()))return OP1_DRUM_SELECTION;r=gin_sdk_release_bound(&gin_fw_runtime.binding[v],canonical);if(!r&&!observation(2,v,p,r,0))return OP1_DRUM_RANGE;return r;}
uint32_t gin_fw_release(void *p,const int16_t *canonical){uint32_t t=gin_profile_cycles(),r=release_body(p,canonical);gin_profile_event_done(GIN_PROFILE_RELEASE,t);return r;}
