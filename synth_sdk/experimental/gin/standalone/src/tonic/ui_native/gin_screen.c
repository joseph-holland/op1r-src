/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */



#include "gin_screen.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#ifdef GIN_PROFILE_DIAGNOSTIC
#include "gin_profile.h"
#include "gin_fw.h"
#include "gin_controls.h"

#endif

#define Q(v) ((int32_t)((v) * 1024))
#define LIQUID_STROKE Q(3)
#define COUNT(a) ((unsigned)(sizeof(a)/sizeof((a)[0])))
#ifndef GIN_PARAMS_ONLY
#define GIN_PARAMS_ONLY 0
#endif


#ifdef OP1_HOST
__attribute__((used))
#else
__attribute__((used,section(".data.gin_variant")))
#endif
const volatile uint32_t gin_params_only=GIN_PARAMS_ONLY;
static const uint8_t bg[4]={0,0,0,255},face[4]={0,0,0,255};
static const uint8_t blue[4]={105,142,255,255},green[4]={0,237,149,255};
static const uint8_t white[4]={230,231,255,255},orange[4]={255,66,63,255};
static const uint8_t dim[4]={98,93,119,255};
#include "gin_geometry.inc"
#include "gin_logo.inc"
#include "gin_motion.inc"
#include "gin_motion.h"
static op1_pt moving_body[96],moving_mouth[96],moving_brand[2];
static int32_t moving_matrix[9],mouth_x[2],mouth_y[2];
static gin_motion_t motion;
static int32_t draw_dx,draw_dy;
static unsigned native_vessels,motion_visible;
static uint32_t text_alpha=32768;
static int32_t liquid_wave[65],wave_params[2][4];
static uint32_t wave_id,wave_morph=UINT32_MAX,visual_revision;
volatile uint32_t gin_scene_observation[12];
typedef struct {int from,to;int32_t x[160][2];} span_cache_t;
static span_cache_t glass_spans,glass_fill_spans,mouth_spans[2];

typedef struct {int from,to;uint8_t count[160];int32_t x[160][4];} bottle_cache_t;
static bottle_cache_t bottle_spans[2];
static uint32_t clip_ready,span_pose[2]={UINT32_MAX,UINT32_MAX};
#ifdef OP1_HOST
uint32_t gin_host_edges,gin_host_text_divisions;
#endif
static op1_pt move_point(op1_pt p,const int32_t *m){op1_pt q;q.x=(int32_t)((m[0]*p.x+m[2]*p.y)>>10)+m[4];q.y=(int32_t)((m[1]*p.x+m[3]*p.y)>>10)+m[5];return q;}

static void line(void *r,int32_t x0,int32_t y0,int32_t x1,int32_t y1,int32_t w,const uint8_t *c){
 op1_pt a={x0+draw_dx,y0+draw_dy},b={x1+draw_dx,y1+draw_dy};
 if(x0!=x1||y0!=y1)op1_raster_line(r,&a,&b,w,c);
}
static void stroke(void *r,const op1_pt *p,unsigned n,int closed,int32_t w,const uint8_t *c){
 unsigned i;for(i=1;i<n;++i)line(r,p[i-1].x,p[i-1].y,p[i].x,p[i].y,w,c);
 if(closed)line(r,p[n-1].x,p[n-1].y,p[0].x,p[0].y,w,c);
}



static unsigned intersections(const op1_pt *p,unsigned n,int32_t y,int32_t out[96]){
 unsigned i,j,count=0;
 if(!p||n<3||n>96)return 0;
#ifdef OP1_HOST
 gin_host_edges+=n;
#endif
 for(i=0,j=n-1;i<n;j=i++){
  int32_t a=p[j].y,b=p[i].y,x;
  if((a<=y&&b>y)||(b<=y&&a>y)){


   int32_t dx=(p[i].x-p[j].x)>>5,dy=y-a,den=b-a;
   if(!den)continue;x=p[j].x+((dx*dy)/den)*32;
   if(count<96)out[count++]=x;
  }
 }
 for(i=1;i<count;++i){int32_t x=out[i];j=i;while(j&&out[j-1]>x){out[j]=out[j-1];--j;}out[j]=x;}
 return count;
}
static void prepare_spans(span_cache_t *c,const op1_pt *p,unsigned n){
 int32_t y,x[96],lo=Q(160),hi=0;unsigned i,count;
 for(i=0;i<n;i++){if(p[i].y<lo)lo=p[i].y;if(p[i].y>hi)hi=p[i].y;}
 c->from=lo<0?0:((lo+1023)>>10);c->to=hi>Q(160)?160:((hi+511)>>10);
 for(int row=c->from;row<c->to;row++){y=row*1024+512;count=intersections(p,n,y,x);c->x[row][0]=count>=2?x[0]:0;c->x[row][1]=count>=2?x[count-1]:0;}
}


static void paint_spans(void *r,const span_cache_t *c,int32_t level,const uint8_t *above,const uint8_t *below){
 for(int row=c->from;row<c->to;row++)line(r,c->x[row][0],row*1024+512,c->x[row][1],row*1024+512,1024,row*1024+512<level?above:below);
}



static unsigned restrict_spans(int32_t mask[96],unsigned count,const int32_t *limit,unsigned limits){
 int32_t next[96];unsigned used=0;
 for(unsigned i=1;i<count;i+=2)for(unsigned j=1;j<limits;j+=2){int32_t l=mask[i-1]>limit[j-1]?mask[i-1]:limit[j-1],h=mask[i]<limit[j]?mask[i]:limit[j];if(h>l&&used<94){next[used++]=l;next[used++]=h;}}
 for(unsigned i=0;i<used;i++)mask[i]=next[i];return used;
}
static unsigned band_spans(const op1_pt *p,unsigned n,int32_t y,int32_t half,int32_t out[96]){
 int32_t limits[96],low=y-half,high=y+half;unsigned count=intersections(p,n,y,out),lim;
 lim=intersections(p,n,low,limits);count=restrict_spans(out,count,limits,lim);
 lim=intersections(p,n,high,limits);count=restrict_spans(out,count,limits,lim);


 for(unsigned i=0;i<n&&count;i++)if(p[i].y>low&&p[i].y<high){lim=intersections(p,n,p[i].y,limits);count=restrict_spans(out,count,limits,lim);lim=intersections(p,n,p[i].y-1,limits);count=restrict_spans(out,count,limits,lim);}
 unsigned used=0;for(unsigned i=1;i<count;i+=2){int32_t l=out[i-1]+half+64,h=out[i]-half-64;if(h>l){out[used++]=l;out[used++]=h;}}
 return used<=4?used:0;
}
static void prepare_bottle_spans(bottle_cache_t *c,const op1_pt *p,unsigned n){
 int32_t low=Q(160),high=0,out[96];for(unsigned i=0;i<n;i++){if(p[i].y<low)low=p[i].y;if(p[i].y>high)high=p[i].y;}
 c->from=low<0?0:((low+1023)>>10);c->to=high>Q(160)?160:((high+511)>>10);
 for(int row=c->from;row<c->to;row++){unsigned count=band_spans(p,n,row*1024+512,(LIQUID_STROKE+1)/2,out);c->count[row]=(uint8_t)count;for(unsigned j=0;j<count;j++)c->x[row][j]=out[j];}
}


