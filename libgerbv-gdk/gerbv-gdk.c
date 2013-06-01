/*
 * gEDA - GNU Electronic Design Automation
 * This file is a part of gerbv.
 *
 *   Copyright (C) 2000-2003 Stefan Petersen (spe@stacken.kth.se)
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */
 
 
/** \file gerbv-gdk.c
    \brief This file contains high-level functions for the libgerbv library
    \ingroup libgerbv
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h> /* dirname */
#endif
#include <errno.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <pango/pango.h>

#include <locale.h>

#include "common.h"
#include "gerbv.h"
#include "gerber.h"
#include "drill.h"

#include "draw-gdk.h"
#include "draw.h"

#include "pick-and-place.h"

/* DEBUG printing.  #define DEBUG 1 in config.h to use this fcn. */
#define dprintf if(DEBUG) printf

/* ------------------------------------------------------------------ */
void
gerbv_render_to_pixmap_using_gdk (gerbv_project_t *gerbvProject, GdkPixmap *pixmap,
		gerbv_render_info_t *renderInfo, gerbv_selection_info_t *selectionInfo,
		GdkColor *selectionColor){
	GdkGC *gc = gdk_gc_new(pixmap);
	GdkPixmap *colorStamp, *clipmask;
	int i;
	GdkColor bg, layer;
	
	/* 
	 * Remove old pixmap, allocate a new one, draw the background.
	 */
	bg.pixel = 0;
	bg.red = 65535 * gerbvProject->background.red;
	bg.green = 65535 * gerbvProject->background.green;
	bg.blue = 65535 * gerbvProject->background.blue;
	gdk_colormap_alloc_color(gdk_colormap_get_system(), &bg, FALSE, TRUE);
	gdk_gc_set_foreground(gc, &bg);
	gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, -1, -1);

	/*
	 * Allocate the pixmap and the clipmask (a one pixel pixmap)
	 */
	colorStamp = gdk_pixmap_new(pixmap, renderInfo->displayWidth,
						renderInfo->displayHeight, -1);
	clipmask = gdk_pixmap_new(NULL, renderInfo->displayWidth,
						renderInfo->displayHeight, 1);
							
	/* 
	* This now allows drawing several layers on top of each other.
	* Higher layer numbers have higher priority in the Z-order. 
	*/
	for(i = gerbvProject->last_loaded; i >= 0; i--) {
		if (gerbvProject->file[i] && gerbvProject->file[i]->isVisible) {
			/*
			* Fill up image with all the foreground color. Excess pixels
			* will be removed by clipmask.
			*/
			layer.pixel = 0;
			layer.red = 65535 * gerbvProject->file[i]->color.red;
			layer.green = 65535 * gerbvProject->file[i]->color.green;
			layer.blue = 65535 * gerbvProject->file[i]->color.blue;
	 		gdk_colormap_alloc_color(gdk_colormap_get_system(), &layer, FALSE, TRUE);
			gdk_gc_set_foreground(gc, &layer);
			
			/* switch back to regular draw function for the initial
			   bitmap clear */
			gdk_gc_set_function(gc, GDK_COPY);
			gdk_draw_rectangle(colorStamp, gc, TRUE, 0, 0, -1, -1);
			
			if (renderInfo->renderType == GERBV_RENDER_TYPE_GDK) {
				gdk_gc_set_function(gc, GDK_COPY);
			}
			else if (renderInfo->renderType == GERBV_RENDER_TYPE_GDK_XOR) {
				gdk_gc_set_function(gc, GDK_XOR);
			}
			/*
			* Translation is to get it inside the allocated pixmap,
			* which is not always centered perfectly for GTK/X.
			*/
			dprintf("  .... calling image2pixmap on image %d...\n", i);
			// Dirty scaling solution when using GDK; simply use scaling factor for x-axis, ignore y-axis
			draw_gdk_image_to_pixmap(&clipmask, gerbvProject->file[i]->image,
				renderInfo->scaleFactorX, -(renderInfo->lowerLeftX * renderInfo->scaleFactorX),
				(renderInfo->lowerLeftY * renderInfo->scaleFactorY) + renderInfo->displayHeight,
				DRAW_IMAGE, NULL, renderInfo, gerbvProject->file[i]->transform);

			/* 
			* Set clipmask and draw the clipped out image onto the
			* screen pixmap. Afterwards we remove the clipmask, else
			* it will screw things up when run this loop again.
			*/
			gdk_gc_set_clip_mask(gc, clipmask);
			gdk_gc_set_clip_origin(gc, 0, 0);
			gdk_draw_drawable(pixmap, gc, colorStamp, 0, 0, 0, 0, -1, -1);
			gdk_gc_set_clip_mask(gc, NULL);
		}
	}
	/* render the selection group to the top of the output */
	if ((selectionInfo) && (selectionInfo->type != GERBV_SELECTION_EMPTY)) {
		if (!selectionColor->pixel)
	 		gdk_colormap_alloc_color(gdk_colormap_get_system(), selectionColor, FALSE, TRUE);

		gdk_gc_set_foreground(gc, selectionColor);
		gdk_gc_set_function(gc, GDK_COPY);
		gdk_draw_rectangle(colorStamp, gc, TRUE, 0, 0, -1, -1);
		
		/* for now, assume everything in the selection buffer is from one image */
		gerbv_image_t *matchImage;
		int j;
		if (selectionInfo->selectedNodeArray->len > 0) {
			gerbv_selection_item_t sItem = g_array_index (selectionInfo->selectedNodeArray,
					gerbv_selection_item_t, 0);
			matchImage = (gerbv_image_t *) sItem.image;	

			for(j = gerbvProject->last_loaded; j >= 0; j--) {
				if ((gerbvProject->file[j]) && (gerbvProject->file[j]->image == matchImage)) {
					draw_gdk_image_to_pixmap(&clipmask, gerbvProject->file[j]->image,
						renderInfo->scaleFactorX, -(renderInfo->lowerLeftX * renderInfo->scaleFactorX),
						(renderInfo->lowerLeftY * renderInfo->scaleFactorY) + renderInfo->displayHeight,
						DRAW_SELECTIONS, selectionInfo,
						renderInfo, gerbvProject->file[j]->transform);
				}
			}
			gdk_gc_set_clip_mask(gc, clipmask);
			gdk_gc_set_clip_origin(gc, 0, 0);
			gdk_draw_drawable(pixmap, gc, colorStamp, 0, 0, 0, 0, -1, -1);
			gdk_gc_set_clip_mask(gc, NULL);
		}
	}

	gdk_pixmap_unref(colorStamp);
	gdk_pixmap_unref(clipmask);
	gdk_gc_unref(gc);
	gdk_colormap_free_colors(gdk_colormap_get_system(), &bg, 1);
	gdk_colormap_free_colors(gdk_colormap_get_system(), &layer, 1);
}

