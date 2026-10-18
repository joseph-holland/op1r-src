/* Native SVG owns vessel geometry; absolute matrices precede graph drawing.
 * Horizontal clipped liquid, text, waves and streams are small raster overlays.
 * Handles are rebound per resource load; old/partial assets fail the marker. */
static void *static_root,*static_bottle[2],*static_glass,*static_liquid[9],*static_hit;
static unsigned static_ready,static_colour;
static uint32_t static_colour_seen,static_hit_seen,static_visible_seen;
static int32_t static_matrix[3][9];
static void static_visible(void *node,unsigned on){
#if !defined(OP1_HOST) || defined(GIN_HOST_RETAINED)
 if(node)op1_node_set_visible(node,on);
#else
 (void)node;(void)on;
#endif
}
static void static_init(void *scr){
 static_ready=0;static_root=0;
#if !defined(OP1_HOST) || defined(GIN_HOST_RETAINED)
 char name[32];static_root=op1_find_node(scr,"gin-static-scene");if(!static_root)return;
 static_visible(static_root,0);static_ready=op1_find_node(scr,"gin-fluid-scene")!=0;
 for(unsigned b=0;b<2;b++){snprintf(name,sizeof(name),"gin-bottle-%u",b);static_bottle[b]=op1_find_node(scr,name);if(!static_bottle[b])static_ready=0;}
 static_glass=op1_find_node(scr,"gin-glass");if(!static_glass)static_ready=0;
 for(unsigned c=0;c<9;c++){snprintf(name,sizeof(name),"gin-liquid-%u",c);static_liquid[c]=op1_find_node(scr,name);if(!static_liquid[c])static_ready=0;static_visible(static_liquid[c],0);}
 static_hit=op1_find_node(scr,"gin-hit");if(!static_hit)static_ready=0;static_visible(static_hit,0);
#else
 (void)scr;
#endif
 memset(static_matrix,0,sizeof(static_matrix));
 static_colour_seen=static_hit_seen=static_visible_seen=UINT32_MAX;
}
static void static_show(unsigned show){if(static_visible_seen!=show){static_visible(static_root,show);static_visible_seen=show;}}
static void static_transform(unsigned i,void *node,const int32_t *m){
 unsigned changed=0;for(unsigned j=0;j<9;j++)if(static_matrix[i][j]!=m[j])changed=1;
 if(changed){memcpy(static_matrix[i],m,sizeof(static_matrix[i]));
#if !defined(OP1_HOST) || defined(GIN_HOST_RETAINED)
  op1_node_set_transform(node,static_matrix[i]);
#else
  (void)node;
#endif
 }
}
static void static_prepare(const gin_ui_snapshot_t *v,uint32_t now){
 uint32_t hit=gin_native_hit_count();int32_t m[9];
 if(!clip_ready){prepare_glass_spans();clip_ready=1;}
 prepare_wave(v);gin_motion_step(&motion,v->id,v->morph,v->side,now,hit,settle_ticks(v));
 if(visual_revision!=v->revision&&(v->readout[0].active||v->readout[1].active||v->readout[2].active||v->readout[3].active))gin_motion_interaction(&motion);visual_revision=v->revision;
 for(unsigned b=0;b<2;b++){bottle_matrix(b,(unsigned)motion.progress[b],m);bottle_shake(b,m);m[6]+=draw_dx;m[7]+=draw_dy;static_transform(b,static_bottle[b],m);}
 memset(m,0,sizeof(m));m[0]=m[4]=m[8]=1024;
 m[6]=(motion_sine[((now*420u)/1024u)&63]*motion.shake)>>15;
 m[7]=(motion_sine[((now*420u)/1024u+16)&63]*motion.shake)>>16;
 static_transform(2,static_glass,m);draw_dx=draw_dy=0;
 static_colour=(v->morph+62u)/125u;
 if(static_colour!=static_colour_seen){for(unsigned i=0;i<9;i++)static_visible(static_liquid[i],i==static_colour);static_colour_seen=static_colour;}
 unsigned on=0;if(on!=static_hit_seen){static_visible(static_hit,on);static_hit_seen=on;}
 gin_scene_observation[0]=(uint32_t)motion.progress[0];gin_scene_observation[1]=(uint32_t)motion.progress[1];gin_scene_observation[2]=motion.flags;gin_scene_observation[3]=(uint32_t)motion.ripple;gin_scene_observation[4]=now;gin_scene_observation[5]=hit;gin_scene_observation[6]=v->id;++gin_scene_observation[7];gin_scene_observation[8]=(uint32_t)motion.shake;gin_scene_observation[9]=(uint32_t)m[6];gin_scene_observation[10]=(uint32_t)m[7];
}
static void static_overlay(void *r,const gin_ui_snapshot_t *v,uint32_t now){
 uint8_t colour[4],liquid[4];unsigned m=static_colour*125u;op1_pt wave[35];int32_t min=Q(109),max=Q(109),x[96];
 const char *brand=v->decoration==GIN_UI_DECOR_LINN?"LINN":"GIN";
 native_vessels=1;
 if(v->side==GIN_UI_A){bottle(r,1,0,(unsigned)motion.progress[1],brand);bottle(r,0,1,(unsigned)motion.progress[0],brand);}else{bottle(r,0,0,(unsigned)motion.progress[0],brand);bottle(r,1,1,(unsigned)motion.progress[1],brand);}
 native_vessels=0;draw_dx=static_matrix[2][6];draw_dy=static_matrix[2][7];
 if(m<=500){static const uint8_t mid[4]={177,107,255,255};blend(colour,blue,mid,m,500);}else{static const uint8_t mid[4]={177,107,255,255};blend(colour,mid,orange,m-500,500);}blend(liquid,face,colour,17,100);
 for(unsigned j=0;j<=32;j++){wave[j].x=Q(125)+Q(71)*(int32_t)j/32;wave[j].y=liquid_y(wave[j].x,now);if(wave[j].y<min)min=wave[j].y;if(wave[j].y>max)max=wave[j].y;}
 wave[33]=(op1_pt){Q(196),Q(141)};wave[34]=(op1_pt){Q(125),Q(141)};
 for(unsigned pass=0;pass<2;pass++)for(int row=98;row<124;row++){
  int32_t y=row*1024+512,l=glass_fill_spans.x[row][0]+Q(1.2),h=glass_fill_spans.x[row][1]-Q(1.2);if(h<=l)continue;
  if(!pass)line(r,l,y,h,y,LIQUID_STROKE,face);
  else if(y>=max)line(r,l,y,h,y,LIQUID_STROKE,liquid);
  else if(y>=min){unsigned count=intersections(wave,35,y,x);for(unsigned j=1;j<count;j+=2){int32_t a=x[j-1]>l?x[j-1]:l,b=x[j]<h?x[j]:h;if(b>a)line(r,a,y,b,y,LIQUID_STROKE,liquid);}}
 }
 for(unsigned j=1;j<=32;j++)if(inside(glass_body,COUNT(glass_body),wave[j-1].x,wave[j-1].y)&&inside(glass_body,COUNT(glass_body),wave[j].x,wave[j].y))line(r,wave[j-1].x,wave[j-1].y,wave[j].x,wave[j].y,Q(1.35),colour);
 int32_t noise=(float_q15(v->endpoint[0].Mix)*(int32_t)(1000-v->morph)+float_q15(v->endpoint[1].Mix)*(int32_t)v->morph)/1000;
 unsigned bubbles=(unsigned)(noise*4+16383)/32767+1;if(bubbles>5)bubbles=5;
 for(unsigned i=0;i<bubbles;i++){
  int32_t cx=Q(143+(i*11)%35),cy=Q(127)-(int32_t)((now*9u+i*Q(3))%Q(9));
  for(unsigned j=0;j<8;j++){unsigned k=j*2,n=(k+2)&15;op1_pt a={cx+circle_unit[k][0],cy+circle_unit[k][1]},b={cx+circle_unit[n][0],cy+circle_unit[n][1]};if(inside(glass_body,COUNT(glass_body),a.x,a.y)&&inside(glass_body,COUNT(glass_body),b.x,b.y))line(r,a.x,a.y,b.x,b.y,Q(.75),green);}
 }
 if(v->decoration==GIN_UI_DECOR_FISH)fish(r,now);
 int32_t gx=draw_dx,gy=draw_dy;draw_dx=draw_dy=0;
 for(unsigned b=0;b<2;b++)if((motion.flags&(1u<<b))&&(b?v->morph>0:v->morph<1000)&&mouth_x[b]>=Q(136)+gx&&mouth_x[b]<=Q(185)+gx&&mouth_y[b]<=Q(78)+gy){
  int32_t target=(b?Q(166):Q(154))+gx,landing=liquid_y(target-gx,now)+gy;op1_pt a={mouth_x[b],mouth_y[b]+Q(1)},c1={target,landing-Q(12)},d={target,landing},c0={a.x+(b?-Q(8):Q(9)),a.y+Q(2)},last=a;
  for(unsigned j=1;j<=16;j++){op1_pt next=bezier(a,c0,c1,d,(int32_t)j*1024/16);if(((now/60u+j)&1u)==0)line(r,last.x,last.y,next.x,next.y,Q(1),b?orange:blue);last=next;}
 }
 draw_dx=gx;draw_dy=gy;glass_rim(r);draw_dx=draw_dy=0;
}
