/////////////////////////////////////////
//
//   OpenLieroX
//
//   Auxiliary Software class library
//
//   based on the work of JasonB
//   enhanced by Dark Charlie and Albert Zeyer
//
//   code under LGPL
//
/////////////////////////////////////////


// Graphics primitives
// Created 12/11/01
// By Jason Boettcher


#ifndef __GFXPRIMITIVES_H__
#define __GFXPRIMITIVES_H__

#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#include <SDL/SDL_gfxPrimitives.h>

#include "LieroX.h"



extern	int		iSurfaceFormat;


SDL_Surface*	LoadImage(const std::string& _filename, bool withalpha = false);

#define		LOAD_IMAGE(bmp,name) if (!Load_Image(bmp,name)) {return false;}
#define		LOAD_IMAGE_WITHALPHA(bmp,name) if (!Load_Image_WithAlpha(bmp,name)) {return false;}


inline bool Load_Image(SDL_Surface*& bmp, const std::string& name)  {
	bmp = LoadImage(name); 
	if (bmp == NULL)  { 
		printf("WARNING: could not load image %s\n", name.c_str()); 
		return false;
	}
	return true;
}

inline bool Load_Image_WithAlpha(SDL_Surface*& bmp, const std::string& name)  {
	bmp = LoadImage(name, true);
	if (bmp == NULL)  { 
		printf("WARNING: could not load image %s\n", name.c_str()); 
		return false;
	}
	return true;
}



// Surface stuff

// Creates a buffer with the same details as the screen
inline SDL_Surface* gfxCreateSurface(int width, int height) {
	SDL_PixelFormat *fmt = SDL_GetVideoSurface()->format;

	return SDL_CreateRGBSurface(iSurfaceFormat, width, height, 
		fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);
}


// Creates a buffer with the same details as screen, but with alpha channel
inline SDL_Surface* gfxCreateSurfaceAlpha(int width, int height) {
	SDL_PixelFormat *fmt = SDL_GetVideoSurface()->format;

	// it's also correct for big endian
	const Uint32 alpha = 0xff000000;

	return SDL_CreateRGBSurface(iSurfaceFormat | SDL_SRCALPHA,
		width, height, fmt->BitsPerPixel + 8,
		fmt->Rmask, fmt->Gmask, fmt->Bmask, alpha);
}



// Image drawing

// Simply draw the image
inline void DrawImage(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int x, int y) {
	static SDL_Rect	rDest;
	rDest.x = x; rDest.y = y;
	SDL_BlitSurface(bmpSrc,NULL,bmpDest,&rDest);
}

// Draw the image, with more options
inline void DrawImageEx(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int x, int y, int w, int h) {
	static SDL_Rect	rDest, rSrc;
	rDest.x = x; rDest.y = y;
	rSrc.x = 0; rSrc.y = 0;
	rSrc.w = w; rSrc.h = h;
	SDL_BlitSurface(bmpSrc,&rSrc,bmpDest,&rDest);
}

// Draw the image with a huge amount of options
inline void DrawImageAdv(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int sx, int sy, int dx, int dy, int w, int h) {
	static SDL_Rect	rDest, rSrc;
	rDest.x = dx; rDest.y = dy;
	rSrc.x = sx; rSrc.y = sy;
	rSrc.w = w; rSrc.h = h;
	SDL_BlitSurface(bmpSrc,&rSrc,bmpDest,&rDest);
}

// Draw the image mirrored
inline SDL_Surface* GetMirroredImage(SDL_Surface *bmpSrc) {
	return rotozoomSurfaceXY(bmpSrc, 0, -1, 1, 0);
}


inline SDL_Surface* GetStretched2Image(SDL_Surface* src) {
	return zoomSurface(src, 2, 2, 0);
}

inline SDL_Surface* GetStretched2Image(SDL_Surface* src, int x, int y, int w, int h) {
	if(x == 0 && y == 0 && w == src->w && h == src->h)
		return zoomSurface(src, 2, 2, 0);
	else {
		SDL_Surface* tmp = gfxCreateSurface(w,h);
		DrawImageAdv(tmp, src, x, y, 0, 0, w, h);
		SDL_Surface* stretched_surf = zoomSurface(tmp, 2, 2, 0);
		SDL_FreeSurface(tmp);
		return stretched_surf;
	}
}

