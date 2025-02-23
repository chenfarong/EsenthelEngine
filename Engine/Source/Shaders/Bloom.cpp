/******************************************************************************/
#include "!Header.h"

#include "!Set Prec Struct.h"
BUFFER(Bloom)
   VecH4 BloomParams; // x=original, y=scale, z=cut, w=glow/(res*res)
BUFFER_END
#include "!Set Prec Default.h"

#ifndef GLOW
   #define GLOW 0
#endif
#ifndef CLAMP
   #define CLAMP 0
#endif
#ifndef HALF_RES
   #define HALF_RES 0
#endif
#ifndef DITHER
   #define DITHER 0
#endif
#define BLOOM_GLOW_GAMMA_PER_PIXEL 0 // #BloomGlowGammaPerPixel can be disabled because it will be faster but visual difference will be minimal
/******************************************************************************/
void BloomDS_VS(VtxInput vtx,
    NOPERSP out Vec2 outTex:TEXCOORD,
    NOPERSP out Vec4 outVtx:POSITION)
{
   outTex=vtx.tex (); if(GLOW)outTex-=ImgSize.xy*Vec2(HALF_RES ? 0.5 : 1.5, HALF_RES ? 0.5 : 1.5);
   outVtx=vtx.pos4();
}
VecH BloomColor(VecH color)
{
   Half col_lum=Max(color), lum=col_lum*BloomParams.y+BloomParams.z;
   return (lum>0) ? color*(Sqr(lum)/col_lum) : VecH(0, 0, 0);
}
VecH4 BloomDS_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET // "Max(0, " of the result is not needed because we're rendering to 1 byte per channel RT
{
   if(GLOW)
   {
      const Int res=(HALF_RES ? 2 : 4);

      VecH  color=0;
      VecH4 glow =0;
      UNROLL for(Int y=0; y<res; y++)
      UNROLL for(Int x=0; x<res; x++)
      {
         VecH4 c=TexLod(Img, UVClamp(inTex+ImgSize.xy*Vec2(x, y), CLAMP)); // can't use 'TexPoint' because 'Img' can be supersampled
         if(BLOOM_GLOW_GAMMA_PER_PIXEL)c.a=SRGBToLinearFast(c.a); // have to convert to linear because small glow of 1/255 would give 12.7/255 sRGB (Glow was sampled from non-sRGB texture and stored in RT alpha channel without any gamma conversions)
         color   +=c.rgb;
         glow.rgb+=c.rgb*c.a;
         glow.a  +=c.a;
      }
      if(!BLOOM_GLOW_GAMMA_PER_PIXEL)glow.a=SRGBToLinearFast(glow.a); // have to convert to linear because small glow of 1/255 would give 12.7/255 sRGB (Glow was sampled from non-sRGB texture and stored in RT alpha channel without any gamma conversions)
      glow.rgb*=(glow.a*BloomParams.w)/Max(Max(glow.rgb), HALF_MIN);
      color =BloomColor(color);
      color+=glow.rgb;
      return VecH4(color, 0);
   }else
   {
      if(HALF_RES)
      {
         return VecH4(BloomColor(TexLod(Img, UVClamp(inTex, CLAMP)).rgb), 0);
      }else
      {
         Vec2 tex_min=UVClamp(inTex-ImgSize.xy, CLAMP),
              tex_max=UVClamp(inTex+ImgSize.xy, CLAMP);
         return VecH4(BloomColor(TexLod(Img, Vec2(tex_min.x, tex_min.y)).rgb
                                +TexLod(Img, Vec2(tex_max.x, tex_min.y)).rgb
                                +TexLod(Img, Vec2(tex_min.x, tex_max.y)).rgb
                                +TexLod(Img, Vec2(tex_max.x, tex_max.y)).rgb), 0);
      }
   }
}
/******************************************************************************/
VecH4 Bloom_PS(NOPERSP Vec2 inTex:TEXCOORD,
               NOPERSP PIXEL              ):TARGET
{
   // final=src*original + Sat((src-cut)*scale)
   VecH4 col;
   col.rgb=TexLod(Img, inTex).rgb; // original, can't use 'TexPoint' because 'Img' can be supersampled
   col.rgb=col.rgb*BloomParams.x + TexLod(Img1, inTex).rgb; // bloom, can't use 'TexPoint' because 'Img1' can be smaller
   if(DITHER)ApplyDither(col.rgb, pixel.xy);
#if ALPHA
   col.a=TexLod(ImgX, inTex).r; // can't use 'TexPoint' because 'ImgX' can be supersampled
#else
   col.a=1; // force full alpha so back buffer effects can work ok
#endif
   return col;
}
/******************************************************************************/
