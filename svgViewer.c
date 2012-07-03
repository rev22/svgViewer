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

static int verbose_flag;
static int debug_flag;

static const struct option long_options[] = {
  /* These options set a flag. */
  {"verbose", no_argument,       &verbose_flag, 1},
  {"debug",   no_argument,       &debug_flag, 1},
  {0, 0, 0, 0}
};

#define FAIL(x...) do { fprintf(stderr, x); exit(EXIT_FAILURE); } while (0)
#define DEBUG(x...) do { if (debug_flag) fprintf(stderr, x); } while (0)

int main(int argc, char *argv[]) {
	SDL_Surface *screen;

	GError *error = NULL;
	RsvgHandle *handle;
	RsvgDimensionData dim;
	int width, height;
	char *filename;
	//const char *output_filename = argv[2];
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_t *cr2;
	cairo_status_t status;
	int c;

	/* Process options */
	while (1) {
	  int option_index = 0;
	  
	  c = getopt_long (argc, argv, "d:v:",
			   long_options, &option_index);
	  
	  /* Detect the end of the options. */
	  if (c == -1) break;
	}
     
	if (argc != optind+1) FAIL("Usage: %s OPTIONS input_file.svg\n", argv[0]);
	filename = argv[optind];

	g_type_init();

	rsvg_set_default_dpi(72.0);
	handle = rsvg_handle_new_from_file(filename, &error);
	if (error != NULL)
		FAIL(error->message);

	rsvg_handle_get_dimensions(handle, &dim);
	width = dim.width;
	height = dim.height;

	/* Initialize SDL, open a screen */
	DEBUG("Initializing SDL screen: %dx%d\n", width, height);
	screen = init_screen(width, height, 32);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cr2 = cairo_create (surface); 
	//surface = cairo_pdf_surface_create (output_filename, width, height);
	// SDL_LockSurface(screen);
	{
		//cairo_scale (cr, screen->w, screen->h);

	  {  
	    rsvg_handle_render_cairo(handle, cr2);
	    
	    status = cairo_status(cr2);
	    if (status)
	      FAIL(cairo_status_to_string(status));
	    
	    cr = cairosdl_create(screen);
	  }

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
	}
	// SDL_UnlockSurface(screen);
	SDL_UpdateRect(screen, 0, 0, 0, 0);

	{
	  SDL_Event event;
	  while (SDL_WaitEvent(&event)) {
	    switch (event.type) {
	    case SDL_KEYDOWN:
	      switch (event.key.keysym.sym) {
	      case SDLK_ESCAPE:
		goto exit;
	      default:
		break;
	      }
	      break;
	    case SDL_VIDEORESIZE:
	      {
		width = event.resize.w;
		height = event.resize.h;
		cairo_surface_destroy(surface);
		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
		cairo_destroy(cr2);
		cr2 = cairo_create (surface);
		cairo_scale(cr2, ((double)width)/dim.width, ((double)height)/dim.height);
		rsvg_handle_render_cairo(handle, cr2);
		cairosdl_destroy(cr);
		screen = init_screen(width, height, 32);
		cr = cairosdl_create(screen);
		cairo_save(cr);
		cairo_set_source_surface (cr, surface, 0, 0);
		cairo_paint(cr);
		cairo_restore(cr);
	      };
	      break;
	    case SDL_QUIT:
	      goto exit;
	    default:
	      break;
	    }
	    SDL_UpdateRect(screen, 0, 0, 0, 0);
	  };
	};

 exit:
	cairosdl_destroy(cr);
	if (status)
		FAIL(cairo_status_to_string(status));

	cairo_destroy (cr2);
	SDL_Quit();
	exit(EXIT_SUCCESS);
}
