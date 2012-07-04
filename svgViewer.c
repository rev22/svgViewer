/* gcc `pkg-config --cflags --libs librsvg-2.0 cairo-pdf` -o svg2pdf svg2pdf.c
 *
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors: Kristian Høgsberg <krh@redhat.com>
 *	    Carl Worth <cworth@redhat.com>
 *	    Behdad Esfahbod <besfahbo@redhat.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <getopt.h>
#include <errno.h>

#include "./cairosdl/cairosdl.h"

#define PIXELS_PER_POINT 1

static SDL_Surface *init_screen(int width, int height, int bpp) {
	SDL_Surface *screen;

	/* Initialize SDL */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Unable to initialize SDL: %s\n",
			SDL_GetError());
		exit(1);
	}

	/* Open a screen with the specified properties */
	screen = SDL_SetVideoMode(width, height, bpp,
				  SDL_SWSURFACE | SDL_RESIZABLE);
	if (screen == NULL) {
		fprintf(stderr, "Unable to set %ix%i video: %s\n",
			width, height, SDL_GetError());
		exit(1);
	}

	SDL_WM_SetCaption("testing", "ICON");

	return screen;
}

static int verbose_flag  = 0;
static int debug_flag    = 0;
static int stretch_flag  = 0;

static const struct option long_options[] = {
  /* These options set a flag. */
  {"verbose", no_argument,        &verbose_flag,  1},
  {"debug",   no_argument,        &debug_flag,    1},
  {"stretch",   no_argument,        &stretch_flag,  1},
  {"zoom",    required_argument,  NULL,           'z'},
  {0, 0, 0, 0}
};

#define FAIL(x...) do { fprintf(stderr, x); exit(EXIT_FAILURE); } while (0)
#define DEBUG(x...) do { if (debug_flag) fprintf(stderr, x); } while (0)

struct svgviewer_view {
  double x, y;
  double zoom;
  unsigned int pixel_width, pixel_height;
};

RsvgDimensionData dim;

void view_transform(cairo_t *c, struct svgviewer_view *v) {
  double pw = (double)v->pixel_width;
  double ph = (double)v->pixel_height;
  double iw = (double)dim.width;
  double ih = (double)dim.height;
  double cx = iw *  0.5;
  double cy = ih *  0.5;
  double zx = pw / iw;
  double zy = ph / ih;
  cx += v->x * dim.width;
  cy += v->y * dim.height;
  //cairo_translate(c, - pw * 0.5, - ph * 0.5);
  if (stretch_flag) {
    cairo_scale(c, zx *= v->zoom, zy *= v->zoom);
    cx *= zx; cy *= zy;
    cx = (pw * 0.5 - cx) / zx;
    cy = (ph * 0.5 - cy) / zy;
  } else {
    double z = (zx < zy) ? zx : zy;
    z *= v->zoom;
    cairo_scale(c, z, z);
    cx *= z; cy *= z;
    cx = (pw * 0.5 - cx) / z;
    cy = (ph * 0.5 - cy) / z;
  }
  cairo_translate(c, cx, cy);
}

