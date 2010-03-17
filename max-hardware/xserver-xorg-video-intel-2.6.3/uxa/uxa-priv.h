/*
 *
 * Copyright © 2000,2008 Keith Packard
 *             2005 Zack Rusin, Trolltech
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef UXAPRIV_H
#define UXAPRIV_H

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#else
#include <xorg-server.h>
#endif
#include "xf86.h"

#include "uxa.h"

#include <X11/X.h>
#define NEED_EVENTS
#include <X11/Xproto.h>
#ifdef MITSHM
#define _XSHM_SERVER_
#include <X11/extensions/shmstr.h>
#endif
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mibstore.h"
#include "colormapst.h"
#include "gcstruct.h"
#include "input.h"
#include "mipointer.h"
#include "mi.h"
#include "dix.h"
#include "fb.h"
#include "fboverlay.h"
#ifdef RENDER
//#include "fbpict.h"
#include "glyphstr.h"
#endif
#include "damage.h"

#define DEBUG_TRACE_FALL	0
#define DEBUG_MIGRATE		0
#define DEBUG_PIXMAP		0
#define DEBUG_OFFSCREEN		0
#define DEBUG_GLYPH_CACHE	0

#if DEBUG_TRACE_FALL
#define UXA_FALLBACK(x)     					\
do {								\
	ErrorF("UXA fallback at %s: ", __FUNCTION__);		\
	ErrorF x;						\
} while (0)

char
uxa_drawable_location(DrawablePtr pDrawable);
#else
#define UXA_FALLBACK(x)
#endif

#if DEBUG_PIXMAP
#define DBG_PIXMAP(a) ErrorF a
#else
#define DBG_PIXMAP(a)
#endif

typedef struct {
    unsigned char sha1[20];
} uxa_cached_glyph_t;

typedef struct {
    /* The identity of the cache, statically configured at initialization */
    unsigned int format;
    int glyphWidth;
    int glyphHeight;

    int size; /* Size of cache; eventually this should be dynamically determined */

    /* Hash table mapping from glyph sha1 to position in the glyph; we use
     * open addressing with a hash table size determined based on size and large
     * enough so that we always have a good amount of free space, so we can
     * use linear probing. (Linear probing is preferrable to double hashing
     * here because it allows us to easily remove entries.)
     */
    int *hashEntries;
    int hashSize;
    
    uxa_cached_glyph_t *glyphs;
    int glyphCount; /* Current number of glyphs */
    
    PicturePtr picture;   /* Where the glyphs of the cache are stored */
    int yOffset;          /* y location within the picture where the cache starts */
    int columns;          /* Number of columns the glyphs are layed out in */
    int evictionPosition; /* Next random position to evict a glyph */
} uxa_glyph_cache_t;

#define UXA_NUM_GLYPH_CACHES 4

typedef void (*EnableDisableFBAccessProcPtr)(int, Bool);
typedef struct {
    uxa_driver_t		*info;
    CreateGCProcPtr 		 SavedCreateGC;
    CloseScreenProcPtr 		 SavedCloseScreen;
    GetImageProcPtr 		 SavedGetImage;
    GetSpansProcPtr 		 SavedGetSpans;
    CreatePixmapProcPtr 	 SavedCreatePixmap;
    DestroyPixmapProcPtr 	 SavedDestroyPixmap;
    CopyWindowProcPtr 		 SavedCopyWindow;
    ChangeWindowAttributesProcPtr SavedChangeWindowAttributes;
    BitmapToRegionProcPtr        SavedBitmapToRegion;
#ifdef RENDER
    CompositeProcPtr             SavedComposite;
    TrianglesProcPtr		 SavedTriangles;
    GlyphsProcPtr                SavedGlyphs;
    TrapezoidsProcPtr            SavedTrapezoids;
    AddTrapsProcPtr		 SavedAddTraps;
#endif
    EnableDisableFBAccessProcPtr SavedEnableDisableFBAccess;

    Bool			 swappedOut;
    unsigned			 disableFbCount;
    unsigned			 offScreenCounter;

    uxa_glyph_cache_t             glyphCaches[UXA_NUM_GLYPH_CACHES];
} uxa_screen_t;

/*
 * This is the only completely portable way to
 * compute this info.
 */
#ifndef BitsPerPixel
#define BitsPerPixel(d) (\
    PixmapWidthPaddingInfo[d].notPower2 ? \
    (PixmapWidthPaddingInfo[d].bytesPerPixel * 8) : \
    ((1 << PixmapWidthPaddingInfo[d].padBytesLog2) * 8 / \
    (PixmapWidthPaddingInfo[d].padRoundUp+1)))