static void paint_bottle(void *r,const bottle_cache_t *c,int32_t level,const uint8_t *liquid){
 for(unsigned pass=0;pass<2;pass++)for(int row=c->from;row<c->to;row++){
  int32_t y=row*1024+512;if(pass&&y<level)continue;
  for(unsigned j=1;j<c->count[row];j+=2){int32_t inset=native_vessels?Q(1.2):0;
   if(c->x[row][j]-c->x[row][j-1]>2*inset)line(r,c->x[row][j-1]+inset,y,c->x[row][j]-inset,y,LIQUID_STROKE,pass?liquid:face);}
 }
}
static void prepare_glass_spans(void){
 prepare_spans(&glass_spans,glass_body,COUNT(glass_body));
 glass_fill_spans.from=glass_spans.from;glass_fill_spans.to=glass_spans.to;
 for(int row=glass_spans.from;row<glass_spans.to;row++){
  int32_t x[96];unsigned n=band_spans(glass_body,COUNT(glass_body),row*1024+512,(LIQUID_STROKE+1)/2,x);
  glass_fill_spans.x[row][0]=n==2?x[0]:0;glass_fill_spans.x[row][1]=n==2?x[1]:0;
 }
}
static void bottle_surface(void *r,const op1_pt *p,unsigned n,int32_t y,int32_t weight,const uint8_t *colour){
 int32_t out[96];unsigned count=band_spans(p,n,y,(weight+1)/2,out);for(unsigned j=1;j<count;j+=2)line(r,out[j-1],y,out[j],y,weight,colour);
}
static int bottle_contains(const bottle_cache_t *c,int row,int32_t x,int32_t margin){
 if(row<c->from||row>=c->to)return 0;for(unsigned j=1;j<c->count[row];j+=2)if(x>=c->x[row][j-1]+margin&&x<=c->x[row][j]-margin)return 1;return 0;
}
static void fill(void *r,const op1_pt *p,unsigned n,int32_t from_y,const uint8_t *c){
 int32_t y,min=Q(160),max=0,x[96];unsigned i,count;
 for(i=0;i<n;++i){if(p[i].y<min)min=p[i].y;if(p[i].y>max)max=p[i].y;}
 if(from_y>min)min=from_y;if(min<0)min=0;if(max>Q(160))max=Q(160);
 for(y=((min+1023)>>10)*1024+512;y<max;y+=1024){
  count=intersections(p,n,y,x);
  for(i=1;i<count;i+=2)line(r,x[i-1],y,x[i],y,1024,c);
 }
}
static void blend(uint8_t out[4],const uint8_t *a,const uint8_t *b,unsigned t,unsigned scale){
 unsigned i;for(i=0;i<3;++i)out[i]=(uint8_t)(((uint32_t)a[i]*(scale-t)+(uint32_t)b[i]*t+scale/2)/scale);out[3]=255;
}
static void text(void *r,const char *s,int32_t x,int32_t y,int32_t h,const uint8_t *c,int align,int32_t w,int32_t max,const int32_t *m){
 uint8_t faded[4];if(text_alpha<32768){blend(faded,bg,c,text_alpha,32768);c=faded;}
 if(m&&(draw_dx||draw_dy)){int32_t translated[9];for(unsigned i=0;i<9;i++)translated[i]=m[i];translated[6]+=draw_dx;translated[7]+=draw_dy;gin_native_draw_text(r,s,x,y,h,c,align,w,max,translated);}
 else gin_native_draw_text(r,s,x+draw_dx,y+draw_dy,h,c,align,w,max,m);
}


static void bottle_matrix(unsigned which,unsigned progress,int32_t out[9]){
 unsigned index=progress>>10,fraction=progress&1023;
 for(unsigned j=0;j<6;j++){
  unsigned k=j<2?j:j<4?j+1:j+2;
  out[k]=pour_matrix[which][index][j]+(index<32?((pour_matrix[which][index+1][j]-pour_matrix[which][index][j])*(int32_t)fraction)/1024:0);
 }
 out[2]=out[5]=0;out[8]=1024;
}
static void bottle_shake(unsigned which,const int32_t *m){
 const op1_pt *p=which?b_body:a_body;unsigned n=which?COUNT(b_body):COUNT(a_body);
 int32_t lo=Q(320),hi=0;
 draw_dx=(motion_sine[((motion.last_tick*420u)/1024u+which*16)&63]*motion.shake*3)>>17;
 draw_dy=(motion_sine[((motion.last_tick*420u)/1024u+which*16+16)&63]*motion.shake)>>17;
 for(unsigned j=0;j<n;j++){int32_t x=(int32_t)(((int64_t)m[0]*p[j].x+(int64_t)m[3]*p[j].y)>>10)+m[6];if(x<lo)lo=x;if(x>hi)hi=x;}
 if(lo+draw_dx<Q(65))draw_dx=Q(65)-lo;
 if(hi+draw_dx>Q(255))draw_dx=Q(255)-hi;
}
static void bottle(void *r,int which,int active,unsigned progress,const char *gin_label){
 const op1_pt *body=which?b_body:a_body,*mouth=which?b_mouth:a_mouth,*brand=which?b_brand_line:a_brand_line;
 unsigned n=which?COUNT(b_body):COUNT(a_body),mn=which?COUNT(b_mouth):COUNT(a_mouth);
 int32_t absolute[9];bottle_matrix((unsigned)which,progress,absolute);bottle_shake((unsigned)which,absolute);
 const uint8_t *colour=which?orange:blue,*outline=active?white:dim;
 const int32_t *matrix=which?b_matrix:a_matrix;int32_t y=which?b_surface_y:a_surface_y,px=which?Q(218):Q(90);uint8_t liquid[4];
 if(progress){
  int32_t transform[6];unsigned index=progress>>10,fraction=progress&1023;const int32_t *m=transform;int32_t low=Q(160),high=0;
  for(unsigned j=0;j<6;j++)transform[j]=pour_matrix[which][index][j]+(index<32?((pour_matrix[which][index+1][j]-pour_matrix[which][index][j])*(int32_t)fraction)/1024:0);
  for(unsigned j=0;j<n;j++){moving_body[j]=move_point(body[j],m);}
  for(unsigned j=0;j<mn;j++)moving_mouth[j]=move_point(mouth[j],m);
  for(unsigned j=0;j<2;j++)moving_brand[j]=move_point(brand[j],m);
  for(unsigned j=0;j<2;j++){moving_matrix[j*3]=(int32_t)(((int64_t)m[0]*matrix[j*3]+(int64_t)m[2]*matrix[j*3+1])>>10);moving_matrix[j*3+1]=(int32_t)(((int64_t)m[1]*matrix[j*3]+(int64_t)m[3]*matrix[j*3+1])>>10);moving_matrix[j*3+2]=0;}
  {op1_pt t={matrix[6],matrix[7]};t=move_point(t,m);moving_matrix[6]=t.x;moving_matrix[7]=t.y;moving_matrix[8]=1024;}
  for(unsigned j=0;j<4;j++){op1_pt c=move_point(liquid_corners[which][j],m);if(c.y<low)low=c.y;if(c.y>high)high=c.y;}
  body=moving_body;mouth=moving_mouth;brand=moving_brand;matrix=moving_matrix;y=low+(high-low)*48/100;
 }
 mouth_x[which]=draw_dx;mouth_y[which]=draw_dy;for(unsigned j=0;j<mn;j++){mouth_x[which]+=mouth[j].x/(int32_t)mn;mouth_y[which]+=mouth[j].y/(int32_t)mn;}
 if(span_pose[which]!=progress){prepare_bottle_spans(&bottle_spans[which],body,n);prepare_spans(&mouth_spans[which],mouth,mn);span_pose[which]=progress;}
 blend(liquid,face,colour,18,100);paint_bottle(r,&bottle_spans[which],y,liquid);
 bottle_surface(r,body,n,y,Q(1.05),colour);
 if(which){const bottle_cache_t *c=&bottle_spans[which];int from=(y>>10)+2,to=c->to-2,height=to-from;
  if(from<c->from)from=c->from;height=to-from;
  if(height>2)for(unsigned i=0;i<3;i++){
   int32_t cy=to*1024-(int32_t)((motion.last_tick*12u+ i*Q(7))%(uint32_t)(height*1024)),cx;int row=cy>>10;
   int32_t left=0,right=0;for(unsigned k=1;k<c->count[row];k+=2)if(c->x[row][k]-c->x[row][k-1]>right-left){left=c->x[row][k-1];right=c->x[row][k];}if(right-left<Q(3))continue;cx=left+(right-left)*(int32_t)(i+1)/4;


   int safe=1;for(int rr=(cy-Q(1.1))>>10;rr<=((cy+Q(1.1))>>10);rr++)if(!bottle_contains(c,rr,cx,Q(1.15))){safe=0;break;}if(!safe)continue;
   op1_pt previous={cx+Q(.8),cy};
   for(unsigned j=1;j<=12;j++){unsigned phase=j*64/12;op1_pt next={cx+((Q(.8)*motion_sine[(phase+16)&63])>>10),cy+((Q(.8)*motion_sine[phase&63])>>10)};
    int r0=previous.y>>10,r1=next.y>>10;if(bottle_contains(c,r0,previous.x,Q(.35))&&bottle_contains(c,r1,next.x,Q(.35)))line(r,previous.x,previous.y,next.x,next.y,Q(.6),orange);previous=next;
   }
  }
 }
 if(!native_vessels){stroke(r,body,n,1,Q(1.15),outline);
 paint_spans(r,&mouth_spans[which],0,bg,bg);stroke(r,mouth,mn,1,Q(1),outline);stroke(r,brand,2,0,Q(.75),outline);}
 text(r,which?"TONIC":gin_label,px,which?Q(69):Q(63),which?Q(7.25):Q(8.5),outline,0,Q(1),0,matrix);
 text(r,which?"B":"A",px,which?Q(83):Q(84),Q(11),outline,0,Q(1),0,matrix);
}


