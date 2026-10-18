/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#include "gin_ui.h"
#include "po32_lut.h"
#include "po32_param_maps.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

float gin_ui_curve_power(float base,float exponent){return po32_lut_powf(base,exponent);}
static float clamp(float x, float lo, float hi) { return x<lo?lo:x>hi?hi:x; }
static int valid_patch(const po32_patch_params_t *p) {
#define CHECK(n) if (!(p->n>=0 && p->n<=1)) return 0;
    TONIC_PARAMETERS(CHECK)
#undef CHECK
    return 1;
}


static float hz(float x) { return 20.0f*po32_lut_powf(1000.0f,x); }
static float decay(float x) { return 10.0f*po32_lut_powf(1000.0f,x); }
static float attack(float x) { return po32_param_attack_seconds(x)*1000.0f; }
static float inverse(float y, float (*mapping)(float)) {
    float lo=0,hi=1;
    if (y<=mapping(0)) return 0;
    if (y>=mapping(1)) return 1;
    for (int i=0;i<24;i++) { float mid=(lo+hi)*.5f; if(mapping(mid)<y)lo=mid;else hi=mid; }
    return (lo+hi)*.5f;
}
void gin_ui_init(gin_ui_t *u) { if(u) { memset(u,0,sizeof(*u));u->last_encoder=-1; } }
po32_status_t gin_ui_set_state(gin_ui_t *u, gin_ui_state_t state) {
    if(!u || state<GIN_UI_UNAVAILABLE || state>GIN_UI_ERROR || state==GIN_UI_READY)return PO32_ERR_INVALID_ARG;
    if(u->revision==UINT32_MAX)return PO32_ERR_RANGE;
    uint32_t revision=u->revision+1;
    gin_ui_init(u);u->state=state;u->revision=revision;return PO32_OK;
}
po32_status_t gin_ui_load(gin_ui_t *u, const gin_ui_sound_t *s, uint32_t count, uint32_t rate) {
    if(!u||!s)return PO32_ERR_INVALID_ARG;
    if(!count||count>GIN_UI_MAX_SOUNDS||rate<TONIC_MIN_RATE||rate>TONIC_MAX_RATE||u->revision==UINT32_MAX)return PO32_ERR_RANGE;
    for(uint32_t i=0;i<count;i++) {
        if(!s[i].id||s[i].morph>1000||s[i].side<GIN_UI_A||s[i].side>GIN_UI_NO_HISTORY||!s[i].name[0]||
           !memchr(s[i].name,0,GIN_UI_NAME_BYTES)||!valid_patch(&s[i].endpoint[0])||!valid_patch(&s[i].endpoint[1]))return PO32_ERR_RANGE;
        for(uint32_t j=0;j<i;j++)if(s[i].id==s[j].id)return PO32_ERR_RANGE;
        for(size_t j=0;s[i].name[j];j++)if((unsigned char)s[i].name[j]<32||(unsigned char)s[i].name[j]>126)return PO32_ERR_RANGE;
    }
    uint32_t revision=u->revision+1;
    gin_ui_init(u);memcpy(u->sound,s,count*sizeof(*s));
    for(uint32_t i=0;i<count;i++)if(u->sound[i].side==GIN_UI_NO_HISTORY)u->sound[i].side=GIN_UI_A;
    strcpy(u->kit_name,"GIN");u->kit_revision=revision;u->state=GIN_UI_READY;u->count=count;u->sample_rate=rate;u->revision=revision;return PO32_OK;
}
po32_status_t gin_ui_select(gin_ui_t *u, uint32_t index) {
    if(!u||u->state!=GIN_UI_READY)return PO32_ERR_INVALID_ARG;
    if(index>=u->count||u->revision==UINT32_MAX)return PO32_ERR_RANGE;


    if(index==u->selected)return PO32_OK;
    u->selected=index;u->last_encoder=-1;u->edit_until_ms=0;u->revision++;return PO32_OK;
}
po32_status_t gin_ui_shift(gin_ui_t *u, int held) {
    if(!u||u->state!=GIN_UI_READY||(held!=0&&held!=1))return PO32_ERR_INVALID_ARG;
    u->shift=held;u->last_encoder=-1;u->edit_until_ms=0;return PO32_OK;
}
const char *const gin_ui_page_titles[GIN_UI_PAGE_COUNT]={"TONE","TONE ENVELOPE","PITCH MOD","NOISE FILTER","NOISE ENVELOPE","MIX AND LEVEL","EQ","VELOCITY"};
static unsigned page_index(uint32_t page){return page<GIN_UI_VIEW_COUNT?page:GIN_UI_VIEW_COUNT-1;}
static const int fields[GIN_UI_PAGE_COUNT][4]={{-1,0,1,-1},{-1,2,3,-1},{-1,4,6,5},{-1,7,8,9},{-1,10,11,12},{-1,13,14,17},{-1,15,16,-1},{-1,18,19,20}};
static float *parameter(po32_patch_params_t *p,int field){int index=0;
#define PICK(n) if(index++==field)return &p->n;
 TONIC_PARAMETERS(PICK)
#undef PICK
 return 0;
}
static int enum_field(int field){return field==0||field==4||field==7||field==10;}
static void change_parameter(po32_patch_params_t *p,int field,int d){float *x=parameter(p,field);if(!x)return;
 if(enum_field(field)){int e=*x<1.f/3?0:*x<2.f/3?1:2;e=(int)clamp((float)(e+d),0,2);*x=e*.5f;}
 else if(field==1)*x=clamp(*x+d/(12.f*po32_lut_log2f(1000)),0,1);
 else if(field==2||field==11)*x=inverse(attack(*x)+d*(field==11&&p->NEnvMod>=1.f/3&&p->NEnvMod<2.f/3?1.5f:1.f),attack);
 else if(field==3||field==12)*x=inverse(decay(*x)*po32_lut_exp2f(d/12.f),decay);
 else if(field==8||field==15)*x=clamp(*x+d/(24.f*po32_lut_log2f(1000)),0,1);
 else if(field==6)*x=po32_param_mod_from_semitones(po32_param_mod_semitones(*x,p->ModMode)+d,p->ModMode<1.f/3);
 else *x=clamp(*x+d*.01f,0,1);
}
po32_status_t gin_ui_set_side(gin_ui_t *u,gin_ui_side_t side){
 if(!u||u->state!=GIN_UI_READY||side>GIN_UI_B||side<GIN_UI_A)return PO32_ERR_INVALID_ARG;
 if(u->revision==UINT32_MAX)return PO32_ERR_RANGE;
 u->sound[u->selected].side=side;u->last_encoder=-1;u->edit_until_ms=0;++u->revision;u->dirty=1;return PO32_OK;
}
po32_status_t gin_ui_toggle_editor(gin_ui_t *u){
 if(!u||u->state!=GIN_UI_READY)return PO32_ERR_INVALID_ARG;if(u->revision==UINT32_MAX)return PO32_ERR_RANGE;
 if(u->mode==3)return gin_ui_toggle_help(u);
 u->message[0]=0;
 if(u->mode==2){gin_name_cancel(&u->draft);u->mode=1;u->page=page_index(u->name_return_page);u->selected=u->name_return_selected;strcpy(u->message,"NAME NOT APPLIED");}
 else if(!u->mode){u->mode=1;u->page=0;}
 else u->mode=0;
 u->last_encoder=-1;u->edit_until_ms=0;++u->revision;return PO32_OK;
}
po32_status_t gin_ui_toggle_help(gin_ui_t *u){
 if(!u||u->state!=GIN_UI_READY)return PO32_ERR_INVALID_ARG;
 if(u->revision==UINT32_MAX)return PO32_ERR_RANGE;
 if(u->mode==3){u->mode=u->help_return_mode;u->page=page_index(u->page);}else{u->help_return_mode=u->mode;u->mode=3;}
 u->last_encoder=-1;u->edit_until_ms=0;++u->revision;return PO32_OK;
}
po32_status_t gin_ui_page(gin_ui_t *u,int d){
 if(!u||u->state!=GIN_UI_READY||u->mode!=1)return PO32_ERR_INVALID_ARG;
 if(d< -1||d>1||u->revision==UINT32_MAX)return PO32_ERR_RANGE;
 u->message[0]=0;u->page=(uint32_t)clamp((float)((int)page_index(u->page)+d),0,GIN_UI_VIEW_COUNT-1);u->last_encoder=-1;u->edit_until_ms=0;++u->revision;return PO32_OK;
}
static void name_message(gin_ui_t *u,int result){
 if(result==GIN_NAME_FULL)strcpy(u->message,"NAME TOO LONG");
 else if(result==GIN_NAME_INVALID)strcpy(u->message,"EMPTY NAME");
 else if(result==GIN_NAME_STALE)strcpy(u->message,"NAME NOT APPLIED");
}
po32_status_t gin_ui_press(gin_ui_t *u,unsigned e){
 if(!u||u->state!=GIN_UI_READY||e>3)return PO32_ERR_INVALID_ARG;if(u->revision==UINT32_MAX)return PO32_ERR_RANGE;
 if((u->mode==1&&u->page>=GIN_UI_ABOUT_PAGE)||u->mode==3||(u->mode!=2&&e!=0)||(u->mode==2&&e==2))return PO32_OK;
 u->name_applied=0;u->message[0]=0;
 if(u->mode==2){
  if(e!=3)return PO32_OK;
  int target=-1;char result[24];
  if(u->draft.target==GIN_NAME_SOUND){for(unsigned i=0;i<u->count;i++)if(u->sound[i].id==u->draft.id)target=(int)i;
   if(target<0){gin_name_cancel(&u->draft);u->mode=1;u->page=page_index(u->name_return_page);u->selected=u->name_return_selected;strcpy(u->message,"NAME NOT APPLIED");++u->revision;return PO32_OK;}}
  int r=gin_name_execute(&u->draft,u->kit_slot,u->kit_revision,result);
  if(r==GIN_NAME_APPLY){memcpy(u->sound[target].name,result,24);u->name_applied=1;u->dirty=1;strcpy(u->message,"NAME APPLIED");}
  else name_message(u,r);
  if(!u->draft.active){u->mode=1;u->page=page_index(u->name_return_page);u->selected=u->name_return_selected;}++u->revision;return PO32_OK;
 }
 if(e==0)return gin_ui_set_side(u,u->sound[u->selected].side==GIN_UI_A?GIN_UI_B:GIN_UI_A);
 return PO32_OK;
}

