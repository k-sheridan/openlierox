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

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <string>
#include <assert.h>

//
// Misc routines, defines and variables
//

// Flags used for screen and new surfaces
extern	int		iSurfaceFormat;

// Like in SDL_video.c in SDL_DisplayFormatAlpha
#define ALPHASURFACE_RMASK 0x00ff0000
#define ALPHASURFACE_GMASK 0x0000ff00
#define ALPHASURFACE_BMASK 0x000000ff
#define ALPHASURFACE_AMASK 0xff000000

///////////////////
// If you want to use the adress of some Uint32 directly with memcpy or similar, use this
inline Uint32 SDLColourToNativeColour(Uint32 pixel, short bpp) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	return (pixel << (32 - 8 * bpp));
#else
	return pixel;
#endif
}

/////////////////
// If you copied some data directly with memcpy into an Uint32, use this
inline Uint32 NativeColourToSDLColour(Uint32 pixel, short bpp) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	return (pixel >> (32 - 8 * bpp));
#else
	return pixel;
#endif
}


//
// Clipping routines
//

/////////////////////
// Clip the line to the surface
bool ClipLine(SDL_Surface * dst, int * x1, int * y1, int * x2, int * y2);


class SDLRectBasic : private SDL_Rect {
public:
	typedef Sint16 Type;
	typedef Uint16 TypeS;
	
	inline Type& x() { return ((SDL_Rect*)this)->x; }
	inline Type& y() { return ((SDL_Rect*)this)->y; }
	inline TypeS& width() { return w; }
	inline TypeS& height() { return h; }
	
	inline Type x() const { return ((SDL_Rect*)this)->x; }
	inline Type y() const { return ((SDL_Rect*)this)->y; }
	inline Type width() const { return w; }
	inline Type height() const { return h; }
};

template<typename _Type, typename _TypeS>
class RefRectBasic {
public:
	typedef _Type Type;
	typedef _TypeS TypeS;
private:
	Type &m_x, &m_y;
	TypeS &m_w, &m_h;
public:
	RefRectBasic(Type& x_, Type& y_, TypeS& w_, TypeS& h_)
	: m_x(x_), m_y(y_), m_w(w_), m_h(h_) {}
	
	inline Type& x() { return m_x; }
	inline Type& y() { return m_y; }
	inline TypeS& width() { return m_w; }
	inline TypeS& height() { return m_h; }
	
	inline Type x() const { return m_x; }
	inline Type y() const { return m_y; }
	inline Type width() const { return m_w; }
	inline Type height() const { return m_h; }
};


// _RectBasic has to provide the following public members:
//		typedef ... Type; // type for x,y
//		typedef ... TypeS; // type for w,h
//		Type x();
//		Type y();
//		TypeS width();
//		TypeS height();
//		and the above as const
template<typename _RectBasic>
class Rect : public _RectBasic {
public:
	
	class AssignX2 : private Rect {
	public:
		inline AssignX2& operator=(const typename Rect::Type& v)
		{ this->Rect::width() = v - this->Rect::x(); return *this; }
		inline operator typename Rect::Type () const
		{ return this->Rect::x() + this->Rect::width(); }
	};
	inline AssignX2& x2() { return (AssignX2&)*this; }
	inline const AssignX2& x2() const { return (const AssignX2&)*this; }
	
	class AssignY2 : private Rect {
	public:
		inline AssignY2& operator=(const typename Rect::Type& v)
		{ this->Rect::height() = v - this->Rect::y(); return *this; }
		inline operator typename Rect::Type () const
		{ return this->Rect::y() + this->Rect::height(); }
	};
	inline AssignY2& y2() { return (AssignY2&)*this; }
	inline const AssignY2& y2() const { return (AssignY2&)*this; }
	