static const int16_t circle_unit[16][2]={{1024,0},{946,392},{724,724},{392,946},{0,1024},{-392,946},{-724,724},{-946,392},{-1024,0},{-946,-392},{-724,-724},{-392,-946},{0,-1024},{392,-946},{724,-724},{946,-392}};
static int inside(const op1_pt *p,unsigned n,int32_t x,int32_t y){
 int row=y>>10;(void)p;(void)n;
 return row>=glass_spans.from&&row<glass_spans.to&&x>=glass_spans.x[row][0]&&x<=glass_spans.x[row][1];
}
static void circle(void *r,int32_t cx,int32_t cy,int32_t radius,int32_t w,const uint8_t *c,int clipped){
 unsigned i;op1_pt a,b;
 for(i=0;i<16;++i){a.x=cx+((radius*circle_unit[i][0])>>10);a.y=cy+((radius*circle_unit[i][1])>>10);b.x=cx+((radius*circle_unit[(i+1)&15][0])>>10);b.y=cy+((radius*circle_unit[(i+1)&15][1])>>10);
  if(!clipped||(inside(glass_body,COUNT(glass_body),a.x,a.y)&&inside(glass_body,COUNT(glass_body),b.x,b.y)))line(r,a.x,a.y,b.x,b.y,w,c);
 }
}
static int32_t float_q15(float v){union{float f;uint32_t u;}b;b.f=v;return op1_f01_to_q15(b.u);}
static int32_t visual_sine(int32_t phase){unsigned i=(unsigned)(phase>>10)&63;return (motion_sine[i]+(motion_sine[(i+1)&63]-motion_sine[i])*(phase&1023)/1024)*32;}
static int32_t visual_tanh(int32_t value){int negative=value<0;if(negative)value=-value;if(value>=131072)return negative?-motion_tanh[128]:motion_tanh[128];unsigned index=(unsigned)value>>10;int32_t v=motion_tanh[index]+(motion_tanh[index+1]-motion_tanh[index])*(value&1023)/1024;return negative?-v:v;}



static void prepare_wave(const gin_ui_snapshot_t *v){
 int32_t p[2][4];int changed=wave_id!=v->id||wave_morph!=v->morph;
 for(unsigned i=0;i<2;i++){p[i][0]=float_q15(v->endpoint[i].OscFreq);p[i][1]=float_q15(v->endpoint[i].ModAmt);p[i][2]=float_q15(v->endpoint[i].Mix);p[i][3]=float_q15(v->endpoint[i].DistAmt);for(unsigned j=0;j<4;j++)if(p[i][j]!=wave_params[i][j])changed=1;}
 if(!changed)return;wave_id=v->id;wave_morph=v->morph;
 for(unsigned i=0;i<2;i++)for(unsigned j=0;j<4;j++)wave_params[i][j]=p[i][j];
 for(unsigned j=0;j<=64;j++){
  int32_t x=Q(125)+Q(71)*(int32_t)j/64,t=(x-Q(132))*32/57,shape[2];
  for(unsigned i=0;i<2;i++){
   int32_t cycles=4915+(int32_t)(((int64_t)p[i][0]*245760)>>15),phase=t+(((2*p[i][1]-32768)*visual_sine(t)>>15)*2949>>15);
   int32_t amplitude=(int32_t)(((int64_t)p[i][0]*(32768-((3*p[i][0])>>2)))>>15);
   int32_t value=(int32_t)(((int64_t)visual_sine((int32_t)(((int64_t)phase*cycles)>>14))*amplitude)>>15);value+=((p[i][2]*4588)>>15)*visual_sine(t*34)>>15;
   int32_t gain=32768+2*p[i][3];shape[i]=(int32_t)((int64_t)visual_tanh((int32_t)(((int64_t)value*gain)>>15))*32768/visual_tanh(gain));
  }
  liquid_wave[j]=-(int32_t)(((int64_t)(shape[0]*(int32_t)(1000-v->morph)+shape[1]*(int32_t)v->morph)*Q(4))/32768000);
 }
}
static int32_t liquid_y(int32_t x,uint32_t now){
 int32_t phase=(x-Q(132))*96/57-(int32_t)(now*104u);unsigned i=(unsigned)(phase>>10)&63;int32_t fraction=phase&1023;
 int32_t sine=motion_sine[i]+(motion_sine[(i+1)&63]-motion_sine[i])*fraction/1024;
 int32_t position=(x-Q(125))*64/71;if(position<0)position=0;if(position>65536)position=65536;unsigned index=(unsigned)position>>10;int32_t base=liquid_wave[index];if(index<64)base+=(liquid_wave[index+1]-base)*(position&1023)/1024;
 return Q(109)+base-((sine*motion.ripple*3)>>15);
}
static uint32_t duration_ms(float v){
 union{float f;uint32_t u;}b;b.f=v;int e=(int)((b.u>>23)&255)-127;uint32_t mantissa=(b.u&0x7fffffu)|0x800000u;
 if(e<0)return 0;if(e>13)return 10000;return mantissa>>(23-e);
}

