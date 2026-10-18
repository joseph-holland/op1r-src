/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */

#include "gin_factory.h"
#include "gin_prepare.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
int main(void){
 static gin_preset_owned_t kit;gin_prepared_t prepared;po32_patch_params_t patch;
 unsigned bank,sound;FILE *out=fopen("factory.prepared.bin","wb"),*kits=fopen("factory.kits.bin","wb");assert(out&&kits);
 for(bank=0;bank<GIN_FACTORY_KIT_COUNT;bank++){
  assert(!gin_factory_load_index(bank,&kit));
  assert(fwrite(&kit,1,sizeof(kit),kits)==sizeof(kit));
  for(sound=0;sound<24;sound++){
   memcpy(&patch,kit.sounds[sound].endpoint[0],sizeof(patch));
   assert(!gin_integer_prepare(&prepared,&patch,127,gin_factory_duration(bank,sound)));
   assert(fwrite(&prepared,1,sizeof(prepared),out)==sizeof(prepared));
  }
 }
 assert(!fclose(out)&&!fclose(kits));printf("prepared=%zu kit=%zu kits=%u sounds=%u\n",sizeof(prepared),sizeof(kit),GIN_FACTORY_KIT_COUNT,GIN_FACTORY_KIT_COUNT*24);return 0;
}