int main(int argc, char *argv[]) {
	SDL_Surface *screen;

	GError *error = NULL;
	RsvgHandle *handle;
	struct svgviewer_view vw;
	char *filename;
	//const char *output_filename = argv[2];
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_t *cr2;
	cairo_status_t status;
	int c;
	int rerender = 0;

	vw.zoom = 1;

	/* Process options */
	while (1) {
	  int option_index = 0;
	  
	  c = getopt_long (argc, argv, "d:v:",
			   long_options, &option_index);

	  /* Detect the end of the options. */
	  if (c == -1) break;
	  switch (c) {
	  case 0:
	    break;
	  case 'z':
	    errno = 0;
	    vw.zoom = strtod(optarg, NULL);
	    DEBUG("Zoom set to %g\n", vw.zoom);
	    if (errno) FAIL("Usage: -z or --zoom requires a floating point argument");
	    break;
	  default:
	    abort();
	  };
	}
     
	if (argc != optind+1) FAIL("Usage: %s OPTIONS input_file.svg\n", argv[0]);
	filename = argv[optind];

	g_type_init();

	rsvg_set_default_dpi(72.0);
	handle = rsvg_handle_new_from_file(filename, &error);
	if (error != NULL)
		FAIL(error->message);

	rsvg_handle_get_dimensions(handle, &dim);
	vw.x = 0; vw.y = 0;
	vw.pixel_width   = dim.width;
	vw.pixel_height  = dim.height;
	vw.zoom    = 1;

	/* Initialize SDL, open a screen */
	DEBUG("Initializing SDL screen: %dx%d\n", vw.pixel_width, vw.pixel_height);
	screen = init_screen(vw.pixel_width, vw.pixel_height, 32);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, vw.pixel_width, vw.pixel_height);
	cr2 = cairo_create (surface); 
	//cairo_translate(cr2, ((double)dim.width)/-2, ((double)dim.height)/-2);
	//surface = cairo_pdf_surface_create (output_filename, width, height);
	// SDL_LockSurface(screen);
	//cairo_scale (cr, screen->w, screen->h);
	cairo_save(cr2);
	view_transform(cr2, &vw);
	rsvg_handle_render_cairo(handle, cr2);
	status = cairo_status(cr2);
	if (status) FAIL(cairo_status_to_string(status));
	cairo_restore(cr2);
	cr = cairosdl_create(screen);

	// cairo_set_source_rgb(cr, 1, 1, 1);
	// cairo_paint(cr);
	
	cairo_save(cr);
	//cairo_scale(cr, .5, .5);
	//cairo_translate(cr, width/2, height/2 );
	//cairo_rotate( cr, 3.14/2 );
	//cairo_translate(cr, -width/2, -height/2 );
	cairo_set_source_surface (cr, surface, 0, 0);
	cairo_paint(cr);
	cairo_restore(cr);
	
	status = cairo_status(cr);
	if (status)
	  FAIL(cairo_status_to_string(status));
	
	// SDL_UnlockSurface(screen);
	SDL_UpdateRect(screen, 0, 0, 0, 0);

	{
	  SDL_Event event;
	  while (SDL_WaitEvent(&event)) {
	    switch (event.type) {
	    case SDL_KEYDOWN:
	      switch (event.key.keysym.sym) {
	      case SDLK_UP:
		vw.y -= 0.1 / vw.zoom;
		rerender = 1;
		break;
	      case SDLK_DOWN:
		vw.y += 0.1 / vw.zoom;
		rerender = 1;
		break;
	      case SDLK_LEFT:
		vw.x -= 0.1 / vw.zoom;
		rerender = 1;
		break;
	      case SDLK_RIGHT:
		vw.x += 0.1 / vw.zoom;
		rerender = 1;
		break;
	      case SDLK_a:
		vw.zoom *= 1.025;
		rerender = 1;
		break;
	      case SDLK_z:
		vw.zoom /= 1.025;
		rerender = 1;
		break;
	      case SDLK_ESCAPE:
		goto exit;
	      default:
		break;
	      }
	      break;
	    case SDL_VIDEORESIZE:
	      {
		vw.pixel_width = event.resize.w;
		vw.pixel_height = event.resize.h;
		cairo_destroy(cr2);
		cairo_surface_destroy(surface);
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, vw.pixel_width, vw.pixel_height);
		cr2 = cairo_create(surface);
		//cairo_translate(cr2, ((double)dim.width)/-2, ((double)dim.height)/-2);
		cairo_save(cr2);
		view_transform(cr2, &vw);
		rsvg_handle_render_cairo(handle, cr2);
		cairo_restore(cr2);
		screen = init_screen(vw.pixel_width, vw.pixel_height, 32);
		cairosdl_destroy(cr);
		cr = cairosdl_create(screen);
		cairo_save(cr);
		cairo_set_source_surface(cr, surface, 0, 0);
		cairo_paint(cr);
		cairo_restore(cr);
	      };
	      break;
	    case SDL_QUIT:
	      goto exit;
	    default:
	      break;
	    }
	    if (rerender) {
		cairo_save(cr2);
		view_transform(cr2, &vw);
		rsvg_handle_render_cairo(handle, cr2);
		status = cairo_status(cr2);
		if (status) FAIL(cairo_status_to_string(status));
		cairo_restore(cr2);
		cairo_set_source_surface(cr, surface, 0, 0);
		cairo_paint(cr);
		rerender = 0;
	    }
	    SDL_UpdateRect(screen, 0, 0, 0, 0);
	  };
	};

 exit:
	cairosdl_destroy(cr);
	if (status)
		FAIL(cairo_status_to_string(status));

	cairo_destroy (cr2);
	cairo_surface_destroy(surface);
	SDL_Quit();
	exit(EXIT_SUCCESS);
}