#endif

extern int uxa_screen_index;
#define uxa_get_screen(s) ((uxa_screen_t *)dixLookupPrivate(&(s)->devPrivates, &uxa_screen_index))

/** Align an offset to an arbitrary alignment */
#define UXA_ALIGN(offset, align) (((offset) + (align) - 1) - \
	(((offset) + (align) - 1) % (align)))
/** Align an offset to a power-of-two alignment */
#define UXA_ALIGN2(offset, align) (((offset) + (align) - 1) & ~((align) - 1))

typedef struct {
    INT16 xSrc;
    INT16 ySrc;
    INT16 xDst;
    INT16 yDst;
    INT16 width;
    INT16 height;
} uxa_composite_rect_t;

/**
 * exaDDXDriverInit must be implemented by the DDX using EXA, and is the place
 * to set EXA options or hook in screen functions to handle using EXA as the AA.
  */
void exaDDXDriverInit (ScreenPtr pScreen);

Bool
uxa_prepare_access_window(WindowPtr pWin);

void
uxa_finish_access_window(WindowPtr pWin);

/* uxa-unaccel.c */
Bool
uxa_prepare_access_gc(GCPtr pGC);

void
uxa_finish_access_gc(GCPtr pGC);

void
uxa_check_fill_spans  (DrawablePtr pDrawable, GCPtr pGC, int nspans,
		   DDXPointPtr ppt, int *pwidth, int fSorted);

void
uxa_check_set_spans (DrawablePtr pDrawable, GCPtr pGC, char *psrc,
		 DDXPointPtr ppt, int *pwidth, int nspans, int fSorted);

void
uxa_check_put_image (DrawablePtr pDrawable, GCPtr pGC, int depth,
		 int x, int y, int w, int h, int leftPad, int format,
		 char *bits);

RegionPtr
uxa_check_copy_area (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		 int srcx, int srcy, int w, int h, int dstx, int dsty);

RegionPtr
uxa_check_copy_plane (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		  int srcx, int srcy, int w, int h, int dstx, int dsty,
		  unsigned long bitPlane);

void
uxa_check_poly_point (DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		  DDXPointPtr pptInit);

void
uxa_check_poly_lines (DrawablePtr pDrawable, GCPtr pGC,
		  int mode, int npt, DDXPointPtr ppt);

void
uxa_check_poly_segment (DrawablePtr pDrawable, GCPtr pGC,
		    int nsegInit, xSegment *pSegInit);

void
uxa_check_poly_arc (DrawablePtr pDrawable, GCPtr pGC,
		int narcs, xArc *pArcs);

void
uxa_check_poly_fill_rect (DrawablePtr pDrawable, GCPtr pGC,
		     int nrect, xRectangle *prect);

void
uxa_check_image_glyph_blt (DrawablePtr pDrawable, GCPtr pGC,
		      int x, int y, unsigned int nglyph,
		      CharInfoPtr *ppci, pointer pglyphBase);

void
uxa_check_poly_glyph_blt (DrawablePtr pDrawable, GCPtr pGC,
		     int x, int y, unsigned int nglyph,
		     CharInfoPtr *ppci, pointer pglyphBase);

void
uxa_check_push_pixels (GCPtr pGC, PixmapPtr pBitmap,
		   DrawablePtr pDrawable,
		   int w, int h, int x, int y);

void
uxa_check_get_spans (DrawablePtr pDrawable,
		 int wMax,
		 DDXPointPtr ppt,
		 int *pwidth,
		 int nspans,
		 char *pdstStart);

void
uxa_check_add_traps (PicturePtr	pPicture,
		  INT16		x_off,
		  INT16		y_off,
		  int		ntrap,
		  xTrap		*traps);

/* uxa-accel.c */

static _X_INLINE Bool
uxa_gc_reads_destination(DrawablePtr pDrawable, unsigned long planemask,
		      unsigned int fillStyle, unsigned char alu)
{
    return ((alu != GXcopy && alu != GXclear &&alu != GXset &&
	     alu != GXcopyInverted) || fillStyle == FillStippled ||
	    !UXA_PM_IS_SOLID(pDrawable, planemask));
}

void
uxa_copy_window(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc);

Bool
uxa_fill_region_tiled (DrawablePtr	pDrawable, RegionPtr pRegion, PixmapPtr pTile,
		    DDXPointPtr pPatOrg, CARD32 planemask, CARD32 alu);

