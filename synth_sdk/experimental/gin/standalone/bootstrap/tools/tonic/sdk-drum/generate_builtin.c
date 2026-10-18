/* Copyright (c) 2026 SquareWave Studio; libpo32-derived portions (c) 2026 Eric Lewis. MIT; see LICENSES/MIT.txt. */

#include "gin_prepare.h"
#include "gin_preset_state.h"
#include "gin_factory.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
int main(void){
 gin_preset_owned_t kit={0};gin_prepared_t prepared[GIN_PRESET_MAX_SOUNDS];uint32_t words[GIN_PRESET_MAX_SOUNDS][21];unsigned i,j;
 FILE *f;gin_factory_load(&kit);
 for(i=0;i<GIN_PRESET_MAX_SOUNDS;++i){po32_patch_params_t p;float pv[21];
  for(j=0;j<21;++j)pv[j]=kit.sounds[i].endpoint[0][j];
  memcpy(&p,pv,sizeof(p));memcpy(words[i],pv,sizeof(words[i]));assert(!gin_integer_prepare(&prepared[i],&p,127,gin_factory_duration(0,i)));
 }
 f=fopen("builtin.kit.bin","wb");assert(f&&fwrite(&kit,1,sizeof(kit),f)==sizeof(kit));fclose(f);
 f=fopen("builtin.prepared.bin","wb");assert(f&&fwrite(prepared,1,sizeof(prepared),f)==sizeof(prepared));fclose(f);
 f=fopen("builtin.patch.bin","wb");assert(f&&fwrite(words,1,sizeof(words),f)==sizeof(words));fclose(f);
 printf("kit=%zu prepared=%zu full21patch=%zu sounds=24 keys=24 velocity_timbre=127 morph=0\n",sizeof(kit),sizeof(prepared[0]),sizeof(words[0]));return 0;
}
