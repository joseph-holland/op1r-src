/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */


#include "gin_controls.h"
#include "gin_fw.h"
#include "gin_profile.h"
#ifdef GIN_NATIVE_SNAPSHOTS
#include "gin_fw_snapshot.h"
#endif
#include "gin_prepare.h"
#include "gin_factory.h"
#include "po32_lut.h"
#include "po32_param_maps.h"
#include "op1_drum_ui.h"
#include "gin_event_queue.h"
#include <string.h>
#define TABLE ((void *volatile *)0x01df4e54)
#define SCREEN_ID 0x11u
#define CURRENT_SCREEN (*(volatile uint32_t *)0x018b65c8)
#define DRUM_MODE (*(volatile uint32_t *)0x01de4c40)
#define SELECTED_SLOT (*(volatile uint32_t *)0xff8016d8)
#define SHIFT (*(volatile uint8_t *)0x01dd7f78)
static uint32_t screen[32],vtable[10];
static void *original_screen;
static uint32_t ready,loaded_slot=8,loaded_revision;
static gin_ui_t owner;
static gin_ui_sound_t load_sounds[GIN_UI_MAX_SOUNDS];
static const op1_drum_screen_module_t *descriptor;
static gin_ui_snapshot_t cached;
static uint8_t selected_sound[8];
static uint32_t focus_id[8],clock_last,clock_started,hit_seen,hit_ui;
static uint64_t clock_now,message_until;
#define GUI_TICK (*(volatile uint32_t *)0xff8016e0)
#define LAST_PAD (*(volatile uint32_t *)0xff801044)
static volatile uint32_t head,tail;
static event_t events[GIN_EVENT_CAPACITY];
static uint32_t t_owned,click_owned,name_epoch;

static volatile uint32_t name_exit_pending,name_exit_epoch;
volatile uint32_t gin_ui_fault,gin_ui_edits,gin_ui_frames;
volatile uint32_t gin_ui_svg_fallback;