static uint32_t settle_ticks(const gin_ui_snapshot_t *v){
 static const uint16_t ms[]={10,20,50,100,200,500,1000,2000,5000,10000};
 static const uint16_t tau[]={280,626,1084,1430,1776,2234,2580,2926,3384,3730};
 uint32_t tone=(duration_ms(v->decay_ms[0][0])*(1000-v->morph)+duration_ms(v->decay_ms[1][0])*v->morph)/1000;
 uint32_t noise=(duration_ms(v->decay_ms[0][1])*(1000-v->morph)+duration_ms(v->decay_ms[1][1])*v->morph)/1000;
 uint32_t d=tone>noise?tone:noise;if(d<=10)return 280;
 for(unsigned i=1;i<COUNT(ms);i++)if(d<=ms[i])return tau[i-1]+((d-ms[i-1])*(tau[i]-tau[i-1]))/(ms[i]-ms[i-1]);return 3730;
}
static void glass_liquid(void *r,uint32_t now,const uint8_t *colour,const uint8_t *liquid){
 op1_pt wave[67];int32_t min=Q(109),max=Q(109),x[96];unsigned count;

 for(unsigned j=0;j<=64;j++){wave[j].x=Q(125)+(Q(71)*(int32_t)j)/64;wave[j].y=liquid_y(wave[j].x,now);if(wave[j].y<min)min=wave[j].y;if(wave[j].y>max)max=wave[j].y;}
 wave[65]=(op1_pt){Q(196),Q(141)};wave[66]=(op1_pt){Q(125),Q(141)};
 for(unsigned pass=0;pass<2;pass++)for(int row=glass_fill_spans.from;row<glass_fill_spans.to;row++){
  int32_t y=row*1024+512,l=glass_fill_spans.x[row][0],h=glass_fill_spans.x[row][1];if(h<=l)continue;
  if(!pass)line(r,l,y,h,y,LIQUID_STROKE,face);
  else if(y>=max)line(r,l,y,h,y,LIQUID_STROKE,liquid);
  else if(y>=min){count=intersections(wave,67,y,x);for(unsigned j=1;j<count;j+=2){int32_t a=x[j-1]>l?x[j-1]:l,b=x[j]<h?x[j]:h;if(b>a)line(r,a,y,b,y,LIQUID_STROKE,liquid);}}
 }
 for(unsigned j=1;j<=64;j++)if(inside(glass_body,COUNT(glass_body),wave[j-1].x,wave[j-1].y)&&inside(glass_body,COUNT(glass_body),wave[j].x,wave[j].y))line(r,wave[j-1].x,wave[j-1].y,wave[j].x,wave[j].y,Q(1.35),colour);
}

static void glass_rim(void *r){
 static const uint8_t purple[4]={177,107,255,255};uint8_t colour[4];
 blend(colour,white,purple,(unsigned)motion.pulse,32768);
 stroke(r,glass_front_rim,COUNT(glass_front_rim),0,Q(1.35)+(motion.pulse*Q(.6)>>15),colour);
}
static int32_t lerp(int32_t a,int32_t b,int32_t t){return a+(b-a)*t/1024;}
static op1_pt bezier(op1_pt a,op1_pt b,op1_pt c,op1_pt d,int32_t t){
 op1_pt ab={lerp(a.x,b.x,t),lerp(a.y,b.y,t)},bc={lerp(b.x,c.x,t),lerp(b.y,c.y,t)},cd={lerp(c.x,d.x,t),lerp(c.y,d.y,t)};
 op1_pt abc={lerp(ab.x,bc.x,t),lerp(ab.y,bc.y,t)},bcd={lerp(bc.x,cd.x,t),lerp(bc.y,cd.y,t)};
 op1_pt q={lerp(abc.x,bcd.x,t),lerp(abc.y,bcd.y,t)};return q;
}


static void fish(void *r,uint32_t now){
 static const op1_pt body[]={{Q(155),Q(120)},{Q(158),Q(118)},{Q(162),Q(118)},{Q(164),Q(120)},{Q(162),Q(122)},{Q(158),Q(122)}};
 static const op1_pt tail[]={{Q(155),Q(120)},{Q(152),Q(117)},{Q(152),Q(123)}};
 int32_t old=draw_dy;draw_dy+=(motion_sine[((now*42u)/1024u)&63]*motion.ripple)>>15;
 stroke(r,body,COUNT(body),1,Q(.75),green);stroke(r,tail,COUNT(tail),1,Q(.75),green);
 line(r,Q(162),Q(119.5),Q(162.5),Q(119.5),Q(.75),green);draw_dy=old;
}
static void scene(void *r,const gin_ui_snapshot_t *v,uint32_t now){
 uint8_t colour[4],liquid[4];int32_t noise;unsigned i,bubbles,m=v->morph;
 uint32_t hit=gin_native_hit_count();
 if(!clip_ready){prepare_glass_spans();clip_ready=1;}
 prepare_wave(v);gin_motion_step(&motion,v->id,v->morph,v->side,now,hit,settle_ticks(v));
 if(visual_revision!=v->revision&&(v->readout[0].active||v->readout[1].active||v->readout[2].active||v->readout[3].active))gin_motion_interaction(&motion);visual_revision=v->revision;
 {const char *brand=v->decoration==GIN_UI_DECOR_LINN?"LINN":"GIN";
 if(v->side==GIN_UI_A){bottle(r,1,0,(unsigned)motion.progress[1],brand);bottle(r,0,1,(unsigned)motion.progress[0],brand);}else{bottle(r,0,0,(unsigned)motion.progress[0],brand);bottle(r,1,1,(unsigned)motion.progress[1],brand);}}
 gin_scene_observation[0]=(uint32_t)motion.progress[0];gin_scene_observation[1]=(uint32_t)motion.progress[1];gin_scene_observation[2]=motion.flags;gin_scene_observation[3]=(uint32_t)motion.ripple;gin_scene_observation[4]=now;gin_scene_observation[5]=hit;gin_scene_observation[6]=v->id;++gin_scene_observation[7];
 draw_dx=(motion_sine[((now*420u)/1024u)&63]*motion.shake)>>15;draw_dy=(motion_sine[((now*420u)/1024u+16)&63]*motion.shake)>>16;
 int32_t glass_dx=draw_dx,glass_dy=draw_dy;gin_scene_observation[8]=(uint32_t)motion.shake;gin_scene_observation[9]=(uint32_t)draw_dx;gin_scene_observation[10]=(uint32_t)draw_dy;
 if(m<=500){static const uint8_t mid[4]={177,107,255,255};blend(colour,blue,mid,m,500);}else{static const uint8_t mid[4]={177,107,255,255};blend(colour,mid,orange,m-500,500);}
 blend(liquid,face,colour,17,100);glass_liquid(r,now,colour,liquid);
 noise=(float_q15(v->endpoint[0].Mix)*(int32_t)(1000-m)+float_q15(v->endpoint[1].Mix)*(int32_t)m)/1000;
 bubbles=(unsigned)(noise*7+16383)/32767+1;if(bubbles>8)bubbles=8;
 for(i=0;i<bubbles;++i)circle(r,Q(143+(i*11)%35),Q(120+(i*5)%9)+((motion_sine[((now*42u)/1024u+i*10)&63]*motion.ripple)>>15),Q(1),Q(.75),green,1);
 if(v->decoration==GIN_UI_DECOR_FISH)fish(r,now);
 stroke(r,glass_body,COUNT(glass_body),0,Q(1.35),white);
 fill(r,glass_foot,COUNT(glass_foot),0,bg);stroke(r,glass_stem,2,0,Q(2),white);stroke(r,glass_foot,COUNT(glass_foot),1,Q(1.05),white);
 stroke(r,glass_back_rim,COUNT(glass_back_rim),0,Q(1.35),white);
 draw_dx=draw_dy=0;
 for(i=0;i<2;i++)if((motion.flags&(1u<<i))&&(i?m>0:m<1000)&&mouth_x[i]>=Q(136)+glass_dx&&mouth_x[i]<=Q(185)+glass_dx&&mouth_y[i]<=Q(78)+glass_dy){
  int32_t x=mouth_x[i],y=mouth_y[i]+Q(1),target=i?(x-Q(7)>Q(163)+glass_dx?x-Q(7):Q(163)+glass_dx):(x+Q(9)<Q(156)+glass_dx?x+Q(9):Q(156)+glass_dx);
  int32_t direction=i?-Q(8):Q(9),weight=Q(.65)+(Q(1.7)*(int32_t)(i?m:1000-m))/1000;const uint8_t *c=i?orange:blue;op1_pt last={x,y};
  int32_t landing=liquid_y(target-glass_dx,now)+glass_dy,arc=0;op1_pt a={x,y},b={x+direction,y+Q(2)},c1={target,landing-Q(12)},d={target,landing};
  for(unsigned j=1;j<=24;j++){op1_pt next=bezier(a,b,c1,d,(int32_t)j*1024/24);int32_t dx=next.x-last.x,dy=next.y-last.y;if(dx<0)dx=-dx;if(dy<0)dy=-dy;arc+=dx>dy?dx+dy/2:dy+dx/2;
   if((uint32_t)(arc+now*26u)%Q(4.5)<Q(2.5))line(r,last.x,last.y,next.x,next.y,weight,c);last=next;
  }
 }
 draw_dx=glass_dx;draw_dy=glass_dy;glass_rim(r);draw_dx=draw_dy=0;

}


