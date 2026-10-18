/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */



#include "gin_screen.h"
#include <stddef.h>
typedef struct {int32_t x0,y0,x1,y1;} gin_font_segment_t;
typedef struct {uint16_t code,first,count;int32_t width;} gin_font_glyph_t;
#include "gin_font.private.inc"
#define GLYPHS (sizeof(gin_font_glyphs)/sizeof(gin_font_glyphs[0]))
#define BASE_HEIGHT 58053
static const gin_font_glyph_t *glyph(unsigned char c){
 unsigned i;if(c>='a'&&c<='z')c-='a'-'A';
 for(i=0;i<GLYPHS;++i)if(gin_font_glyphs[i].code==c)return &gin_font_glyphs[i];return 0;
}

static const struct {unsigned char code,row[7];} fallback[]={{33, {4,4,4,4,4,0,4}},{34, {10,10,10,0,0,0,0}},{35, {10,31,10,10,31,10,0}},{36, {4,15,20,14,5,30,4}},{38, {12,18,20,8,21,18,13}},{39, {4,4,8,0,0,0,0}},{40, {2,4,8,8,8,4,2}},{41, {8,4,2,2,2,4,8}},{42, {0,21,14,31,14,21,0}},{44, {0,0,0,0,0,4,8}},{59, {0,4,0,0,4,4,8}},{60, {1,2,4,8,4,2,1}},{61, {0,0,31,0,31,0,0}},{62, {16,8,4,2,4,8,16}},{63, {14,17,1,2,4,0,4}},{64, {14,17,23,21,23,16,14}},{91, {14,8,8,8,8,8,14}},{92, {16,16,8,4,2,1,1}},{93, {14,2,2,2,2,2,14}},{94, {4,10,17,0,0,0,0}},{96, {8,4,2,0,0,0,0}},{123, {2,4,4,8,4,4,2}},{124, {4,4,4,4,4,4,4}},{125, {8,4,4,2,4,4,8}},{126, {0,0,9,22,0,0,0}}};
static const unsigned char *bitmap(unsigned char c){for(unsigned i=0;i<sizeof(fallback)/sizeof(fallback[0]);i++)if(fallback[i].code==c)return fallback[i].row;return 0;}
static int32_t scale(int32_t v,int32_t factor){return(int32_t)(((int64_t)v*factor)>>24);}
static int32_t factor_for(int32_t h){
#ifdef OP1_HOST
 extern uint32_t gin_host_text_divisions;++gin_host_text_divisions;
#endif
 return(int32_t)(((int64_t)h<<24)/BASE_HEIGHT);
}
static int32_t advance(const gin_font_glyph_t *g,int32_t h,int32_t factor){
 int32_t w=g?scale(g->width,factor):h*48/100,min=h*22/100;
 if(g&&w<min)w=min;return g?w+h*17/100:w;
}
static int32_t char_advance(unsigned char c,const gin_font_glyph_t *g,int32_t h,int32_t factor){if(c=='_'&&!g)return advance(glyph('-'),h,factor);return !g&&c!=' '?h*6/7:advance(g,h,factor);}
static int32_t width(const char *s,int32_t h,int32_t factor){
 int32_t n=0;unsigned i;int used=0;
 for(i=0;i<40&&s[i];++i){const gin_font_glyph_t *g=glyph((unsigned char)s[i]);if((unsigned char)s[i]>=32&&(unsigned char)s[i]<=126){n+=char_advance((unsigned char)s[i],g,h,factor);used=1;}}
 return used?n-h*17/100:0;
}
static op1_pt point(int32_t x,int32_t y,const int32_t *m){
 op1_pt p;
 if(m){p.x=(int32_t)(((int64_t)x*m[0]+(int64_t)y*m[3])/1024)+m[6];p.y=(int32_t)(((int64_t)x*m[1]+(int64_t)y*m[4])/1024)+m[7];}
 else{p.x=x;p.y=y;}return p;
}
void gin_native_draw_text(void *r,const char *s,int32_t x,int32_t y,int32_t h,const uint8_t rgba[4],int align,int32_t weight,int32_t max,const int32_t m[9]){
 unsigned i,j;int32_t w,cursor=0,factor;
 if(!r||!s||!rgba||h<=0||h>32*1024||weight<=0)return;
 factor=factor_for(h);w=width(s,h,factor);if(max>0&&w>max){h=(int32_t)(((int64_t)h*max)/w);factor=factor_for(h);w=width(s,h,factor);}
 if(align==0)x-=w/2;else if(align>0)x-=w;
 for(i=0;i<40&&s[i];++i){const gin_font_glyph_t *g=glyph((unsigned char)s[i]);
  if(s[i]==' '){cursor+=advance(0,h,factor);continue;}if(!g){if(s[i]=='_'){const gin_font_glyph_t *dash=glyph('-');int32_t ink=dash?scale(dash->width,factor):h*48/100;op1_pt a=point(x+cursor,y+h,m),b=point(x+cursor+ink,y+h,m);op1_raster_line(r,&a,&b,weight,rgba);cursor+=char_advance('_',0,h,factor);continue;}const unsigned char *rows=bitmap((unsigned char)s[i]);if(rows){int32_t cell=h/7;for(unsigned yy=0;yy<7;yy++)for(unsigned xx=0;xx<5;xx++)if(rows[yy]&(1u<<(4-xx))){op1_pt a=point(x+cursor+(int32_t)xx*cell,y+(int32_t)yy*cell,m),b=point(x+cursor+(int32_t)(xx+1)*cell,y+(int32_t)yy*cell,m);op1_raster_line(r,&a,&b,cell,rgba);}}cursor+=h*6/7;continue;}
  int32_t ink=scale(g->width,factor),minimum=h*22/100,bearing=ink<minimum?(minimum-ink)/2:0;
  for(j=0;j<g->count;){
   const gin_font_segment_t *a=&gin_font_segments[g->first+j];op1_pt p={x+cursor+bearing+scale(a->x0,factor),y+scale(a->y0,factor)},q={x+cursor+bearing+scale(a->x1,factor),y+scale(a->y1,factor)},pending[16];unsigned used=1,k;
   pending[0]=q;



   for(k=j+1;k<g->count&&used<16;k++){
    const gin_font_segment_t *next=&gin_font_segments[g->first+k],*last=&gin_font_segments[g->first+k-1];op1_pt end;int32_t dx,dy;int good=1;
    if(next->x0!=last->x1||next->y0!=last->y1)break;
    end=(op1_pt){x+cursor+bearing+scale(next->x1,factor),y+scale(next->y1,factor)};dx=end.x-p.x;dy=end.y-p.y;
    if(!dx&&!dy)break;
    for(unsigned n=0;n<used;n++){int64_t cross=(int64_t)dx*(pending[n].y-p.y)-(int64_t)dy*(pending[n].x-p.x);int32_t ax=dx<0?-dx:dx,ay=dy<0?-dy:dy;if(cross<0)cross=-cross;int64_t dot=(int64_t)dx*(pending[n].x-p.x)+(int64_t)dy*(pending[n].y-p.y),length=(int64_t)dx*dx+(int64_t)dy*dy;if(cross>(int64_t)128*(ax+ay)||dot<0||dot>length){good=0;break;}}
    if(!good)break;pending[used++]=end;q=end;
   }
   {op1_pt start=point(p.x,p.y,m),end=point(q.x,q.y,m);if(start.x!=end.x||start.y!=end.y)op1_raster_line(r,&start,&end,weight,rgba);}
   j+=used;
  }
  cursor+=advance(g,h,factor);
 }
}

int32_t gin_native_text_cursor(const char *s,unsigned at,int32_t h,int32_t max){
 if(!s||h<=0||h>32*1024)return 0;int32_t f=factor_for(h),w=width(s,h,f);if(max>0&&w>max){h=(int32_t)(((int64_t)h*max)/w);f=factor_for(h);}
 int32_t x=0;for(unsigned i=0;i<at&&i<23&&s[i];i++)x+=char_advance((unsigned char)s[i],glyph((unsigned char)s[i]),h,f);return x;
}