	template<typename _ClipRect>
	inline bool clipWith(const _ClipRect& clip) {
		// Horizontal
		this->Rect::x() = MAX( (typename Rect::Type)this->Rect::x(), (typename Rect::Type)clip.x() );
		this->Rect::x2() = MIN( (typename Rect::Type)this->Rect::x2(), (typename Rect::Type)clip.x2() );
		
		// Vertical
		this->Rect::y() = MAX( (typename Rect::Type)this->Rect::y(), (typename Rect::Type)clip.y() );
		this->Rect::y2() = MIN( (typename Rect::Type)this->Rect::y2(), (typename Rect::Type)clip.y2() );
		
		return (this->Rect::width() && this->Rect::height());
	}
};


typedef Rect<SDLRectBasic> SDLRect;  // Use this for creating clipping rects from SDL

template<typename _Type, typename _TypeS, typename _ClipRect>
inline bool ClipRefRectWith(_Type& x, _Type& y, _TypeS& w, _TypeS& h, const _ClipRect& clip) {
	RefRectBasic<_Type, _TypeS> refrect = RefRectBasic<_Type, _TypeS>(x, y, w, h);
	return ((Rect<RefRectBasic<_Type, _TypeS> >&) refrect).clipWith(clip);
}

template<typename _ClipRect>
inline bool ClipRefRectWith(SDL_Rect& rect, const _ClipRect& clip) {
	return ((SDLRect&)rect).clipWith(clip);
}


//
// Image loading and saving
//

//////////////////
// Load an image
SDL_Surface*	LoadImage(const std::string& _filename, bool withalpha = false);

/////////////////
// Loads an image and quits with error if could not load
#define		LOAD_IMAGE(bmp,name) if (!Load_Image(bmp,name)) {return false;}
#define		LOAD_IMAGE_WITHALPHA(bmp,name) if (!Load_Image_WithAlpha(bmp,name)) {return false;}

/////////////////
// Gets the colorkey from the surface
#define		COLORKEY(bmp) ((bmp)->format->colorkey)


/////////////////////
// Load an image, without alpha channel
inline bool Load_Image(SDL_Surface*& bmp, const std::string& name)  {
	bmp = LoadImage(name); 
	if (bmp == NULL)  { 
		printf("WARNING: could not load image %s\n", name.c_str()); 
		return false;
	}
	return true;
}

////////////////////
// Load an image with alpha channel
inline bool Load_Image_WithAlpha(SDL_Surface*& bmp, const std::string& name)  {
	bmp = LoadImage(name, true);
	if (bmp == NULL)  { 
		printf("WARNING: could not load image %s\n", name.c_str()); 
		return false;
	}
	return true;
}

///////////////////
// Save surface in the specified format
bool SaveSurface(SDL_Surface *Image, const std::string& FileName, int Format, bool Tournament);


//
// Surface stuff
//

//////////////////
// Creates a buffer with the same details as the screen
inline SDL_Surface* gfxCreateSurface(int width, int height) {
	if (width <= 0 || height <= 0) // Nonsense, can cause trouble
		return NULL;

	SDL_PixelFormat* fmt = SDL_GetVideoSurface()->format;

	SDL_Surface* result = SDL_CreateRGBSurface(
			iSurfaceFormat, width, height, 
			fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);

	if (result)
		// OpenGL strictly requires the surface to be cleared
		SDL_FillRect(result, NULL, SDL_MapRGBA(result->format, 0, 0, 0, 255));
	
	return result;
}


///////////////////
// Creates an ARGB 32bit surface
inline SDL_Surface* gfxCreateSurfaceAlpha(int width, int height) {
	if (width <= 0 || height <= 0) // Nonsense, can cause trouble
		return NULL;

	SDL_Surface* result = SDL_CreateRGBSurface(
			SDL_SWSURFACE | SDL_SRCALPHA,
			width, height, 32,
			ALPHASURFACE_RMASK, ALPHASURFACE_GMASK, ALPHASURFACE_BMASK, ALPHASURFACE_AMASK);

	if (result)
		// OpenGL strictly requires the surface to be cleared
		SDL_FillRect( result, NULL, SDL_MapRGB(result->format, 0, 0, 0));
	
	return result;
}



//
// Image drawing
//

///////////////
// Copies one surface to another (not blitting, so the alpha values are kept!)
void CopySurface(SDL_Surface* dst, SDL_Surface* src, int sx, int sy, int dx, int dy, int w, int h);

