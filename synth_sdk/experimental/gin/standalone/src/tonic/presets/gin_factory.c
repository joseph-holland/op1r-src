/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#include "gin_factory.h"
#include "../factory_bank/gin_factory_bank.h"



int gin_factory_load_index(unsigned index,gin_preset_owned_t *out) {
 unsigned i,j,side,k;unsigned char *zero;
 const gin_factory_bank_entry_t *bank;
 if(!out||index>=GIN_FACTORY_BANK_COUNT)return -1;
 bank=&gin_factory_bank[index];zero=(unsigned char *)out;
 for(i=0;i<sizeof(*out);++i)zero[i]=0;
 out->count=GIN_PRESET_MAX_SOUNDS;
 for(i=0;i<GIN_PRESET_NAME_BYTES-1&&bank->name[i];++i)out->name[i]=bank->name[i];
 for(i=0;i<GIN_PRESET_MAX_SOUNDS;++i){
  out->sounds[i].id=1000u+i;
  for(j=0;j<GIN_PRESET_NAME_BYTES;++j)out->sounds[i].name[j]=bank->sound_name[i][j];
  for(side=0;side<2;++side){
   k=0;
#define COPY_PARAMETER(name) out->sounds[i].endpoint[side][k++]=bank->patch[i][side].name;
   TONIC_PARAMETERS(COPY_PARAMETER)
#undef COPY_PARAMETER
  }
 }
 for(i=0;i<GIN_PRESET_KEY_COUNT;++i)out->key_map[i]=bank->key_map[i];
 return 0;
}
void gin_factory_load(gin_preset_owned_t *out){(void)gin_factory_load_index(0,out);}
float gin_factory_duration(unsigned index,unsigned sound){
 return index<GIN_FACTORY_BANK_COUNT&&sound<GIN_PRESET_MAX_SOUNDS?
  gin_factory_bank[index].duration[sound]:0.f;
}