static void controls(void *r,const gin_ui_snapshot_t *v,uint32_t now){
 (void)now;const uint8_t *colours[4]={blue,green,white,orange};
 const char *labels[2][4]={{"MORPH","PITCH","DECAY","BALANCE"},{"MORPH","MOD","NOISE","CUTOFF"}};
 const po32_patch_params_t *p=&v->endpoint[v->side];
 float raw[4]={v->morph*.001f,v->shift?p->ModAmt:p->OscFreq,v->shift?p->NEnvDcy:p->OscDcy,v->shift?p->NFilFrq:p->Mix};
 for(unsigned i=0;i<4;i++){
  int right=i>=2;int32_t x=Q(right?312:8),y=Q(i==0||i==2?8:81);char number[8];
  text(r,labels[v->shift!=0][i],x,y,Q(11),colours[i],right?1:-1,Q(1),0,0);
  snprintf(number,sizeof(number),"%.0f",raw[i]*100.f);text(r,number,x,y+Q(17),Q(28),white,right?1:-1,Q(1),0,0);
 }
 gin_scene_observation[11]=32768;
}



static int32_t diagram_shape(unsigned page,unsigned j,const po32_patch_params_t *p,const gin_ui_snapshot_t *v){
 int32_t a,b,z;unsigned mode;
 switch(page){
 case 0:
  mode=(unsigned)(float_q15(p->OscWave)*3u/32768u);if(mode>2)mode=2;
  if(!mode)return visual_sine((int32_t)j*4096);
  z=(int32_t)(j&15u)*4096;
  if(mode==1)return z<32768?32768-z*2:z*2-98304;
  return z-32768;
 case 1:case 4:{
  float attack=v->readout[page==1?1:2].primary,decay=v->readout[page==1?2:3].primary;
  float time=(attack+decay)*(float)j/32.f,env;
  mode=page==4?(p->NEnvMod<1.f/3?0:p->NEnvMod<2.f/3?1:2):0;
  if(time<attack)env=mode==2?1.f:mode==1?time/attack:gin_ui_curve_power(.001f,1.f-time/attack);
  else env=decay>0?(mode==1?1.f-(time-attack)/decay:gin_ui_curve_power(.001f,(time-attack)/decay)):0;
  if(mode==2){
   float period=p->NEnvDcy>0.0001f?p->NEnvDcy/44100.f*gin_ui_curve_power(10000.f,p->NEnvDcy):0;
   float gain=1;if(period>0){if(period<.0015f){period*=8;gain=gin_ui_curve_power(.5f/period,.5f);}else period*=2;}
   if(time>=attack&&period>0)env*=((float)visual_sine((int32_t)((.25f+(time-attack)*.001f/period-(int32_t)((time-attack)*.001f/period))*65536.f))/32768.f)*gain;
   if(gain>1)env/=gain;
  }
  return (int32_t)(env*32768.f);}
 case 2:
  mode=p->ModMode<1.f/3?0:p->ModMode<2.f/3?1:2;
  a=float_q15(p->ModAmt)*2-32768;b=a<0?-a:a;a=a*b/32768;if(mode)a/=2;

  if(!mode)return (int32_t)(a*(p->ModRate>0?gin_ui_curve_power(.001f,(float)j/32.f):1));
  b=1+float_q15(p->ModRate)/8192;
  if(mode==1)z=visual_sine((int32_t)j*2048*b);
  else{unsigned k=j*(unsigned)b/2u;z=(int32_t)((k*1103515245u+12345u)>>16&65535u)-32768;}
  return z*a/32768;
 case 3:
  mode=(unsigned)(float_q15(p->NFilMod)*3u/32768u);if(mode>2)mode=2;
  a=4+float_q15(p->NFilFrq)*24/32768;b=(int32_t)j-a;if(b<0)b=-b;
  z=32768/(1+b*b*(1+float_q15(p->NFilQ)*7/32768)/3);
  if(mode==1)return z;
  z=(int32_t)j<a?24576:8192/(1+b*b/3);
  if(mode==2)z=24576-z;
  return z+(float_q15(p->NFilQ)/4)/(1+b*b);
 case 6:
  a=4+float_q15(p->EQFreq)*24/32768;b=(int32_t)j-a;
  return (float_q15(p->EQGain)*2-32768)/(1+b*b/5);
 default:return 0;
 }
}


