/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */

#include "op1_drum.h"
const uint32_t op1_drum_native_slot_offsets[4]={0x14,0x1c,0x24,0x2c};
static int equal(const char *a,const char *b){uint32_t i;if(!a||!b)return 0;for(i=0;i<16u;++i){if(a[i]!=b[i])return 0;if(!a[i])return 1;}return 0;}
const op1_drum_engine_t *op1_drum_find(const char *name){uint32_t i;for(i=0;i<op1_drum_count;++i)if(equal(name,op1_drums[i].name))return op1_drums[i].engine;return 0;}
int op1_drum_name_matches(const char *name,uint32_t count){uint32_t i,j;if(!name||count>15u)return 0;for(i=0;i<op1_drum_count;++i){const char *n=op1_drums[i].name;for(j=0;j<count;++j)if(!n[j]||n[j]!=name[j])break;if(j==count&&!n[j])return 1;}return 0;}
uint32_t op1_drum_native_trigger(void *s,const int16_t *p){const op1_drum_engine_t *d=op1_drum_resolve_receiver(s);return d&&d->trigger?d->trigger(s,p):OP1_DRUM_UNBOUND;}
uint32_t op1_drum_native_render(void *s,int16_t *out,const int16_t *p){const op1_drum_engine_t *d=op1_drum_resolve_receiver(s);uint32_t i;if(d&&d->render)return d->render(s,out,p);if(out)for(i=0;i<128u;++i)out[i]=0;return OP1_DRUM_UNBOUND;}
uint32_t op1_drum_native_status(const void *s){const op1_drum_engine_t *d=op1_drum_resolve_receiver(s);return d&&d->status?d->status(s):0;}
uint32_t op1_drum_native_release(void *s,const int16_t *p){const op1_drum_engine_t *d=op1_drum_resolve_receiver(s);return d&&d->release?d->release(s,p):OP1_DRUM_UNBOUND;}
void op1_drum_build_selected_view(op1_drum_selected_view_t *v,const uintptr_t *prefix){uint32_t i;if(!v||!prefix)return;for(i=0;i<4u;++i)v->prefix[i]=prefix[i];v->render_adjust=v->trigger_adjust=v->status_adjust=v->release_adjust=0;v->render=op1_drum_native_render;v->trigger=op1_drum_native_trigger;v->status=op1_drum_native_status;v->release=op1_drum_native_release;}
