/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#include "gin_name.h"
#include <string.h>
const char gin_name_characters[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_";
const char *const gin_name_actions[4]={"DONE","CANCEL","INSERT SPACE","DELETE"};
static unsigned length(const char *s){unsigned i;for(i=0;i<GIN_NAME_BYTES&&s[i];i++){}return i;}
static int clamp(int x,int high){return x<0?0:x>high?high:x;}
static int valid(const char *s){unsigned n=length(s),ink=0;if(!n||n>=GIN_NAME_BYTES)return 0;for(unsigned i=0;i<n;i++){if((unsigned char)s[i]<32||(unsigned char)s[i]>126)return 0;if(s[i]!=' ')ink=1;}return(int)ink;}
void gin_name_cancel(gin_name_t *n){if(n)n->active=0;}
int gin_name_begin(gin_name_t *n,uint32_t target,uint32_t slot,uint32_t revision,uint32_t id,const char *name){
 gin_name_t candidate={0};if(!n||!name||target!=GIN_NAME_SOUND||slot>=8||!revision||!id||!valid(name))return GIN_NAME_INVALID;
 candidate.target=target;candidate.slot=slot;candidate.revision=revision;candidate.id=id;candidate.active=1;candidate.action=GIN_NAME_DELETE;memcpy(candidate.text,name,length(name)+1);for(unsigned i=0;candidate.text[i];++i)if(candidate.text[i]>='a'&&candidate.text[i]<='z')candidate.text[i]-='a'-'A';*n=candidate;return GIN_NAME_OK;
}
int gin_name_turn(gin_name_t *n,unsigned e,int d){
 if(!n||!n->active||e>3||d< -96||d>96||n->bank||n->action<GIN_NAME_SPACE||n->action>GIN_NAME_DELETE)return GIN_NAME_INVALID;if(!d)return GIN_NAME_OK;
 unsigned len=length(n->text);if(len>=GIN_NAME_BYTES||n->cursor>len)return GIN_NAME_INVALID;
 if(e==0)n->cursor=(uint32_t)clamp((int)n->cursor+d,(int)len);
 else if(e==2)return GIN_NAME_OK;
 else if(e==3)n->action=clamp((n->action==GIN_NAME_SPACE?1:0)+d,1)?GIN_NAME_SPACE:GIN_NAME_DELETE;
 else{
  const char *bank=gin_name_characters;int size=(int)sizeof(gin_name_characters)-1,index=-1;
  if(n->cursor==len&&len==GIN_NAME_BYTES-1)return GIN_NAME_FULL;
  for(int i=0;i<size;i++)if(n->text[n->cursor]==bank[i]){index=i;break;}
  index=index<0?(d>0?d-1:d):index+d;index%=size;if(index<0)index+=size;
  n->text[n->cursor]=bank[index];if(n->cursor==len)n->text[len+1]=0;
 }
 return GIN_NAME_OK;
}
int gin_name_execute(gin_name_t *n,uint32_t slot,uint32_t revision,char out[GIN_NAME_BYTES]){
 if(!n||!n->active||n->target!=GIN_NAME_SOUND||!n->id||!out||n->action>3)return GIN_NAME_INVALID;
 unsigned len=length(n->text);if(len>=GIN_NAME_BYTES||n->cursor>len)return GIN_NAME_INVALID;
 if(slot!=n->slot||revision!=n->revision){n->active=0;return GIN_NAME_STALE;}
 if(n->action==GIN_NAME_DONE){if(!valid(n->text))return GIN_NAME_INVALID;char copy[GIN_NAME_BYTES]={0};memcpy(copy,n->text,len);memcpy(out,copy,GIN_NAME_BYTES);n->active=0;return GIN_NAME_APPLY;}
 if(n->action==GIN_NAME_CANCEL){n->active=0;return GIN_NAME_CANCELLED;}
 if(n->action==GIN_NAME_SPACE){if(len==GIN_NAME_BYTES-1)return GIN_NAME_FULL;for(unsigned i=len+1;i>n->cursor;i--)n->text[i]=n->text[i-1];n->text[n->cursor]=' ';++n->cursor;}
 else if(len){unsigned at=n->cursor<len?n->cursor:len-1;for(unsigned i=at;i<len;i++)n->text[i]=n->text[i+1];if(n->cursor>len-1)n->cursor=len-1;}
 return GIN_NAME_OK;
}