po32_status_t gin_ui_begin_name(gin_ui_t *u,unsigned target){
 if(!u||u->state!=GIN_UI_READY||u->mode!=1||u->page>=GIN_UI_ABOUT_PAGE||u->shift||target!=GIN_NAME_SOUND)return PO32_ERR_INVALID_ARG;
 if(u->revision==UINT32_MAX)return PO32_ERR_RANGE;
 gin_ui_sound_t *sound=&u->sound[u->selected];
 int r=gin_name_begin(&u->draft,target,u->kit_slot,u->kit_revision,sound->id,sound->name);
 if(r==GIN_NAME_OK){u->page=page_index(u->page);u->name_return_page=u->page;u->name_return_selected=u->selected;u->mode=2;u->last_encoder=-1;u->edit_until_ms=0;u->message[0]=0;}else name_message(u,r);
 ++u->revision;return PO32_OK;
}

po32_status_t gin_ui_name_finish(gin_ui_t *u,int apply){
 if(!u||u->state!=GIN_UI_READY||u->mode!=2||(apply!=0&&apply!=1))return PO32_ERR_INVALID_ARG;
 if(u->revision==UINT32_MAX)return PO32_ERR_RANGE;
 unsigned previous=u->draft.action;
 u->draft.action=apply?GIN_NAME_DONE:GIN_NAME_CANCEL;
 int result=gin_ui_press(u,3);
 if(u->mode==2&&u->draft.active)u->draft.action=previous;
 return result;
}
po32_status_t gin_ui_rotate(gin_ui_t *u,unsigned encoder,int d,uint64_t now){
 if(!u||u->state!=GIN_UI_READY||encoder>3)return PO32_ERR_INVALID_ARG;
 if(d< -96||d>96||u->revision==UINT32_MAX)return PO32_ERR_RANGE;if(!d)return PO32_OK;
 if((u->mode==1&&u->page>=GIN_UI_ABOUT_PAGE)||u->mode==3||(u->mode==2&&encoder==2))return PO32_OK;
 u->message[0]=0;
 if(u->mode==2){int r=gin_name_turn(&u->draft,encoder,d);name_message(u,r);++u->revision;return PO32_OK;}
 gin_ui_sound_t *s=&u->sound[u->selected];int field;
 if(encoder==0){
  uint16_t previous=s->morph;s->morph=(uint16_t)clamp(s->morph+d*10.f,0,1000);


  if(s->morph!=previous&&s->morph!=500)s->side=s->morph<500?GIN_UI_A:GIN_UI_B;
 }
 else if(u->mode==1&&u->shift){
  if(encoder==1){int selected=(int)u->selected+d;selected=(int)clamp((float)selected,0,(float)u->count-1);return gin_ui_select(u,(unsigned)selected);}
  return PO32_OK;
 }else{
  static const int performance[2][4]={{-1,1,3,13},{-1,6,12,8}};
  field=u->mode==1?fields[page_index(u->page)][encoder]:performance[u->shift][encoder];if(field<0)return PO32_OK;
  change_parameter(&s->endpoint[s->side],field,d);
 }
 u->message[0]=0;u->last_encoder=(int)encoder;u->edit_until_ms=now>UINT64_MAX-GIN_UI_READOUT_HOLD_TICKS?UINT64_MAX:now+GIN_UI_READOUT_HOLD_TICKS;++u->revision;u->dirty=1;return PO32_OK;
}