///////////////
// Simply draw the image
inline void DrawImage(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int x, int y) {
	static SDL_Rect	rDest;
	rDest.x = x; rDest.y = y;
	SDL_BlitSurface(bmpSrc,NULL,bmpDest,&rDest);
}

///////////////
// Draw the image, with more options
inline void DrawImageEx(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int x, int y, int w, int h) {
	static SDL_Rect	rDest, rSrc;
	rDest.x = x; rDest.y = y;
	rSrc.x = 0; rSrc.y = 0;
	rSrc.w = w; rSrc.h = h;
	SDL_BlitSurface(bmpSrc,&rSrc,bmpDest,&rDest);
}

//////////////
// Draw the image with a huge amount of options
inline void DrawImageAdv(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int sx, int sy, int dx, int dy, int w, int h) {
	static SDL_Rect	rDest, rSrc;
	rDest.x = dx; rDest.y = dy;
	rSrc.x = sx; rSrc.y = sy;
	rSrc.w = w; rSrc.h = h;
	SDL_BlitSurface(bmpSrc,&rSrc,bmpDest,&rDest);
}


///////////////
// Draws image mirror flipped
// WARNING: passing invalid source x/y/w/h causes a segfault
void DrawImageAdv_Mirror(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int sx, int sy, int dx, int dy, int w, int h);

////////////////
// Draws the image doubly stretched (fast)
void DrawImageStretch2(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int sx, int sy, int dx, int dy, int w, int h);

/////////////////
// Draws the image doubly stretched while checking for colorkey
void DrawImageStretch2Key(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int sx, int sy, int dx, int dy, int w, int h, Uint32 key);

/////////////////
// Draws image doubly stretched, mirrored and checking for colorkey
// WARNING: passing invalid source x/y/w/h causes a segfault
void DrawImageStretchMirrorKey(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int sx, int sy, int dx, int dy, int w, int h, Uint32 key);

/////////////////
// Creates a new surface of the same size and draws the image mirror flipped onto it
inline SDL_Surface *GetMirroredImage(SDL_Surface *bmpSrc)  {
	SDL_Surface* result = SDL_CreateRGBSurface(
			bmpSrc->flags,
			bmpSrc->w, bmpSrc->h,
			bmpSrc->format->BitsPerPixel,
			bmpSrc->format->Rmask,
			bmpSrc->format->Gmask,
			bmpSrc->format->Bmask,
			bmpSrc->format->Amask);
	if (!result)
		return NULL;
	DrawImageAdv_Mirror(result, bmpSrc, 0, 0, 0, 0, bmpSrc->w, bmpSrc->h);
	return result;
}

/////////////////
// Draws a sprite doubly stretched but not so advanced
inline void	DrawImageStretch(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int dx, int dy) {
	DrawImageStretch2(bmpDest,bmpSrc,0,0,dx,dy,bmpSrc->w,bmpSrc->h);
}

/////////////////
// Draws a sprite doubly stretched, with a colour key and not so advanced
inline void	DrawImageStretchKey(SDL_Surface *bmpDest, SDL_Surface *bmpSrc, int dx, int dy, Uint32 key) {
	DrawImageStretch2Key(bmpDest, bmpSrc, 0, 0, dx, dy, bmpSrc->w, bmpSrc->h, key);
}

/////////////////
// Draws the image resized according to ratios
void DrawImageResizedAdv( SDL_Surface *bmpDest, SDL_Surface *bmpSrc, float sx, float sy, int dx, int dy, int sw, int sh, float xratio, float yratio);

/////////////////
// Draws the image nicely resampled, blur says how much the result should be blurred
void DrawImageResampledAdv( SDL_Surface *bmpDest, SDL_Surface *bmpSrc, float sx, float sy, int dx, int dy, int sw, int sh, float xratio, float yratio, float blur = 1.0f);


//
// Pixel and color routines
//

/////////////////
// Put pixel to a specified address
// WARNING: passing an invalid adress will cause a segfault
inline void PutPixelToAddr(Uint8* p, Uint32 color, short bpp) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	memcpy(p, (Uint8*)&color + 4 - bpp, bpp);
#else	
	memcpy(p, &color, bpp);