inline void DrawImageStretch2(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int sx, int sy, int dx, int dy, int w, int h) {
	SDL_Surface* stretched_surf = GetStretched2Image(bmpSrc, sx, sy, w, h);
	DrawImageEx(bmpDest, stretched_surf, dx, dy, w*2, h*2);
	SDL_FreeSurface(stretched_surf);	
}
// TODO: what about this?
/*inline void DrawImageStretch2(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int sx, int sy, int dx, int dy, int w, int h) {
	static SDL_Rect rSrc,rDest;
	rSrc.x = sx;
	rSrc.y = sy;
	rSrc.w = w;
	rSrc.h = h;
	rDest.x = dx;
	rDest.y = dy;
	rDest.w = w*2;
	rDest.h = h*2;
	SDL_SoftStretch(bmpSrc,&rSrc,bmpDest,&rDest);
}*/

inline void DrawImageStretch2Key(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int sx, int sy, int dx, int dy, int w, int h, Uint32 key) {
	Uint32 oldkey = bmpSrc->format->colorkey;
	SDL_SetColorKey(bmpSrc, 0, 0);  // Temporarily remove any color keys
	SDL_Surface* stretched_surf = GetStretched2Image(bmpSrc, sx, sy, w, h);
	SDL_SetColorKey(bmpSrc,SDL_SRCCOLORKEY, oldkey);  // Restore the original key
	SDL_SetColorKey(stretched_surf, SDL_SRCCOLORKEY, key);
	DrawImageEx(bmpDest, stretched_surf, dx, dy, w*2, h*2);
	SDL_FreeSurface(stretched_surf);
}

inline void DrawImageStretchMirrorKey(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int sx, int sy, int dx, int dy, int w, int h, Uint32 key) {
	SDL_Surface* stretched_surf = GetStretched2Image(bmpSrc, sx, sy, w, h);
	SDL_Surface* mirrored_surf = GetMirroredImage(stretched_surf);
	SDL_FreeSurface(stretched_surf);
	SDL_SetColorKey(mirrored_surf, SDL_SRCCOLORKEY, key);
	DrawImageEx(bmpDest, mirrored_surf, dx, dy, w*2, h*2);
	SDL_FreeSurface(mirrored_surf);
}



// Draws a sprite doubly stretched but not so advanced
inline void	DrawImageStretch(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int dx, int dy) {
	DrawImageStretch2(bmpDest,bmpSrc,0,0,dx,dy,bmpSrc->w,bmpSrc->h);
}

// Draws a sprite doubly stretched, with a colour key and not so advanced
inline void	DrawImageStretchKey(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int dx, int dy, Uint32 key) {
	DrawImageStretch2Key(bmpDest,bmpSrc,0,0,dx,dy,bmpSrc->w,bmpSrc->h,key);
}


// Solid drawing
inline void	DrawRectFill(SDL_Surface *bmpDest, int x, int y, int x2, int y2, Uint32 color) {
	Uint8 r,g,b;
	SDL_GetRGB(color, bmpDest->format, &r,&g,&b);
	boxRGBA(bmpDest, x,y,x2-1,y2-1, r,g,b,255);
}

// TODO: what about this?
/*inline void	DrawRectFill(SDL_Surface *bmpDest, int x, int y, int x2, int y2, Uint32 color) {
	static SDL_Rect r;
	r.x = x;
	r.y = y;
	r.w = x2-x;
	r.h = y2-y;
	SDL_FillRect(bmpDest,&r,color);
}*/

inline void	DrawRect(SDL_Surface *bmpDest, int x, int y, int x2, int y2, Uint32 colour) {
	Uint8 r,g,b;
	SDL_GetRGB(colour, bmpDest->format, &r,&g,&b);
	rectangleRGBA(bmpDest, x,y,x2,y2, r,g,b,255);
}

inline void DrawRectFillA(SDL_Surface *bmpDest, int x, int y, int x2, int y2, Uint32 color, int alpha) {
	Uint8 r,g,b;
	SDL_GetRGB(color, SDL_GetVideoSurface()->format, &r,&g,&b);
	boxRGBA(bmpDest, x,y,x2,y2, r,g,b,alpha);
}
// TODO: what about this?
/*inline void DrawRectFillA(SDL_Surface *bmpDest, int x, int y, int x2, int y2, Uint32 color, Uint8 alpha)  {
	SDL_Surface *tmp = gfxCreateSurface(x2-x,y2-y);
	if (tmp)  {
		SDL_SetAlpha(tmp,SDL_SRCALPHA | SDL_RLEACCEL, alpha);
		SDL_FillRect(tmp,NULL,color);
		DrawImage(bmpDest,tmp,x,y);
		SDL_FreeSurface(tmp);
	}
}*/