void
uxa_shm_put_image(DrawablePtr pDrawable, GCPtr pGC, int depth, unsigned int format,
	       int w, int h, int sx, int sy, int sw, int sh, int dx, int dy,
	       char *data);

void
uxa_get_image (DrawablePtr pDrawable, int x, int y, int w, int h,
	     unsigned int format, unsigned long planeMask, char *d);

extern const GCOps uxa_ops;

#ifdef MITSHM
extern ShmFuncs uxa_shm_funcs;

/* XXX these come from shmint.h, which isn't exported by the server */
void
ShmRegisterFuncs(ScreenPtr pScreen, ShmFuncsPtr funcs);

void
ShmSetPixmapFormat(ScreenPtr pScreen, int format);

void
fbShmPutImage(XSHM_PUT_IMAGE_ARGS);

#endif

#ifdef RENDER

/* XXX these are in fbpict.h, which is not installed */
void
fbComposite (CARD8      op,
	     PicturePtr pSrc,
	     PicturePtr pMask,
	     PicturePtr pDst,
	     INT16      xSrc,
	     INT16      ySrc,
	     INT16      xMask,
	     INT16      yMask,
	     INT16      xDst,
	     INT16      yDst,
	     CARD16     width,
	     CARD16     height);

void
fbAddTraps (PicturePtr	pPicture,
	    INT16	xOff,
	    INT16	yOff,
	    int		ntrap,
	    xTrap	*traps);

void
uxa_check_composite (CARD8      op,
		  PicturePtr pSrc,
		  PicturePtr pMask,
		  PicturePtr pDst,
		  INT16      xSrc,
		  INT16      ySrc,
		  INT16      xMask,
		  INT16      yMask,
		  INT16      xDst,
		  INT16      yDst,
		  CARD16     width,
		  CARD16     height);
#endif

/* uxa.c */
Bool
uxa_prepare_access(DrawablePtr pDrawable, uxa_access_t access);

void
uxa_finish_access(DrawablePtr pDrawable);

void
uxa_get_drawable_deltas (DrawablePtr pDrawable, PixmapPtr pPixmap,
		      int *xp, int *yp);

Bool
uxa_drawable_is_offscreen (DrawablePtr pDrawable);

Bool
uxa_pixmap_is_offscreen(PixmapPtr p);

PixmapPtr
uxa_get_offscreen_pixmap (DrawablePtr pDrawable, int *xp, int *yp);

PixmapPtr
uxa_get_drawable_pixmap(DrawablePtr pDrawable);

RegionPtr
uxa_copy_area(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	    int srcx, int srcy, int width, int height, int dstx, int dsty);

void
uxa_copy_n_to_n (DrawablePtr    pSrcDrawable,
	     DrawablePtr    pDstDrawable,
	     GCPtr	    pGC,
	     BoxPtr	    pbox,
	     int	    nbox,
	     int	    dx,
	     int	    dy,
	     Bool	    reverse,
	     Bool	    upsidedown,
	     Pixel	    bitplane,
	     void	    *closure);

/* uxa_render.c */
Bool
uxa_op_reads_destination (CARD8 op);

void
uxa_composite(CARD8	op,
	     PicturePtr pSrc,
	     PicturePtr pMask,
	     PicturePtr pDst,
	     INT16	xSrc,
	     INT16	ySrc,
	     INT16	xMask,
	     INT16	yMask,
	     INT16	xDst,
	     INT16	yDst,
	     CARD16	width,
	     CARD16	height);

void
uxa_composite_rects(CARD8	            op,
		    PicturePtr		    pSrc,
		    PicturePtr		    pDst,
		    int			    nrect,
		    uxa_composite_rect_t    *rects);

void
uxa_trapezoids (CARD8 op, PicturePtr pSrc, PicturePtr pDst,
		PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
		int ntrap, xTrapezoid *traps);

void
uxa_triangles (CARD8 op, PicturePtr pSrc, PicturePtr pDst,
	       PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
	       int ntri, xTriangle *tris);

/* uxa_glyph.c */
void
uxa_glyphs_init(ScreenPtr pScreen);

void
uxa_glyphs_fini (ScreenPtr pScreen);

void
uxa_glyphs (CARD8	op,
	  PicturePtr	pSrc,
	  PicturePtr	pDst,
	  PictFormatPtr	maskFormat,
	  INT16		xSrc,
	  INT16		ySrc,
	  int		nlist,
	  GlyphListPtr	list,
	  GlyphPtr	*glyphs);

#endif /* UXAPRIV_H */