#endif
}

//////////////
// Pixel drawing
// WARNING: passing invalid coordinates will cause a segfault
inline void PutPixel(SDL_Surface *bmpDest, int x, int y, Uint32 color) {
	PutPixelToAddr(
			(Uint8*)bmpDest->pixels + y * bmpDest->pitch + x * bmpDest->format->BytesPerPixel,
			color,
			bmpDest->format->BytesPerPixel);
}

////////////////
// Get a pixel from an 8bit address
// WARNING: passing invalid adress will cause a segfault
inline Uint32 GetPixelFromAddr(Uint8* p, short bpp) {
	static Uint32 result;
	result = 0;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	memcpy((Uint8*)&result + 4 - bpp, p, bpp);
#else
	memcpy(&result, p, bpp);
#endif	
	return result;
}

////////////////
// Get a pixel from the surface
// WARNING: passing invalid coordinates will cause a segfault
inline Uint32 GetPixel(SDL_Surface* bmpSrc, int x, int y) {
	return GetPixelFromAddr(
			(Uint8*)bmpSrc->pixels + y * bmpSrc->pitch + x * bmpSrc->format->BytesPerPixel,
			bmpSrc->format->BytesPerPixel);
}

inline void CopyPixel_SameFormat(
	SDL_Surface* dst, SDL_Surface* src,
	int dx, int dy, int sx, int sy) {
	memcpy(
		(Uint8*)dst->pixels + dy * dst->pitch + dx * dst->format->BytesPerPixel,
		(Uint8*)src->pixels + sy * src->pitch + sx * dst->format->BytesPerPixel,
		dst->format->BytesPerPixel);
}

inline void CopyPixel_SameFormat(
	SDL_Surface* dst, SDL_Surface* src, int x, int y) {
	CopyPixel_SameFormat(dst, src, x, y, x, y);
}


////////////////
// Put pixel alpha blended with the background
// WARNING: passing invalid coordinates will cause a segfault
void PutPixelA(SDL_Surface *bmpDest, int x, int y, Uint32 colour, Uint8 a);

////////////////
// Extract 4 colour components from a packed int
inline void GetColour4(Uint32 pixel, SDL_Surface* img, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a) {
	SDL_GetRGBA(pixel, img->format, r, g, b, a);
}

///////////////
// Extract 3 colour components from a packed int
inline void GetColour3(Uint32 pixel, SDL_Surface* img, Uint8 *r, Uint8 *g, Uint8 *b) {
	SDL_GetRGB(pixel, img->format, r, g, b);
}

///////////////
// Returns true if the two colors are the same, ignoring the alpha
inline bool EqualRGB(Uint32 p1, Uint32 p2, SDL_PixelFormat* fmt) {
	return ((p1|fmt->Amask) == (p2|fmt->Amask));
}

///////////////
// Creates a int colour based on the 3 components
// HINT: format is that one from videosurface!
inline Uint32 MakeColour(Uint8 r, Uint8 g, Uint8 b) {
	return SDL_MapRGB(SDL_GetVideoSurface()->format,r,g,b);
}

////////////////
// Returns true if the color is considered as (partly) transparent on the surface
inline bool IsTransparent(SDL_Surface* surf, Uint32 color)  {
	if((surf->flags & SDL_SRCALPHA) && ((color & surf->format->Amask) != surf->format->Amask))
		return true;
	
	// TODO: should this check be done, if SDL_SRCALPHA was set? SDL/OpenGL possibly will ignore it
	if((surf->flags & SDL_SRCCOLORKEY) && (EqualRGB(color, COLORKEY(surf), surf->format)))
		return true;
		
	return false;
}




//
// Solid drawing
//


///////////////////
// Draw horizontal line
void	DrawHLine(SDL_Surface *bmpDest, int x, int x2, int y, Uint32 colour);

///////////////////
// Draw vertical line
void	DrawVLine(SDL_Surface *bmpDest, int y, int y2, int x, Uint32 colour);