static void ring_diagram(void *r,const gin_ui_snapshot_t *v){
 const po32_patch_params_t *p=&v->endpoint[v->side];
 float hold=v->readout[2].primary,decay=v->readout[3].primary,total=hold+decay;
 float period=p->NEnvDcy>0.0001f?p->NEnvDcy/44100.f*gin_ui_curve_power(10000.f,p->NEnvDcy):0,gain=1;
 if(period>0){if(period<.0015f){period*=8;gain=gin_ui_curve_power(.5f/period,.5f);}else period*=2;}
 float scale=gain>1?gain:1,span=total>0?216.f*decay/total:0,start=304.f-span;
 int band=period>0&&216.f*period*1000.f/total<8.f;char label[20];

 if(period>0&&v->readout[3].active&&!v->message[0]){snprintf(label,sizeof(label),"%.0f HZ",1.f/period);text(r,label,Q(8),Q(53),Q(10),orange,-1,Q(.9),0,0);}

 const int32_t baseline=Q(49),height=Q(22);int32_t xstart=(int32_t)(start*1024.f);
 if(band&&span>=1.8f){
  static const uint8_t shade[4]={70,20,17,255};unsigned count=(unsigned)span+1;
  float step=count>1?(span-1.8f)/(count-1):0,env=gain/scale*gin_ui_curve_power(.001f,1.8f/span),coefficient=gin_ui_curve_power(.001f,step/span);
  for(unsigned j=0;j<count;j++){
   int32_t x=(int32_t)((start+.9f+step*j)*1024.f),h=(int32_t)(env*height);
   if(h>0)line(r,x,baseline-h,x,baseline+h,Q(1.8),shade);env*=coefficient;
  }
 }
 line(r,Q(88),baseline,Q(304),baseline,Q(.8),dim);

 line(r,Q(88),baseline-(int32_t)(height/scale),xstart,baseline-(int32_t)(height/scale),Q(1.8),white);
 unsigned count=band?32u:(unsigned)span+1;if(count>216)count=216;if(!count)count=1;
 float env=gain/scale,coefficient=gin_ui_curve_power(.001f,1.f/count),phase=0,step=period>0?decay*.001f/(period*count):0;
 op1_pt previous={xstart,baseline-(int32_t)(height*env)},lower={xstart,baseline+(int32_t)(height*env)};
 for(unsigned j=1;j<=count;j++){
  env*=coefficient;phase+=step;phase-=(int32_t)phase;
  float carrier=band||period<=0?1.f:(float)visual_sine((int32_t)((phase+.25f)*65536.f))/32768.f;
  op1_pt next={xstart+(Q(304)-xstart)*(int32_t)j/(int32_t)count,baseline-(int32_t)(height*env*carrier)};
  line(r,previous.x,previous.y,next.x,next.y,Q(1.8),orange);previous=next;
  if(band){next.y=baseline+(int32_t)(height*env);line(r,lower.x,lower.y,next.x,next.y,Q(1.8),orange);lower=next;}
 }
}
static void editor_diagram(void *r,const gin_ui_snapshot_t *v){
 const po32_patch_params_t *p=&v->endpoint[v->side];unsigned page=v->page;
 if(v->shift)return;
 if(page==4&&p->NEnvMod>=2.f/3){ring_diagram(r,v);return;}
 if(page==5||page==7){
  const float values[3]={page==5?p->Mix:p->OscVel,page==5?p->DistAmt:p->NVel,page==5?p->Level:p->ModVel};
  const uint8_t *colours[3]={green,white,orange};
  for(unsigned i=0;i<3;i++){int32_t y=Q(29+19*i),x=Q(88)+(float_q15(values[i])*216)/32;
   line(r,Q(88),y,Q(304),y,Q(.8),dim);line(r,Q(88),y,x,y,Q(2.4),colours[i]);line(r,x,y-Q(3),x,y+Q(3),Q(2.4),colours[i]);}
  return;
 }
 const uint8_t *colour=page==0?green:white;
 int envelope=page==1||page==4,bipolar=page==0||page==2||page==6||(page==4&&p->NEnvMod>=2.f/3);
 int32_t baseline=bipolar?Q(49):Q(72),height=bipolar?Q(22):Q(44);
 line(r,Q(88),baseline,Q(304),baseline,Q(.8),dim);
 unsigned attack=0,decay=0,knot=0;
 if(envelope){attack=duration_ms(v->readout[page==1?1:2].primary);decay=duration_ms(v->readout[page==1?2:3].primary);knot=attack+decay?attack*32u/(attack+decay):0;}
 op1_pt prev={Q(88),baseline-diagram_shape(page,0,p,v)*height/32768};
 for(unsigned j=1;j<=32;j++){op1_pt next={Q(88)+Q(216)*(int32_t)j/32,baseline-diagram_shape(page,j,p,v)*height/32768};const uint8_t *c=envelope?(j<=knot?(page==1?green:white):(page==1?white:orange)):colour;line(r,prev.x,prev.y,next.x,next.y,Q(1.8),c);prev=next;}
 if(page==0||page==3||page==6||(page==2&&p->ModMode>=1.f/3)){
  float value=page==0?p->OscFreq:page==2?p->ModRate:page==3?p->NFilFrq:p->EQFreq;
  int32_t x=Q(88)+(float_q15(value)*216)/32;const uint8_t *c=page==2?orange:page==6?green:white;
  line(r,x,Q(42),x,Q(63),Q(2.4),c);
  if(page==3){int32_t y=Q(61)-(float_q15(p->NFilQ)*17)/32;line(r,x-Q(3),y,x+Q(3),y,Q(2.4),orange);}
 }

}
static const char *editor_label(const char *s){
 if(!strcmp(s,"TONE ATK")||!strcmp(s,"NOISE ATK"))return "ATTACK";
 if(!strcmp(s,"TONE DECAY")||!strcmp(s,"NOISE DECAY"))return "DECAY";
 if(!strcmp(s,"MOD AMOUNT"))return "AMOUNT";
 if(!strcmp(s,"RESONANCE"))return "RESO";
 if(!strcmp(s,"ENV SHAPE"))return "SHAPE";
 if(!strcmp(s,"SWEEP TIME"))return "TIME";
 if(!strcmp(s,"SINE RATE")||!strcmp(s,"NOISE RATE"))return "RATE";
 return s;
}
static void key_footer(void *r,const gin_ui_snapshot_t *v){
 const char *const labels[4][4]={{"EDIT","ENV","FX","LFO"},{"EXIT","PREV","NEXT","NAME"},{"OK","","","CANCEL"},{"BACK","","",""}};
 if(v->shift)return;

 for(unsigned i=0;i<4;i++){if((v->mode==0&&i>0)||(v->mode==1&&v->page==GIN_UI_ABOUT_PAGE&&i>1))continue;const uint8_t *c=white;
  if(v->mode==1&&((i==1&&!v->page)||(i==2&&v->page==GIN_UI_ABOUT_PAGE)))c=dim;
  text(r,labels[v->mode][i],Q(40+80*i),Q(132),Q(12),c,0,Q(1.1),0,0);
 }
 if(v->mode==1){char page[12];snprintf(page,sizeof(page),"%u/%u",v->page+1,(unsigned)GIN_UI_VIEW_COUNT);text(r,page,Q(160),Q(133),Q(11),dim,0,Q(.85),0,0);}
}
static unsigned readout_length(const char *s){unsigned n=0;while(n<23&&s[n])++n;return n;}