__attribute__((used,section(".rodata.gin_contract")))
const uint32_t gin_ui_contract_layout[]={
 offsetof(gin_sdk_runtime_t,slot),sizeof(gin_sdk_slot_t),
 offsetof(gin_sdk_slot_t,prepared),offsetof(gin_sdk_slot_t,patch_words),
 offsetof(gin_sdk_runtime_t,binding),sizeof(gin_sdk_binding_t),
 offsetof(gin_sdk_binding_t,owned),sizeof(gin_integer_voice_t),
 offsetof(gin_sdk_binding_t,view_prepared),offsetof(gin_sdk_binding_t,hit_patch_words)
};
__attribute__((used,section(".rodata.gin_contract")))
const uint32_t gin_ui_editor_layout[]={offsetof(gin_ui_snapshot_t,draft),offsetof(gin_ui_snapshot_t,kit_name),offsetof(gin_ui_snapshot_t,message),offsetof(gin_preset_owned_t,name),sizeof(gin_preset_owned_t),offsetof(gin_sdk_slot_t,kit)};
static uint32_t irq_off(void){uint32_t mask;__asm__ volatile("cli %0;":"=d"(mask)::"memory");return mask;}
static void irq_on(uint32_t mask){__asm__ volatile("sti %0;"::"d"(mask):"memory");}
static void mark_name_exit(void){uint32_t mask=irq_off();if(owner.draft.active){name_exit_epoch=name_epoch;name_exit_pending=1;}irq_on(mask);}
static int active(void){uint32_t slot=SELECTED_SLOT;return ready&&slot<8&&DRUM_MODE==1&&CURRENT_SCREEN==SCREEN_ID&&TABLE[SCREEN_ID]==screen&&(gin_fw_selected_mask&(1u<<slot))&&gin_fw_runtime.slot[slot].revision;}
void gin_fw_ui_ctor(void){
 ((void *(*)(void))0x019ba2f8)();
 original_screen=TABLE[SCREEN_ID];
 descriptor=op1_drum_find_screen("gin");
 if(!descriptor||!descriptor->screen||!descriptor->svg){gin_ui_fault=OP1_DRUM_UNBOUND;return;}
 ((void *(*)(void *,int,const char *))0x01a3bab0)(screen,SCREEN_ID,descriptor->svg);
 TABLE[SCREEN_ID]=original_screen;
 for(unsigned i=0;i<10;i++)vtable[i]=((const uint32_t *)0x019103fc)[i];
 vtable[7]=(uint32_t)(uintptr_t)descriptor->screen->update;vtable[9]=(uint32_t)(uintptr_t)descriptor->screen->init;screen[3]=(uint32_t)(uintptr_t)vtable;
}
void gin_fw_ui_load(void *table){
 ((void (*)(void *))0x01a3ba50)(table);
 original_screen=TABLE[SCREEN_ID];
 if(!descriptor)return;
 if(screen[2])screen[0]=(uint32_t)(uintptr_t)((void *(*)(void *))0x01a503d8)((void *)(uintptr_t)screen[2]);




 if(!screen[0]&&original_screen&&((uint32_t *)original_screen)[2]){
  screen[0]=(uint32_t)(uintptr_t)((void *(*)(void *))0x01a503d8)((void *)(uintptr_t)((uint32_t *)original_screen)[2]);gin_ui_svg_fallback=1;
 }
 if(screen[0]){descriptor->screen->init(screen);ready=1;}
}
uint32_t gin_fw_ui_select(void){uint32_t slot=SELECTED_SLOT;if(original_screen){int gin=ready&&slot<8&&(gin_fw_selected_mask&(1u<<slot));if(!gin||CURRENT_SCREEN!=SCREEN_ID)mark_name_exit();TABLE[SCREEN_ID]=gin?screen:original_screen;}return SCREEN_ID;}
static int sound_index(uint32_t slot,uint32_t id){
 const gin_preset_owned_t *kit=&gin_fw_runtime.slot[slot].kit;
 for(unsigned i=0;i<kit->count;i++)if(kit->sounds[i].id==id)return (int)i;
 return -1;
}
static int profiling_click(int code){
#ifdef GIN_PROFILE_DIAGNOSTIC
 if(code==59&&SHIFT){gin_profile_open^=1u;return 1;}
 if(gin_profile_open){if(code==62)gin_profile_reset();return 1;}
#else
 (void)code;
#endif
 return 0;
}
static int profiling_visible(void){
#ifdef GIN_PROFILE_DIAGNOSTIC
 return gin_profile_open!=0;
#else
 return 0;
#endif
}
static void queue(int kind,int value){
 uint32_t slot=SELECTED_SLOT,mask,id;int index;const gin_preset_owned_t *kit;
 if(slot>=8)return;mask=irq_off();kit=&gin_fw_runtime.slot[slot].kit;
 if(!kit->count){irq_on(mask);return;}
 id=focus_id[slot];index=sound_index(slot,id);if(index<0){index=0;id=kit->sounds[0].id;}
 if(kind==5){if(value<0||value>=24){irq_on(mask);return;}id=kit->key_map[value];if(sound_index(slot,id)<0){irq_on(mask);return;}focus_id[slot]=id;}
 if(kind==4){index+=value;while(index<0)index+=(int)kit->count;id=kit->sounds[(unsigned)index%kit->count].id;focus_id[slot]=id;kind=5;}
 {event_t event={slot,gin_fw_runtime.slot[slot].revision,kind,value,SHIFT!=0,id,owner.mode,owner.page,owner.sound[owner.selected].side,name_epoch};


  int dropped=gin_event_push(events,&head,tail,event)!=0;
  gin_profile_queue(head-tail,(uint32_t)dropped);
 }
 irq_on(mask);
}
void gin_fw_ui_key(int code,int pressed){
 uint32_t before=gin_fw_hit_count,slot=SELECTED_SLOT;

 if(code>=59&&code<=62){uint32_t bit=1u<<(code-59);
  if(click_owned&bit){if(!pressed)click_owned&=~bit;return;}
  if(active()){if(pressed){click_owned|=bit;if(!profiling_click(code))queue(8+code-59,0);}return;}
 }
 if(code>=6&&code<=9){uint32_t bit=1u<<(code-6);
  if(t_owned&bit){if(!pressed)t_owned&=~bit;return;}
  if(pressed&&active()&&SHIFT==0&&(code==6||owner.mode==1||owner.mode==2)){
   t_owned|=bit;
   if(owner.mode==1&&owner.page==GIN_UI_ABOUT_PAGE&&(code==8||code==9))return;
   if(code==6){
#ifdef GIN_PROFILE_DIAGNOSTIC
    if(gin_profile_open)gin_profile_open=0;else
#endif
    queue(owner.mode==2?11:6,owner.mode==2?1:0);
   }
   else if(code==7||code==8){if(owner.mode==1)queue(7,code==7?-1:1);}
   else queue(owner.mode==2?11:13,owner.mode==2?2:0);
   return;
  }
 }
 if(active()&&SHIFT==0&&(owner.mode==1||owner.mode==2)&&(code==0x24||code==0x25)){if(pressed)queue(7,code==0x25?1:-1);return;}
 if(pressed&&active()&&code==6&&SHIFT)mark_name_exit();
 ((void (*)(int,int))0x019b6c74)(code,pressed);
 if(!active())mark_name_exit();


 if(pressed&&active()&&SELECTED_SLOT==slot&&gin_fw_hit_count!=before)queue(5,(int)LAST_PAD);
}
void gin_fw_ui_knob(int knob,const int16_t *delta){
 if(active()&&profiling_visible())return;
 if(active()&&knob>=0&&knob<4&&delta){int d=*delta/1024;if(d>96)d=96;if(d< -96)d=-96;if(d)queue(knob,d);return;}
 ((void (*)(int,const int16_t *))0x019b6c0c)(knob,delta);
}
static int load(uint32_t slot){
 uint32_t page=owner.page;gin_ui_sound_t *sounds=load_sounds;const gin_preset_owned_t *kit=&gin_fw_runtime.slot[slot].kit;
 for(unsigned i=0;i<kit->count;i++){sounds[i].id=kit->sounds[i].id;memcpy(sounds[i].name,kit->sounds[i].name,24);memcpy(sounds[i].endpoint,kit->sounds[i].endpoint,sizeof(sounds[i].endpoint));sounds[i].morph=kit->sounds[i].morph;sounds[i].side=kit->sounds[i].edit_side?GIN_UI_B:GIN_UI_A;}
 if(gin_ui_load(&owner,sounds,kit->count,44100))return -1;owner.page=page<GIN_UI_VIEW_COUNT?page:GIN_UI_VIEW_COUNT-1;owner.kit_slot=slot;owner.kit_revision=gin_fw_runtime.slot[slot].revision;memcpy(owner.kit_name,kit->name,24);++name_epoch;
 {int index=sound_index(slot,focus_id[slot]);if(index>=0)gin_ui_select(&owner,(unsigned)index);else if(selected_sound[slot]<owner.count)gin_ui_select(&owner,selected_sound[slot]);}
 loaded_slot=slot;loaded_revision=gin_fw_runtime.slot[slot].revision;return 0;
}
static int publish(uint32_t slot){
 gin_ui_sound_t *s=&owner.sound[owner.selected];po32_patch_params_t patch;float *out=(float *)&patch;const float *a=(const float *)&s->endpoint[0],*b=(const float *)&s->endpoint[1];gin_prepared_t prepared;uint32_t words[21],mask,result;
 float morph=s->morph*.001f;
 for(unsigned i=0;i<21;i++)out[i]=a[i]*(1-morph)+b[i]*morph;
 {float tone=.01f*po32_lut_powf(1000,patch.OscDcy)+po32_param_attack_seconds(patch.OscAtk);
  float noise=.01f*po32_lut_powf(1000,patch.NEnvDcy)+po32_param_attack_seconds(patch.NEnvAtk);
  float duration=tone>noise?tone:noise;duration+=.05f;if(duration>10)duration=10;
  if(gin_integer_prepare(&prepared,&patch,127,duration))return -1;}
 memcpy(words,&patch,sizeof(words));


 mask=irq_off();result=gin_fw_update_sound(slot,owner.selected,&prepared,words,s->endpoint,s->morph,s->side);irq_on(mask);
 return result? -1:0;
}
uint32_t gin_native_snapshot(gin_ui_snapshot_t *out,uint32_t *now_ms){
 uint32_t slot=SELECTED_SLOT,mask;int changed=0;
 if(!out||!now_ms)return OP1_DRUM_RANGE;
 {uint32_t tick=GUI_TICK;if(!clock_started){clock_last=tick;clock_started=1;}clock_now+=(uint32_t)(tick-clock_last);clock_last=tick;*now_ms=(uint32_t)clock_now;}++gin_ui_frames;
 {uint32_t hits=gin_fw_hit_count;if(hits!=hit_seen){hit_seen=hits;++hit_ui;}}
 if(!active()||slot>=8){if(owner.draft.active){gin_name_cancel(&owner.draft);++name_epoch;}owner.mode=0;owner.message[0]=0;message_until=0;memset(&cached,0,sizeof(cached));*out=cached;return 0;}
 if(loaded_slot!=slot||loaded_revision!=gin_fw_runtime.slot[slot].revision){if(load(slot))return OP1_DRUM_SNAPSHOT;changed=1;}
 mask=irq_off();uint32_t cancel=name_exit_pending&&name_exit_epoch==name_epoch;name_exit_pending=0;irq_on(mask);
 if(cancel&&owner.draft.active){gin_name_cancel(&owner.draft);owner.mode=1;owner.page=owner.name_return_page<GIN_UI_PAGE_COUNT?owner.name_return_page:GIN_UI_PAGE_COUNT-1;strcpy(owner.message,"NAME NOT APPLIED");++name_epoch;changed=1;}
 if(owner.shift!=(SHIFT!=0)){gin_ui_shift(&owner,SHIFT!=0);changed=1;}
 for(unsigned n=0;n<16;n++){event_t event;mask=irq_off();if(tail==head){irq_on(mask);break;}event=events[tail&15];++tail;gin_profile_queue(head-tail,0);irq_on(mask);
  if(event.slot!=slot||event.revision!=loaded_revision)continue;
  if(owner.mode==3&&event.kind!=6&&event.kind!=5)continue;
  if(event.kind==6){if(event.mode!=owner.mode||event.name_epoch!=name_epoch)continue;++name_epoch;if(gin_ui_toggle_editor(&owner))gin_ui_fault=OP1_DRUM_SNAPSHOT;changed=1;continue;}
  if(event.kind==7){if(event.mode!=owner.mode||event.name_epoch!=name_epoch)continue;if(owner.mode==2){if(gin_ui_rotate(&owner,0,event.value,clock_now))gin_ui_fault=OP1_DRUM_SNAPSHOT;changed=1;continue;}if(owner.mode==1&&gin_ui_page(&owner,event.value))gin_ui_fault=OP1_DRUM_SNAPSHOT;changed=1;continue;}
  if(event.kind==13){
   if(owner.mode!=1||owner.page>=GIN_UI_ABOUT_PAGE||event.mode!=1||event.page!=owner.page||event.name_epoch!=name_epoch)continue;
   int index=sound_index(slot,event.sound_id);if(index<0)continue;
   if(owner.selected!=(unsigned)index)gin_ui_select(&owner,(unsigned)index);
   if(gin_ui_begin_name(&owner,GIN_NAME_SOUND))gin_ui_fault=OP1_DRUM_SNAPSHOT;
   ++name_epoch;changed=1;continue;
  }
  if(event.kind>=8){
   if(event.mode!=owner.mode||event.page!=owner.page||event.name_epoch!=name_epoch)continue;
   uint32_t previous=owner.mode;gin_name_t capture=owner.draft;
   if(owner.mode!=2){int index=sound_index(slot,event.sound_id);if(index<0)continue;if(owner.selected!=(unsigned)index)gin_ui_select(&owner,(unsigned)index);}
   if(owner.mode==2&&event.kind==11&&event.value){if(gin_ui_name_finish(&owner,event.value==1)){gin_ui_fault=OP1_DRUM_SNAPSHOT;continue;}}
   else if(gin_ui_press(&owner,(unsigned)event.kind-8)){gin_ui_fault=OP1_DRUM_SNAPSHOT;continue;}
   if(previous==2&&owner.name_applied){
    int index=sound_index(slot,capture.id);const char *name=index>=0?owner.sound[index].name:0;
    mask=irq_off();uint32_t result=gin_fw_rename(slot,capture.revision,capture.id,name);irq_on(mask);
    if(result){if(load(slot))gin_ui_fault=OP1_DRUM_SNAPSHOT;strcpy(owner.message,"NAME NOT APPLIED");owner.mode=1;owner.page=owner.name_return_page<GIN_UI_PAGE_COUNT?owner.name_return_page:GIN_UI_PAGE_COUNT-1;}
   }else if(previous!=2&&event.kind==8){if(publish(slot))gin_ui_fault=OP1_DRUM_SNAPSHOT;}
   if(previous==2&&owner.mode!=2){mask=irq_off();if(focus_id[slot]==event.sound_id)focus_id[slot]=owner.sound[owner.selected].id;irq_on(mask);}
   if(previous!=owner.mode)++name_epoch;changed=1;continue;
  }
  if(event.kind<4&&owner.mode==2){if(event.mode!=2||event.name_epoch!=name_epoch)continue;if(gin_ui_rotate(&owner,(unsigned)event.kind,event.value,clock_now))gin_ui_fault=OP1_DRUM_SNAPSHOT;changed=1;continue;}
  if(event.kind<4&&(event.mode!=owner.mode||event.page!=owner.page||(event.kind!=0&&event.side!=(uint32_t)owner.sound[owner.selected].side)))continue;
  {int index=sound_index(slot,event.sound_id);if(index<0)continue;
   if(event.kind==5||owner.selected!=(unsigned)index){gin_ui_select(&owner,(unsigned)index);selected_sound[slot]=(uint8_t)index;}
  }
  if(event.kind!=5){uint32_t before=owner.revision;gin_ui_shift(&owner,event.shift);if(gin_ui_rotate(&owner,(unsigned)event.kind,event.value,clock_now)){gin_ui_fault=OP1_DRUM_SNAPSHOT;continue;}
   if(owner.revision!=before){selected_sound[slot]=(uint8_t)owner.selected;



    if(event.mode==1&&event.shift&&event.kind==1){mask=irq_off();if(focus_id[slot]==event.sound_id)focus_id[slot]=owner.sound[owner.selected].id;irq_on(mask);}
    if(publish(slot)){gin_ui_fault=OP1_DRUM_SNAPSHOT;continue;}++gin_ui_edits;}
  }
  changed=1;
 }
 if(owner.shift!=(SHIFT!=0)){gin_ui_shift(&owner,SHIFT!=0);changed=1;}
 if(owner.message[0]&&strcmp(owner.message,cached.message))message_until=clock_now+GIN_UI_READOUT_HOLD_TICKS;
 if(owner.message[0]&&clock_now>=message_until){owner.message[0]=0;changed=1;}
 if(changed)gin_ui_snapshot(&owner,clock_now,&cached);
#ifdef GIN_FACTORY_BANK
 {uint32_t origin=gin_fw_factory_origin(slot);
 cached.decoration=origin==GIN_FACTORY_LINN_ID?GIN_UI_DECOR_LINN:origin==GIN_FACTORY_DEEP_ID?GIN_UI_DECOR_FISH:GIN_UI_DECOR_NONE;}
#endif
 for(unsigned i=0;i<4;i++)cached.readout[i].active=owner.last_encoder==(int)i&&clock_now<owner.edit_until_ms;
  *out=cached;
#ifdef GIN_NATIVE_SNAPSHOTS
 if(gin_fw_save_error)strcpy(out->message,"SAVE RETRY");
#endif
 return gin_ui_fault;
}

uint32_t gin_native_hit_count(void){return hit_ui;}