///////////////////
// Draw a line
void	DrawLine(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);

//////////////////
// Draw the line nicely antialiased
void	AntiAliasedLine(SDL_Surface * dst, int x1, int y1, int x2, int y2, Uint32 color, void (*proc)(SDL_Surface *, int, int, Uint32, float));

/////////////////////
// Draws a filled rectangle
// WARNING: not threadsafe
inline void	DrawRectFill(SDL_Surface *bmpDest, int x, int y, int x2, int y2, Uint32 color) {
	static SDL_Rect r;
	r.x = x;
	r.y = y;
	r.w = x2-x;
	r.h = y2-y;
	SDL_FillRect(bmpDest,&r,color);
}

////////////////////
// Fills the surface with specified colour
inline void FillSurface(SDL_Surface* dst, Uint32 colour) {
	SDL_FillRect(dst, NULL, colour);
}

////////////////////
// Draws a rectangle
inline void	DrawRect(SDL_Surface *bmpDest, int x, int y, int x2, int y2, Uint32 colour) {
	DrawHLine(bmpDest, x, x2, y, colour);
	DrawHLine(bmpDest, x, x2, y2, colour);
	DrawVLine(bmpDest, y, y2, x, colour);
	DrawVLine(bmpDest, y, y2, x2, colour);
}

///////////////////
// Draws a rectangle with transparency
// WARNING: not threadsafe
inline void DrawRectFillA(SDL_Surface *bmpDest, int x, int y, int x2, int y2, Uint32 color, Uint8 alpha)  {
	SDL_Surface *tmp = gfxCreateSurfaceAlpha(x2-x,y2-y);
	static Uint8 r,g,b;
	GetColour3(color,bmpDest,&r,&g,&b);
	Uint32 friendly_col = SDL_MapRGBA(tmp->format,r,g,b,alpha);
	if (tmp)  {
		// TODO: optimise
		SDL_FillRect(tmp,NULL,friendly_col);
		DrawImage(bmpDest,tmp,x,y);
		SDL_FreeSurface(tmp);
	}
}

//////////////////
// Draw a triangle
inline void DrawTriangle(SDL_Surface *bmpDest, int x1, int y1, int x2, int y2, int x3, int y3, Uint32 colour) {
	DrawLine(bmpDest, x1, y1, x2, y2, colour);
	DrawLine(bmpDest, x2, y2, x3, y3, colour);
	DrawLine(bmpDest, x3, y3, x1, y1, colour);
}



//
// Special lines (rope, laser sight, beam)
//

void	DrawRope(SDL_Surface *bmp, int x1, int y1, int x2, int y2, Uint32 color);
void	DrawBeam(SDL_Surface *bmp, int x1, int y1, int x2, int y2, Uint32 color);
void	DrawLaserSight(SDL_Surface *bmp, int x1, int y1, int x2, int y2, Uint32 color);


//
// Colorkey handling
//

/////////////////
// Set colorkey for an alpha blended surface
void SetColorKeyAlpha(SDL_Surface *dst, Uint8 r, Uint8 g, Uint8 b);

//////////////////
// Set's the game's default color key (pink) to the surface
// Works for both alpha and nonalpha surfaces
inline void SetColorKey(SDL_Surface* dst)  {
	if (dst->flags & SDL_SRCALPHA)
		SetColorKeyAlpha(dst, 255, 0, 255);
	else
		SDL_SetColorKey(dst, SDL_SRCCOLORKEY, SDL_MapRGB(dst->format, 255, 0, 255)); 
}

//////////////////
// Resets the alpha-channel and the colorkey
inline void ResetAlpha(SDL_Surface* dst) {
	SDL_SetColorKey(dst, 0, 0); // Remove the colorkey
	SDL_SetAlpha(dst, 0, 0); // Remove the alpha
	
	int x, y;
	for(y = 0; y < dst->h; y++)
		for(x = 0; x < dst->w; x++)
			PutPixel(dst, x, y, GetPixel(dst, x, y) | dst->format->Amask);
}

#endif  //  __GFXPRIMITIVES_H__