static const char *editor_value(const char *value,char compact[24],int *scientific){
 unsigned i=0,digits=0;int sign=1,decimal=0;float magnitude=0,fraction=.1f;
 *scientific=0;
 if(value[i]=='-'||value[i]=='+'){if(value[i]=='-')sign=-1;++i;}
 for(;i<23&&value[i];i++){
  if(value[i]=='.'&&!decimal){decimal=1;continue;}
  if(value[i]<'0'||value[i]>'9')return value;
  ++digits;if(decimal){magnitude+=(value[i]-'0')*fraction;fraction*=.1f;}else magnitude=magnitude*10+(value[i]-'0');
 }
 if(!digits||magnitude<1000)return value;
 int notation=magnitude>999999;
 if(!notation){snprintf(compact,24,value[0]=='+'?"%+.0f":"%.0f",sign*magnitude);notation=gin_native_text_cursor(compact,readout_length(compact),Q(18),0)>Q(72);}
 if(notation){
  unsigned exponent=0;while(magnitude>=10){magnitude*=.1f;++exponent;}
  if(magnitude>=9.95f){magnitude*=.1f;++exponent;}
  snprintf(compact,24,"%s%.1fE%u",sign<0?"-":value[0]=='+'?"+":"",magnitude,exponent);*scientific=1;
 }
 return compact;
}
static void editor(void *r,const gin_ui_snapshot_t *v){
 const uint8_t *colours[4]={blue,green,white,orange};char header[40];
 unsigned name_len=0;while(name_len<23&&v->name[name_len]){header[name_len]=v->name[name_len];++name_len;}header[name_len]=0;
 while(name_len&&gin_native_text_cursor(header,name_len,Q(12),0)>Q(160))header[--name_len]=0;
 text(r,header,Q(8),Q(4),Q(12),white,-1,Q(.85),0,0);
 text(r,v->side?"EDIT B":"EDIT A",Q(8),Q(22),Q(11),white,-1,Q(.85),0,0);
 text(r,v->shift?"SOUND SELECT":gin_ui_page_titles[v->page],Q(312),Q(4),Q(14),white,1,Q(.9),0,0);
 editor_diagram(r,v);
 for(unsigned i=0;i<4;i++){
  int32_t x=Q(8+80*i);const gin_ui_readout_t *a=&v->readout[i];
  if(!strcmp(a->label,"-"))continue;
  const char *lab=editor_label(a->label);if(!v->shift&&i&&v->page==7){lab=i==1?"TONE":i==2?"NOISE":"MOD";}else if(!v->shift&&i&&v->page==6){lab=i==1?"FREQ":"GAIN";}
  text(r,lab,x,Q(80),Q(11),colours[i],-1,Q(.9),0,0);
  const char *value=!strcmp(a->value,"TRIANGLE")?"TRI":!strcmp(a->value,"PRESS")?"CLICK":!strcmp(a->value,"INF")?"HOLD":a->value,*unit=a->unit;
  if(!strcmp(value,"HOLD"))unit="";
  if(!strcmp(unit,"%"))unit="";else if(!strcmp(unit,"% SENS"))unit="%";
  char compact[24];int scientific;value=editor_value(value,compact,&scientific);
  text(r,value,x,Q(97),Q(18),colours[i],-1,Q(1),0,0);
  int32_t width=gin_native_text_cursor(value,readout_length(value),Q(18),0);
  int32_t uw=gin_native_text_cursor(unit,readout_length(unit),Q(10),0);
  if(!scientific&&width+Q(3)+uw<=Q(72))text(r,unit,x+width+Q(3),Q(105),Q(10),colours[i],-1,Q(.8),0,0);
  else text(r,unit,x,Q(118),Q(11),colours[i],-1,Q(.8),0,0);
 }
 key_footer(r,v);
 if(v->message[0]){
  unsigned at=0;for(unsigned row=0;row<2&&v->message[at];row++){
   char status[24];unsigned n=0,space=0;
   while(n<23&&v->message[at+n]){status[n]=v->message[at+n];if(status[n]==' ')space=n;status[++n]=0;if(gin_native_text_cursor(status,n,Q(10),0)>Q(72)){status[--n]=0;break;}}
   if(v->message[at+n]&&space)n=space;status[n]=0;if(!n)break;
   text(r,status,Q(8),Q(41+14*row),Q(10),white,-1,Q(.9),0,0);at+=n;while(v->message[at]==' ')++at;
  }
 }
}
static void help(void *r,const gin_ui_snapshot_t *v){
 text(r,"GIN",Q(160),Q(5),Q(18),white,0,Q(1),0,0);
 text(r,"PO-32 TONIC INSPIRED DRUM SYNTH",Q(160),Q(28),Q(11),dim,0,Q(.9),0,0);
 int32_t label=gin_native_text_cursor("SQUAREWAVE STUDIO",17,Q(13),0),by=gin_native_text_cursor("BY",2,Q(11),0),left=Q(160)-(by+Q(40)+label)/2;
 text(r,"BY",left,Q(51),Q(11),white,-1,Q(.9),0,0);left+=by+Q(8);
 for(unsigned i=0;i<COUNT(gin_credit_logo);i++){
  const uint8_t *a=gin_credit_logo[i];

  for(unsigned row=a[2];row<a[3];row++){int32_t y=Q(43)+row*512+256;line(r,left+a[0]*512,y,left+a[1]*512,y,Q(.55),white);}
 }
 text(r,"SQUAREWAVE STUDIO",left+Q(32),Q(49),Q(13),white,-1,Q(1),0,0);
 text(r,"WWW.SQUAREWAVE.STUDIO",Q(160),Q(70),Q(11),dim,0,Q(.9),0,0);
 text(r,"BASED ON LIBPO32 BY ERIC LEWIS",Q(160),Q(93),Q(12),white,0,Q(.9),0,0);
 text(r,"PRESETS BASED ON CNTR RNDM KITS",Q(160),Q(112),Q(11),white,0,Q(.9),0,0);
 key_footer(r,v);
}
static void naming(void *r,const gin_ui_snapshot_t *v){
 char header[24];unsigned len=0;
 while(len<23&&v->draft.text[len])++len;
 text(r,"NAME SOUND",Q(8),Q(6),Q(12),white,-1,Q(.85),Q(220),0);
 if(len>=20){snprintf(header,sizeof(header),"%u/23",len);text(r,header,Q(312),Q(6),Q(11),dim,1,Q(.85),0,0);}

 char window[24];unsigned first=0,last=len,cursor=v->draft.cursor;
 if(cursor>len)cursor=len;
 while(first<cursor&&gin_native_text_cursor(v->draft.text+first,cursor-first,Q(20),0)>Q(268))++first;
 while(last>first&&gin_native_text_cursor(v->draft.text+first,last-first,Q(20),0)>Q(288))--last;
 memcpy(window,v->draft.text+first,last-first);window[last-first]=0;
 text(r,window,Q(16),Q(58),Q(20),white,-1,Q(.9),0,0);
 int32_t caret=gin_native_text_cursor(window,cursor-first,Q(20),0);
 line(r,Q(16)+caret,Q(82),Q(16)+caret+Q(5),Q(82),Q(1),blue);
 text(r,"CURSOR",Q(8),Q(105),Q(11),blue,-1,Q(.8),0,0);
 text(r,"CHAR",Q(88),Q(105),Q(11),green,-1,Q(.8),0,0);
 text(r,v->draft.action==GIN_NAME_SPACE?"SPACE":"DELETE",Q(248),Q(92),Q(18),orange,-1,Q(.85),0,0);
 text(r,"CLICK",Q(248),Q(115),Q(11),orange,-1,Q(.75),0,0);
 if(v->message[0])text(r,v->message,Q(16),Q(31),Q(11),orange,-1,Q(.9),Q(288),0);
 key_footer(r,v);
}
#ifdef GIN_PROFILE_DIAGNOSTIC
static void diagnostics(void *r){
 gin_profile_t p;char s[80];gin_profile_snapshot(&p);
 text_alpha=32768;
#if GIN_RATE_2X && GIN_BLOCK_STATE
 snprintf(s,sizeof(s),"GIN 44K BLOCK / %u DSP",(unsigned)GIN_SYNTH_VOICE_LIMIT);
#elif GIN_RATE_2X
 snprintf(s,sizeof(s),"GIN 44K CORE / %u DSP",(unsigned)GIN_SYNTH_VOICE_LIMIT);
#elif GIN_STOCK_PROTECTED_CONTROL
 snprintf(s,sizeof(s),"DBOX CONTEXT / %u DSP",(unsigned)GIN_SYNTH_VOICE_LIMIT);
#elif GIN_SERVICE_TRACE
 snprintf(s,sizeof(s),"AUDIO SERVICE / %u DSP",(unsigned)GIN_SYNTH_VOICE_LIMIT);
#elif GIN_LEVEL_TRACE
#if defined(GIN_FW_OUTPUT_SHIFT) && GIN_FW_OUTPUT_SHIFT == 16
 snprintf(s,sizeof(s),"GIN LEVEL -6 / %u DSP",(unsigned)GIN_SYNTH_VOICE_LIMIT);
#else
 snprintf(s,sizeof(s),"GIN LEVEL / %u DSP",(unsigned)GIN_SYNTH_VOICE_LIMIT);
#endif
#elif defined(GIN_SINE_QUARTER) && GIN_SINE_QUARTER && defined(GIN_FW_OUTPUT_SHIFT) && GIN_FW_OUTPUT_SHIFT == 15
 snprintf(s,sizeof(s),"GIN QTR -6 / %u DSP",(unsigned)GIN_SYNTH_VOICE_LIMIT);
#elif defined(GIN_FW_OUTPUT_SHIFT) && GIN_FW_OUTPUT_SHIFT == 15
 snprintf(s,sizeof(s),"GIN TRACE -6 / %u DSP",(unsigned)GIN_SYNTH_VOICE_LIMIT);
#else
 snprintf(s,sizeof(s),"GIN TRACE / %u DSP",(unsigned)GIN_SYNTH_VOICE_LIMIT);
#endif
 text(r,s,Q(160),Q(6),Q(9),white,0,Q(.85),Q(300),0);
 if(p.period_min)snprintf(s,sizeof(s),"AUD %u%%  PEAK %u%%",gin_profile_percent(p.audio_cycles,p.elapsed_cycles),gin_profile_percent(p.audio_max,p.period_min));
 else snprintf(s,sizeof(s),"WAITING FOR AUDIO PERIOD");
 text(r,s,Q(8),Q(24),Q(9),white,-1,Q(.85),Q(304),0);
 snprintf(s,sizeof(s),"GIN %u%% MAX %uK",gin_profile_percent(p.gin_cycles,p.elapsed_cycles),p.gin_max/1000);
 text(r,s,Q(8),Q(40),Q(8),white,-1,Q(.8),Q(144),0);
 snprintf(s,sizeof(s),"DBOX %u%% MAX %uK",gin_profile_percent(p.stock_cycles,p.elapsed_cycles),p.stock_max/1000);
 text(r,s,Q(168),Q(40),Q(8),white,-1,Q(.8),Q(144),0);
 snprintf(s,sizeof(s),"NATIVE %u/12 PEAK %u  LONG %u",p.voices,p.voices_peak,p.long_calls);
 text(r,s,Q(8),Q(55),Q(8),white,-1,Q(.8),Q(304),0);
 snprintf(s,sizeof(s),"GAP %uK MAX %uK CB %uK",p.period_last/1000,p.period_max/1000,p.audio_max/1000);
 text(r,s,Q(8),Q(70),Q(8),white,-1,Q(.8),Q(304),0);
#if GIN_SERVICE_TRACE
 snprintf(s,sizeof(s),"FIRST CB %uK G %uK/%u D %uK/%u",p.service_first_cycles/1000,p.service_first_gin/1000,p.service_first_gin_calls,p.service_first_stock/1000,p.service_first_stock_calls);
#else
 if(p.peak_gin_calls==UINT32_MAX||p.peak_stock_calls==UINT32_MAX)
  snprintf(s,sizeof(s),"AT PEAK GIN %uK DBOX %uK / COUNT ?",p.peak_gin_cycles/1000,p.peak_stock_cycles/1000);
 else snprintf(s,sizeof(s),"AT PEAK C %uK/%u FULL %u FADE %u",p.peak_gin_cycles/1000,p.peak_gin_calls,p.peak_full_calls,p.peak_fade_calls);
#endif
 text(r,s,Q(8),Q(84),Q(7),white,-1,Q(.8),Q(304),0);
#if GIN_SERVICE_TRACE
 snprintf(s,sizeof(s),"FIRST FULL %u FADE %u",p.service_first_full,p.service_first_fade);
#else
 snprintf(s,sizeof(s),"WRAP T %uK S %uK R %uK D %uK",p.peak_wrapper_cycles[0]/1000,p.peak_wrapper_cycles[1]/1000,p.peak_wrapper_cycles[2]/1000,p.peak_wrapper_cycles[3]/1000);
#endif
 text(r,s,Q(8),Q(96),Q(7),white,-1,Q(.8),Q(304),0);
#if GIN_LEVEL_TRACE
 if(p.peak_level_calls)snprintf(s,sizeof(s),"LV G %u..%u M %d..%d / %u",p.peak_gain_min,p.peak_gain_max,p.peak_mix_min,p.peak_mix_max,p.peak_level_calls);
 else snprintf(s,sizeof(s),"LV NO OWNED RENDER IN PEAK CALLBACK");
#else
 snprintf(s,sizeof(s),"PRE T %uK S %uK R %uK D %uK",p.peak_pre_cycles[0]/1000,p.peak_pre_cycles[1]/1000,p.peak_pre_cycles[2]/1000,p.peak_pre_cycles[3]/1000);
#endif
 text(r,s,Q(8),Q(107),Q(6),white,-1,Q(.8),Q(304),0);
#if GIN_SERVICE_TRACE
 snprintf(s,sizeof(s),"DMA AGAIN %u PEND %u ERR %u HOLD %u SKIP %u",p.service_again,p.service_pending,p.service_error,p.service_first,p.service_unqualified);
#else
 snprintf(s,sizeof(s),"REJECT T %u D %u FIRST %u/%u",p.rejected[0],p.rejected[1],p.first_reject_op,p.first_reject_code);
#endif
 text(r,s,Q(8),Q(117),Q(6),white,-1,Q(.8),Q(304),0);
 text(r,"RED RESET / SHIFT BLUE CLOSE",Q(160),Q(126),Q(5),white,0,Q(.8),Q(304),0);
}
#endif
#include "gin_static_scene.inc"
static void init(void *scr){static_init(scr);}
void gin_screen_update(void *scr,int dt){
 gin_ui_snapshot_t v;uint32_t now=0;void *r;uint32_t result;
#ifdef OP1_HOST
 gin_host_edges=gin_host_text_divisions=0;
#endif
 draw_dx=draw_dy=0;text_alpha=32768;(void)dt;result=gin_native_snapshot(&v,&now);(void)now;
 unsigned play=!result&&v.state==GIN_UI_READY&&v.mode==0&&v.morph<=1000&&(v.side==GIN_UI_A||v.side==GIN_UI_B);
 if(play&&!motion_visible)gin_motion_rebase(&motion,now,gin_native_hit_count());
 motion_visible=play;
 unsigned retained=static_ready&&!gin_params_only&&!result&&v.state==GIN_UI_READY&&v.mode==0&&v.morph<=1000&&(v.side==GIN_UI_A||v.side==GIN_UI_B);
#ifdef GIN_PROFILE_DIAGNOSTIC
 if(gin_profile_open)retained=0;
#endif
 static_show(retained);if(retained)static_prepare(&v,now);
 op1_screen_draw(scr,dt);r=op1_raster();if(!r)return;


 if(!retained)for(unsigned row=0;row<8;row++)line(r,0,Q(10+20*row),Q(320),Q(10+20*row),Q(32),bg);
#ifdef GIN_PROFILE_DIAGNOSTIC
 if(gin_profile_open){diagnostics(r);return;}
#endif
 if(result||v.state!=GIN_UI_READY){const char *s=result?"ERROR":v.state==GIN_UI_LOADING?"LOADING":v.state==GIN_UI_ERROR?"ERROR":"UNAVAILABLE";text(r,s,Q(160),Q(76),Q(14),white,0,Q(.85),0,0);return;}
 if(v.morph>1000||(v.side!=GIN_UI_A&&v.side!=GIN_UI_B)){text(r,"ERROR",Q(160),Q(76),Q(14),white,0,Q(.85),0,0);return;}
 if(v.mode==3){help(r,&v);return;}
 if(v.mode==2){naming(r,&v);return;}
 if(v.mode==1){if(v.page>=GIN_UI_VIEW_COUNT){text(r,"ERROR",Q(160),Q(76),Q(14),white,0,Q(.85),0,0);return;}if(v.page==GIN_UI_ABOUT_PAGE)help(r,&v);else editor(r,&v);return;}
 if(retained)static_overlay(r,&v,now);else if(!gin_params_only)scene(r,&v,now);
 controls(r,&v,now);
 key_footer(r,&v);
}
#ifdef OP1_HOST
const op1_screen_t SCREEN_gin={init,gin_screen_update};
#else
extern void gin_ui_entry_update(void *,int);
const op1_screen_t SCREEN_gin={init,gin_ui_entry_update};
#endif
