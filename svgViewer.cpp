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

#include "./cairosdl/cairosdl.h"

#define FAIL(msg)							\
    do { fprintf (stderr, "FAIL: %s\n", msg); exit (-1); } while (0)

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
				  SDL_HWSURFACE | SDL_RESIZABLE);
	if (screen == NULL) {
		fprintf(stderr, "Unable to set %ix%i video: %s\n",
			width, height, SDL_GetError());
		exit(1);
	}

	SDL_WM_SetCaption("testing", "ICON");

	return screen;
}


int main(int argc, char *argv[]) {
	SDL_Surface *screen;

	GError *error = NULL;
	RsvgHandle *handle;
	RsvgDimensionData dim;
	double width, height;
	const char *filename = argv[1];
	//const char *output_filename = argv[2];
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_status_t status;

	if (argc != 2)
		FAIL("usage: svg2pdf input_file.svg");

	g_type_init();

	rsvg_set_default_dpi(72.0);
	handle = rsvg_handle_new_from_file(filename, &error);
	if (error != NULL)
		FAIL(error->message);

	rsvg_handle_get_dimensions(handle, &dim);
	width = dim.width;
	height = dim.height;

/* Initialize SDL, open a screen */
	screen = init_screen(width*2, height*2, 32);

	//surface = cairo_pdf_surface_create (output_filename, width, height);
	SDL_LockSurface(screen); {
		//cairo_scale (cr, screen->w, screen->h);

		cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
        	cairo_t *cr2 = cairo_create (surface); 
		
		
		rsvg_handle_render_cairo(handle, cr2);
		
		status = cairo_status(cr2);
		if (status)
			FAIL(cairo_status_to_string(status));
	
		cr = cairosdl_create(screen);

		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_paint(cr);

		cairo_save(cr);
		cairo_scale(cr, .5, .5);
		//cairo_translate(cr, width/2, height/2 );
		//cairo_rotate( cr, 3.14/2 );
		//cairo_translate(cr, -width/2, -height/2 );
		
		cairo_set_source_surface (cr, surface, 0, 0);
		cairo_paint(cr);

		cairo_restore(cr);
		cairo_scale(cr, .5, .5);
		//cairo_translate(cr, width/2, height/2 );
		//cairo_rotate( cr, 3.14*1.5 );
		//cairo_translate(cr, -width/2, -height/2 );
		
		cairo_set_source_surface (cr, surface, width, height);
		cairo_paint (cr);


		status = cairo_status(cr);
		if (status)
			FAIL(cairo_status_to_string(status));

		cairosdl_destroy(cr);
	}
	SDL_UnlockSurface(screen);
	SDL_Flip(screen);

	if (status)
		FAIL(cairo_status_to_string(status));

	//cairo_destroy (cr);





	while (true);

	return 0;
}
