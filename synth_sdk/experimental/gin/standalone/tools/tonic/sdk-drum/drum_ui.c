/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#include "op1_drum_ui.h"
const op1_drum_screen_module_t *op1_drum_find_screen(const char *name){
 uint32_t i,n;
 if(!name)return 0;
 for(i=0;i<op1_drum_screen_count;++i){
  const char *s=op1_drum_screens[i].name;
  if(!s)continue;
  for(n=0;n<16&&name[n]&&name[n]==s[n];++n){}
  if(n<16&&!name[n]&&!s[n])return &op1_drum_screens[i];
 }
 return 0;
}
