/******************************************************************************/
#include "!Header.h"
/******************************************************************************/
#ifndef DOWN_SAMPLE
#define DOWN_SAMPLE 0
#endif
/******************************************************************************/
VecH4 Outline_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
#if DOWN_SAMPLE
   inTex=(Floor(inTex*ImgSize.zw)+0.5)*ImgSize.xy; // we're rendering to a smaller RT, so inTex is located in the center between multiple texels, we need to align it so it's located at the center of the nearest one
#endif

   VecH4 col=TexLod(Img, inTex); // use linear filtering because 'Img' can be of different size
   Vec2  t0=inTex+ImgSize.xy*(DOWN_SAMPLE ? 2.5 : 0.5), // 2.5 scale was the min value that worked OK with 2.0 density
         t1=inTex-ImgSize.xy*(DOWN_SAMPLE ? 2.5 : 0.5);
   // use linear filtering because texcoords aren't rounded
#if 0
   if(Dist2(col, TexLod(Img, Vec2(t0.x, t0.y)))
     +Dist2(col, TexLod(Img, Vec2(t0.x, t1.y)))
     +Dist2(col, TexLod(Img, Vec2(t1.x, t0.y)))
     +Dist2(col, TexLod(Img, Vec2(t1.x, t1.y)))<=EPS_COL)col.a=0; // if all neighbors are the same then make this pixel transparent
#else // faster approximate
   if(Length2(col*4-TexLod(Img, Vec2(t0.x, t0.y))
                   -TexLod(Img, Vec2(t0.x, t1.y))
                   -TexLod(Img, Vec2(t1.x, t0.y))
                   -TexLod(Img, Vec2(t1.x, t1.y)))<=EPS_COL)col.a=0; // if all neighbors are the same then make this pixel transparent
#endif
   /* old code used for super sampling
	{
      Flt pos=TexDepthPoint(inTex);
      if(Dist2(col, TexLod(Img, inTex+ImgSize.xy*Vec2(-1,  0)))*(TexDepthPoint(inTex+ImgSize.xy*Vec2(-1,  0))>=pos)
        +Dist2(col, TexLod(Img, inTex+ImgSize.xy*Vec2( 1,  0)))*(TexDepthPoint(inTex+ImgSize.xy*Vec2( 1,  0))>=pos)
        +Dist2(col, TexLod(Img, inTex+ImgSize.xy*Vec2( 0, -1)))*(TexDepthPoint(inTex+ImgSize.xy*Vec2( 0, -1))>=pos)
        +Dist2(col, TexLod(Img, inTex+ImgSize.xy*Vec2( 0,  1)))*(TexDepthPoint(inTex+ImgSize.xy*Vec2( 0,  1))>=pos)<=EPS_COL)col.a=0;
	}*/
#if CLIP
   clip(col.a-EPS);
#endif
   return col;
}
/******************************************************************************/
VecH4 OutlineApply_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH4 color=TexLod(Img, inTex); // use linear filtering because 'Img' can be of different size
   for(Int i=0; i<6; i++)
   {
      Vec2  t=inTex+BlendOfs6[i]*ImgSize.xy;
      VecH4 c=TexLod(Img, t); // use linear filtering because texcoords aren't rounded
      if(c.a>color.a)color=c;
   }
   return color;
}
/******************************************************************************/