static void scaled(gin_ui_readout_t *r,float a,float b,int pair,const char *small,const char *big) {
    int large=(a+.5f>=1000 || (pair&&b+.5f>=1000));
    if(pair && a!=b) {
        if(large)snprintf(r->value,sizeof(r->value),"%.1f/%.1f",a/1000,b/1000);
        else snprintf(r->value,sizeof(r->value),"%.0f/%.0f",a,b);
        snprintf(r->unit,sizeof(r->unit),"%s T/N",large?big:small);
    } else {
        if(large)snprintf(r->value,sizeof(r->value),"%.1f",a/1000);
        else snprintf(r->value,sizeof(r->value),"%.0f",a);
        snprintf(r->unit,sizeof(r->unit),"%s%s",large?big:small,pair?" T/N":"");
    }
    r->primary=a;r->secondary=b;
}
static void field_readout(gin_ui_readout_t *r,const po32_patch_params_t *p,int field,uint32_t rate){
 static const char *const labels[21]={"WAVE","PITCH","TONE ATK","TONE DECAY","MODE","SWEEP TIME","MOD AMOUNT","FILTER","CUTOFF","RESONANCE","ENV SHAPE","NOISE ATK","NOISE DECAY","BALANCE","DRIVE","EQ FREQ","EQ GAIN","LEVEL","TONE VEL","NOISE VEL","MOD VEL"};
 if(field<0){strcpy(r->label,"-");strcpy(r->value,"-");return;}
 po32_patch_params_t copy=*p;float x=*parameter(&copy,field);strcpy(r->label,field==11&&p->NEnvMod>=2.f/3?"HOLD":labels[field]);
 if(enum_field(field)){static const char *const names[4][3]={{"SINE","TRIANGLE","SAW"},{"SWEEP","SINE","NOISE"},{"LP","BP","HP"},{"EXP","LINEAR","RING"}};int set=field==0?0:field==4?1:field==7?2:3;int index=x<1.f/3?0:x<2.f/3?1:2;strcpy(r->value,names[set][index]);r->primary=x;}
 else if(field==1||field==8||field==15){scaled(r,field==1?hz(x):clamp(hz(x),0,rate*.45f),0,0,"HZ","KHZ");}
 else if(field==2||field==11)scaled(r,attack(x)*(field==11&&p->NEnvMod>=1.f/3&&p->NEnvMod<2.f/3?2.f/3:1.f),0,0,"MS","S");
 else if(field==3||field==12)scaled(r,decay(x)*(field==12&&p->NEnvMod>=1.f/3&&p->NEnvMod<2.f/3?2.f/3:1.f),0,0,"MS","S");
 else if(field==5){if(p->ModMode<1.f/3){if(x<=0){strcpy(r->value,"INF");strcpy(r->unit,"MS");}else scaled(r,1000.f*(1.03f-x)/(3.f*x),0,0,"MS","S");}else{strcpy(r->label,p->ModMode<2.f/3?"SINE RATE":"NOISE RATE");scaled(r,po32_param_mod_hz(x,p->ModMode<2.f/3),0,0,"HZ","KHZ");}}
 else if(field==6){r->primary=po32_param_mod_semitones(x,p->ModMode);snprintf(r->value,sizeof(r->value),"%+.0f",r->primary);strcpy(r->unit,"ST");}
 else if(field==9){r->primary=.1f*po32_lut_powf(100000,x);snprintf(r->value,sizeof(r->value),"%.1f",r->primary);strcpy(r->unit,"Q");}
 else if(field==16){r->primary=(x-.5f)*80;snprintf(r->value,sizeof(r->value),"%+.1f",r->primary);strcpy(r->unit,"DB");}
 else if(field==17){if(x<.0001f){strcpy(r->value,"MUTE");}else{r->primary=60*x-49-1/x;snprintf(r->value,sizeof(r->value),"%+.1f",r->primary);strcpy(r->unit,"DB");}}
 else{r->primary=x;snprintf(r->value,sizeof(r->value),"%.0f",x*(field>=18?200:100));strcpy(r->unit,field>=18?"% SENS":"%");}
}
po32_status_t gin_ui_snapshot(const gin_ui_t *u,uint64_t now,gin_ui_snapshot_t *v){
 if(!u||!v)return PO32_ERR_INVALID_ARG;memset(v,0,sizeof(*v));v->state=u->state;if(u->state!=GIN_UI_READY)return PO32_OK;
 const gin_ui_sound_t *s=&u->sound[u->selected];const po32_patch_params_t *p=&s->endpoint[s->side];
 v->id=s->id;v->revision=u->revision;memcpy(v->name,s->name,sizeof(v->name));memcpy(v->endpoint,s->endpoint,sizeof(v->endpoint));
 for(int i=0;i<2;i++){v->decay_ms[i][0]=decay(s->endpoint[i].OscDcy);v->decay_ms[i][1]=decay(s->endpoint[i].NEnvDcy);}
 v->morph=s->morph;v->side=s->side;v->shift=u->shift;v->mode=u->mode;v->page=page_index(u->page);v->dirty=u->dirty;
 memcpy(v->kit_name,u->kit_name,24);memcpy(v->message,u->message,24);v->draft=u->draft;
 if(u->mode==1&&v->page==GIN_UI_ABOUT_PAGE){for(unsigned e=0;e<4;e++)strcpy(v->readout[e].label,"-");return PO32_OK;}
 for(unsigned e=0;e<4;e++){
  gin_ui_readout_t *r=&v->readout[e];r->active=u->last_encoder==(int)e&&now<u->edit_until_ms;
  if(u->mode==2){
   const gin_name_t *n=&u->draft;
   if(e==0){strcpy(r->label,"CURSOR");snprintf(r->value,sizeof(r->value),"%u",n->cursor+1);}
   else if(e==1){strcpy(r->label,"CHARACTER");if(n->text[n->cursor]){r->value[0]=n->text[n->cursor];r->value[1]=0;}else strcpy(r->value,"APPEND");}
   else if(e==2){strcpy(r->label,"-");}
   else{strcpy(r->label,"ACTION");strcpy(r->value,gin_name_actions[n->action]);strcpy(r->unit,"RED PRESS");}
  }else if(e==0){strcpy(r->label,"MORPH");snprintf(r->value,sizeof(r->value),"%u",(unsigned)(s->morph+5)/10);strcpy(r->unit,"%");r->primary=s->morph*.001f;}
  else if(u->mode==1&&u->shift){if(e==1){strcpy(r->label,"SOUND");snprintf(r->value,sizeof(r->value),"%u",u->selected+1);}else{strcpy(r->label,"-");strcpy(r->value,"-");}}
  else{static const int performance[2][4]={{-1,1,3,13},{-1,6,12,8}};field_readout(r,p,u->mode==1?fields[v->page][e]:performance[u->shift][e],u->sample_rate);}
 }
 return PO32_OK;
}