inline void	DrawHLine(SDL_Surface *bmpDest, int x, int x2, int y, Uint32 colour) {
	Uint8 r,g,b;
	SDL_GetRGB(colour, bmpDest->format, &r,&g,&b);
	hlineRGBA(bmpDest, x,x2,y, r,g,b,255);
}
// TODO: what about this?
/*inline void	DrawHLine(SDL_Surface *bmpDest, int x, int x2, int y, Uint32 colour) {
	DrawRectFill(bmpDest,x,y,x2,y,colour);
}*/

inline void	DrawVLine(SDL_Surface *bmpDest, int y, int y2, int x, Uint32 colour) {
	Uint8 r,g,b;
	SDL_GetRGB(colour, bmpDest->format, &r,&g,&b);
	vlineRGBA(bmpDest, x,y,y2, r,g,b,255);
}
// TODO: what about this?
/*inline void	DrawVLine(SDL_Surface *bmpDest, int y, int y2, int x, Uint32 colour) {
	DrawRectFill(bmpDest,x,y,x,y2,colour);
}*/

inline void DrawTriangle(SDL_Surface *bmpDest, int x1, int y1, int x2, int y2, int x3, int y3, Uint32 colour) {
	Uint8 r,g,b;
	SDL_GetRGB(colour, bmpDest->format, &r,&g,&b);
	trigonRGBA(bmpDest, x1,y1, x2,y2, x3,y3, r,g,b,255);
}


// Pixel drawing
inline void PutPixel(SDL_Surface *bmpDest, int x, int y, Uint32 colour) {
	Uint8 r,g,b;
	SDL_GetRGB(colour, bmpDest->format, &r,&g,&b);
	pixelRGBA(bmpDest, x,y, r,g,b,255);
}


// Get a pixel from an 8bit address
inline Uint32 GetPixelFromAddr(Uint8 *p, int bpp) {
	switch(bpp) {
		// 8 bpp
		case 1:
			return *p;

		// 16 bpp
		case 2:
			return *(Uint16 *)p;

		// 24 bpp
		case 3:
			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				return p[0] << 16 | p[1] << 8 | p[2];
			#else
				return p[0] | p[1] << 8 | p[2] << 16;
			#endif

		// 32 bpp
		case 4:
			return *(Uint32 *)p;

		default:
			return 0;
	}
}

// Get a pixel from the surface
inline Uint32 GetPixel(SDL_Surface *bmpSrc, int x, int y) {
    int bpp = bmpSrc->format->BytesPerPixel;
	return GetPixelFromAddr((Uint8 *)bmpSrc->pixels + y * bmpSrc->pitch + x * bpp, bpp);
}




// Extract 4 colour components from a packed int
inline void GetColour4(Uint32 pixel, SDL_Surface *img, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a) {
	SDL_GetRGBA(pixel,img->format,r,g,b,a);
}

// Creates a int colour based on the 3 components
inline Uint32 MakeColour(Uint8 r, Uint8 g, Uint8 b) {
	return SDL_MapRGB(SDL_GetVideoSurface()->format,r,g,b);
}

inline Uint32 MakeColour(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	return SDL_MapRGBA(SDL_GetVideoSurface()->format,r,g,b,a);
}

// Line drawing
inline void DrawLine(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color) {
	Uint8 r,g,b;
	SDL_GetRGB(color, dst->format, &r,&g,&b);
	aalineRGBA(dst, x1,y1, x2,y2, r,g,b,255);
}

// Line drawing
inline void FastDrawLine(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color) {
	Uint8 r,g,b;
	SDL_GetRGB(color, dst->format, &r,&g,&b);
	lineRGBA(dst, x1,y1, x2,y2, r,g,b,255);
}




void	DrawRope(SDL_Surface *bmp, int x1, int y1, int x2, int y2, Uint32 color);
void	DrawBeam(SDL_Surface *bmp, int x1, int y1, int x2, int y2, Uint32 color);
void	DrawLaserSight(SDL_Surface *bmp, int x1, int y1, int x2, int y2, Uint32 color);

bool	SaveSurface(SDL_Surface *Image, const std::string& FileName, int Format, bool Tournament);

#endif  //  __GFXPRIMITIVES_H__
