/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */


#include <stdint.h>
#include "gin_factory.h"
volatile uint32_t gin_fw_factory_visible,gin_fw_user_presets,gin_fw_preset_query_error;
typedef int (*sql_callback_t)(void *,int,char **,char **);
#ifdef OP1_HOST
extern uint32_t gin_fw_stock_json_equal(const void *,const void *);
extern uint32_t gin_fw_stock_preset_query(void *,void *,const char *);
extern uint32_t gin_fw_user_count_query(sql_callback_t,void *);
extern uint32_t gin_fw_stock_preset_copy(void *,uint32_t,const void *);
#else
static uint32_t gin_fw_stock_json_equal(const void *value,const void *other){return ((uint32_t(*)(const void *,const void *))0x019bdea4)(value,other);}
static uint32_t gin_fw_stock_preset_query(void *out,void *db,const char *table){return ((uint32_t(*)(void *,void *,const char *))0x0196fe8c)(out,db,table);}
static uint32_t gin_fw_stock_preset_copy(void *out,uint32_t slot,const void *json){return ((uint32_t(*)(void *,uint32_t,const void *))0x01a04c48)(out,slot,json);}
static uint32_t gin_fw_user_count_query(sql_callback_t callback,void *arg){
 void *db=*(void *volatile *)0x01bab720,*sem=*(void *volatile *)0x01bab718;uint32_t result;
 if(!db||!sem)return 1;
 ((void(*)(void *,int))0xffa0b6e8)(sem,0);
 result=((uint32_t(*)(void *,const char *,sql_callback_t,void *,void *))0x019c9460)(*(void **)db,"SELECT count(*) FROM drum_user;",callback,arg,0);
 ((void(*)(void *))0xffa0b96c)(sem);return result;
}
#endif
extern void gin_fw_preset_selected(uint32_t);
uint32_t gin_fw_browser_preset_copy(void *out,uint32_t slot,const void *json){
 uint32_t result=gin_fw_stock_preset_copy(out,slot,json);
 gin_fw_preset_selected(slot);return result;
}


uint32_t gin_fw_json_dbox(const void *value,const void *literal){
 uint32_t original=gin_fw_stock_json_equal(value,literal);const char *s,*expected;
 if(original||!value||!literal)return original;


 if(((const unsigned char *)value)[8]!=4||((const unsigned char *)literal)[8]!=4)return original;
 s=*(const char *const *)value;expected=*(const char *const *)literal;
 if(!s||!expected||expected[0]!='d'||expected[1]!='b'||expected[2]!='o'||expected[3]!='x'||expected[4])return original;
 return s[0]=='g'&&s[1]=='i'&&s[2]=='n'&&!s[3];
}
static int count_result(void *arg,int columns,char **values,char **names){
 uint32_t n=0,i=0;(void)names;
 if(columns!=1||!values||!values[0]||!values[0][0])return 1;
 while(values[0][i]){unsigned c=(unsigned char)values[0][i++];if(i>6||c<'0'||c>'9')return 1;n=n*10+c-'0';}
 *(uint32_t *)arg=n;return 0;
}
uint32_t gin_fw_factory_preset_query(void *out,void *db,const char *table){
 static char expression[160];uint32_t users=UINT32_MAX,available,i=0,j;
 static const char prefix[]="drum_presets WHERE folder IS NOT 'gin' OR id IN (SELECT id FROM drum_presets WHERE folder='gin' ORDER BY id LIMIT ";


 gin_fw_preset_query_error=gin_fw_user_count_query(count_result,&users);
 gin_fw_user_presets=users;
 available=gin_fw_preset_query_error||users>=34?0:34-users;if(available>GIN_FACTORY_KIT_COUNT)available=GIN_FACTORY_KIT_COUNT;
 gin_fw_factory_visible=available;
 for(j=0;prefix[j];j++)expression[i++]=prefix[j];
 if(available>=10)expression[i++]=(char)('0'+available/10);
 expression[i++]=(char)('0'+available%10);
 expression[i++]=')';expression[i]=0;
 (void)table;return gin_fw_stock_preset_query(out,db,expression);
}
