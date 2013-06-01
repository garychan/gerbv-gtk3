/*
 * gEDA - GNU Electronic Design Automation
 *
 * render.c -- this file is a part of gerbv.
 *
 *   Copyright (C) 2007 Stuart Brorson (SDB@cloud9.net)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/** \file render.c
    \brief Rendering support functions for libgerbv
    \ingroup libgerbv
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_LIBGEN_H
#include <libgen.h> /* dirname */
#endif

#include <math.h>

#include "common.h"
#include "gerbv.h"
#include "main.h"
#include "callbacks.h"
#include "interface.h"
#include "render.h"

#include <cairo.h>
#include "draw.h"

#define dprintf if(DEBUG) printf

/**Global variable to keep track of what's happening on the screen.
   Declared extern in gerbv_screen.h
 */
extern gerbv_screen_t screen;

extern gerbv_render_info_t screenRenderInfo;

/*
static void
render_layer_to_cairo_target_without_transforming(cairo_t *cr, gerbv_fileinfo_t *fileInfo, gerbv_render_info_t *renderInfo );
*/

static void
stroke_tool(cairo_t *cr) {
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

	cairo_set_line_width(cr, 3.0);
	cairo_set_source_rgba(cr, screen.tool_outline_color.red,
		screen.tool_outline_color.green,
		screen.tool_outline_color.blue,
		screen.tool_outline_color.alpha);
	cairo_stroke_preserve(cr);

	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgba(cr, screen.tool_color.red,
		screen.tool_color.green,
		screen.tool_color.blue,
		screen.tool_color.alpha);
	cairo_stroke(cr);
}

static void
draw_tool_handle(cairo_t *cr, gdouble x, gdouble y) {
	cairo_move_to(cr, x, y);
	cairo_rel_move_to(cr, -10, 0);
	cairo_rel_line_to(cr, 20, 0);

	cairo_rel_move_to(cr, -10, -10);
	cairo_rel_line_to(cr, 0, 20);

	stroke_tool(cr);
}

/* Cairo uses a different notion of pixel origin. To get a perfect line
 * without aliasing, you have to draw from the center of a pixel.
 *
 * The coordinate must be rounded to choose the best pixel */
static void
choose_nearest_pixel(gdouble *c) {
	double nearest = rint(*c);

	*c = nearest + 0.5;
}

gboolean
render_check_scale_factor_limits (void) {
	if ((screenRenderInfo.scaleFactorX > 20000)||(screenRenderInfo.scaleFactorY > 20000)) {
		screenRenderInfo.scaleFactorX = 20000;
		screenRenderInfo.scaleFactorY = 20000;
		return FALSE;
	}
	return TRUE;
}

/* ------------------------------------------------------ */
void
render_zoom_display (gint zoomType, gdouble scaleFactor, gdouble mouseX, gdouble mouseY) {
	/*double us_midx, us_midy;*/	/* unscaled translation for screen center */
	/*int half_w, half_h;*/		/* cache for half window dimensions */
	gdouble mouseCoordinateX = 0.0;
	gdouble mouseCoordinateY = 0.0;
	double oldWidth, oldHeight;

	/*
	half_w = screenRenderInfo.displayWidth / 2;
	half_h = screenRenderInfo.displayHeight / 2;
	*/
	
	oldWidth = screenRenderInfo.displayWidth / screenRenderInfo.scaleFactorX;
	oldHeight = screenRenderInfo.displayHeight / screenRenderInfo.scaleFactorY;
	if (zoomType == ZOOM_IN_CMOUSE || zoomType == ZOOM_OUT_CMOUSE) {
		/* calculate what user coordinate the mouse is pointing at */
		mouseCoordinateX = mouseX / screenRenderInfo.scaleFactorX + screenRenderInfo.lowerLeftX;
		mouseCoordinateY = (screenRenderInfo.displayHeight - mouseY) /
			screenRenderInfo.scaleFactorY + screenRenderInfo.lowerLeftY;
	}

	/*
	us_midx = screenRenderInfo.lowerLeftX + (screenRenderInfo.displayWidth / 2.0 )/
			screenRenderInfo.scaleFactorX;
	us_midy = screenRenderInfo.lowerLeftY + (screenRenderInfo.displayHeight / 2.0 )/
			screenRenderInfo.scaleFactorY;		
	*/
	
	switch(zoomType) {
		case ZOOM_IN : /* Zoom In */
		case ZOOM_IN_CMOUSE : /* Zoom In Around Mouse Pointer */
			screenRenderInfo.scaleFactorX += screenRenderInfo.scaleFactorX/3;
			screenRenderInfo.scaleFactorY += screenRenderInfo.scaleFactorY/3;
			(void) render_check_scale_factor_limits ();
			screenRenderInfo.lowerLeftX += (oldWidth - (screenRenderInfo.displayWidth /
				screenRenderInfo.scaleFactorX)) / 2.0;
			screenRenderInfo.lowerLeftY += (oldHeight - (screenRenderInfo.displayHeight /
				screenRenderInfo.scaleFactorY)) / 2.0;	
			break;
		case ZOOM_OUT :  /* Zoom Out */
		case ZOOM_OUT_CMOUSE : /* Zoom Out Around Mouse Pointer */
			if ((screenRenderInfo.scaleFactorX > 10)&&(screenRenderInfo.scaleFactorY > 10)) {
				screenRenderInfo.scaleFactorX -= screenRenderInfo.scaleFactorX/3;
				screenRenderInfo.scaleFactorY -= screenRenderInfo.scaleFactorY/3;
				screenRenderInfo.lowerLeftX += (oldWidth - (screenRenderInfo.displayWidth /
					screenRenderInfo.scaleFactorX)) / 2.0;
				screenRenderInfo.lowerLeftY += (oldHeight - (screenRenderInfo.displayHeight /
					screenRenderInfo.scaleFactorY)) / 2.0;
			}
			break;
		case ZOOM_FIT : /* Zoom Fit */
			gerbv_render_zoom_to_fit_display (mainProject, &screenRenderInfo);
			break;
		case ZOOM_SET : /*explicit scale set by user */
			screenRenderInfo.scaleFactorX = scaleFactor;
			screenRenderInfo.scaleFactorY = scaleFactor;
			(void) render_check_scale_factor_limits ();
			screenRenderInfo.lowerLeftX += (oldWidth - (screenRenderInfo.displayWidth /
				screenRenderInfo.scaleFactorX)) / 2.0;
			screenRenderInfo.lowerLeftY += (oldHeight - (screenRenderInfo.displayHeight /
				screenRenderInfo.scaleFactorY)) / 2.0;
		      break;
		default :
			GERB_MESSAGE("Illegal zoom direction %d\n", zoomType);
	}

	if (zoomType == ZOOM_IN_CMOUSE || zoomType == ZOOM_OUT_CMOUSE) {
		/* make sure the mouse is still pointing at the point calculated earlier */
		screenRenderInfo.lowerLeftX = mouseCoordinateX - mouseX / screenRenderInfo.scaleFactorX;
		screenRenderInfo.lowerLeftY = mouseCoordinateY - (screenRenderInfo.displayHeight - mouseY) /
			screenRenderInfo.scaleFactorY;
	}
	render_refresh_rendered_image_on_screen();
	return;
}


/* --------------------------------------------------------- */
/** Will determine the outline of the zoomed regions.
 * In case region to be zoomed is too small (which correspondes
 * e.g. to a double click) it is interpreted as a right-click 
 * and will be used to identify a part from the CURRENT selection, 
 * which is drawn on screen*/
void
render_calculate_zoom_from_outline(GtkWidget *widget, GdkEventButton *event)
{
	int x1, y1, x2, y2, dx, dy;	/* Zoom outline (UR and LL corners) */
	double centerPointX, centerPointY;
	int half_x, half_y;		/* cache for half window dimensions */

	x1 = MIN(screen.start_x, event->x);
	y1 = MIN(screen.start_y, event->y);
	x2 = MAX(screen.start_x, event->x);
	y2 = MAX(screen.start_y, event->y);
	dx = x2-x1;
	dy = y2-y1;

	if ((dx >= 4) && (dy >= 4)) {
		if (screen.centered_outline_zoom) {
			/* Centered outline mode */
			x1 = screen.start_x - dx;
			y1 = screen.start_y - dy;
			dx *= 2;
			dy *= 2;
		}
		half_x = (x1+x2)/2;
		half_y = (y1+y2)/2;
		centerPointX = half_x/screenRenderInfo.scaleFactorX + screenRenderInfo.lowerLeftX;
		centerPointY = (screenRenderInfo.displayHeight - half_y)/screenRenderInfo.scaleFactorY +
				screenRenderInfo.lowerLeftY;
		
		screenRenderInfo.scaleFactorX *= MIN(((double)screenRenderInfo.displayWidth / dx),
							((double)screenRenderInfo.displayHeight / dy));
		screenRenderInfo.scaleFactorY = screenRenderInfo.scaleFactorX;
		(void) render_check_scale_factor_limits ();
		screenRenderInfo.lowerLeftX = centerPointX - (screenRenderInfo.displayWidth /
					2.0 / screenRenderInfo.scaleFactorX);
		screenRenderInfo.lowerLeftY = centerPointY - (screenRenderInfo.displayHeight /
					2.0 / screenRenderInfo.scaleFactorY);				
	}
	render_refresh_rendered_image_on_screen();
}

/* ------------------------------------------------------ */
void
render_draw_selection_box_outline(void) {

        cairo_t *cr;
	gdouble x1, y1, x2, y2, dx, dy;

	x1 = MIN(screen.start_x, screen.last_x);
	y1 = MIN(screen.start_y, screen.last_y);
	x2 = MAX(screen.start_x, screen.last_x);
	y2 = MAX(screen.start_y, screen.last_y);
	dx = x2-x1;
	dy = y2-y1;

	choose_nearest_pixel(&x1);
	choose_nearest_pixel(&y1);

	if (dx > 0)
		dx -= 1;
	if (dy > 0)
		dy -= 1;

        cr = gdk_cairo_create(gtk_widget_get_window(screen.drawing_area));

        cairo_rectangle(cr, x1, y1, dx, dy);
        stroke_tool(cr);

        cairo_destroy(cr);
}

/* --------------------------------------------------------- */
void
render_draw_zoom_outline(gboolean centered)
{
        cairo_t *cr;
	gint x1, y1, x2, y2, dx, dy;
	gdouble x, y, w, h;

	x1 = MIN(screen.start_x, screen.last_x);
	y1 = MIN(screen.start_y, screen.last_y);
	x2 = MAX(screen.start_x, screen.last_x);
	y2 = MAX(screen.start_y, screen.last_y);
	dx = x2-x1;
	dy = y2-y1;

	if (centered) {
		/* Centered outline mode */
		x1 = screen.start_x - dx;
		y1 = screen.start_y - dy;
		dx *= 2;
		dy *= 2;
		x2 = x1+dx;
		y2 = y1+dy;
	}

        cr = gdk_cairo_create(gtk_widget_get_window(screen.drawing_area));

	/* effective zoom region (dashed) */

	if ((dy == 0) || ((double)dx/dy > (double)screen.drawing_area->allocation.width/
				screen.drawing_area->allocation.height)) {
		h = rint(dx * (double)screen.drawing_area->allocation.height/
			screen.drawing_area->allocation.width);
		w = dx;
	} 
	else {
		w = rint(dy * (double)screen.drawing_area->allocation.width/
			screen.drawing_area->allocation.height);
		h = dy;
	}

	x = (x1+x2-w)/2;
	y = (y1+y2-h)/2;
	choose_nearest_pixel(&x);
	choose_nearest_pixel(&y);

	if (w > 0)
		w -= 1;
	else
		w = 0;
	if (h > 0)
		h -= 1;
	else
		h = 0;

	{
		double dash[] = { 3.0, 5.0 };
		cairo_set_dash(cr, dash, 2, 3.0);
	}
	cairo_rectangle(cr, x, y, w, h);
        stroke_tool(cr);
	cairo_set_dash(cr, NULL, 0, 0.0);

	/* zoom region as dragged by the mouse (solid) */

	x = x1;
	y = y1;
	choose_nearest_pixel(&x);
	choose_nearest_pixel(&y);

	w = dx;
	h = dy;
	if (w > 0)
		w -= 1;
	else
		w = 0;
	if (h > 0)
		h -= 1;
	else
		h = 0;

	cairo_rectangle(cr, x, y, w, h);
        stroke_tool(cr);

        cairo_destroy(cr);
}

/* ------------------------------------------------------ */
/* Transforms board coordinates to screen ones */
static void
render_board2screen(gdouble *X, gdouble *Y, gdouble x, gdouble y) {
	*X = (x - screenRenderInfo.lowerLeftX) * screenRenderInfo.scaleFactorX;
	*Y = screenRenderInfo.displayHeight - (y - screenRenderInfo.lowerLeftY)
		* screenRenderInfo.scaleFactorY;
}

/* Trims the coordinates to avoid overflows in gdk_draw_line */
static void
render_trim_point(gdouble *start_x, gdouble *start_y, gdouble last_x, gdouble last_y)
{
	const gdouble max_coord = (1<<15) - 2;/* a value that causes no overflow
											 and lies out of screen */
	gdouble dx, dy;

    if (fabs (*start_x) < max_coord && fabs (*start_y) < max_coord)
		return;	

	dx = last_x - *start_x;
	dy = last_y - *start_y;

	if (*start_x < -max_coord) {
		*start_x = -max_coord;
		if (last_x > -max_coord && fabs (dx) > 0.1)
			*start_y = last_y - (last_x + max_coord) / dx * dy;
	}
	if (*start_x > max_coord) {
		*start_x = max_coord;
		if (last_x < max_coord && fabs (dx) > 0.1)
			*start_y = last_y - (last_x - max_coord) / dx * dy;
	}

	dx = last_x - *start_x;
	dy = last_y - *start_y;

	if (*start_y < -max_coord) {
		*start_y = -max_coord;
		if (last_y > -max_coord && fabs (dy) > 0.1)
			*start_x = last_x - (last_y + max_coord) / dy * dx;
	}
	if (*start_y > max_coord) {
		*start_y = max_coord;
		if (last_y < max_coord && fabs (dy) > 0.1)
			*start_x = last_x - (last_y - max_coord) / dy * dx;
	}
}

/* ------------------------------------------------------ */
/** Draws/erases measure line
 */
void
render_toggle_measure_line(void)
{
	cairo_t *cr;
	gdouble start_x, start_y, last_x, last_y;

	render_board2screen(&start_x, &start_y,
				screen.measure_start_x, screen.measure_start_y); 
	render_board2screen(&last_x, &last_y,
				screen.measure_last_x, screen.measure_last_y); 
	render_trim_point(&start_x, &start_y, last_x, last_y);
	render_trim_point(&last_x, &last_y, start_x, start_y);

	choose_nearest_pixel(&start_x);
	choose_nearest_pixel(&start_y);
	choose_nearest_pixel(&last_x);
	choose_nearest_pixel(&last_y);

	cr = gdk_cairo_create(gtk_widget_get_window(screen.drawing_area));

	cairo_move_to(cr, start_x, start_y);
	cairo_line_to(cr, last_x, last_y);

	stroke_tool(cr);

	draw_tool_handle(cr, start_x, start_y);
	draw_tool_handle(cr, last_x, last_y);

	cairo_destroy(cr);

} /* toggle_measure_line */

/* ------------------------------------------------------ */
/** Displays a measured distance graphically on screen and in statusbar. */
void
render_draw_measure_distance(void)
{
	gdouble x1, y1, x2, y2;
	gdouble dx, dy;

	x1 = MIN(screen.measure_start_x, screen.measure_last_x);
	y1 = MIN(screen.measure_start_y, screen.measure_last_y);
	x2 = MAX(screen.measure_start_x, screen.measure_last_x);
	y2 = MAX(screen.measure_start_y, screen.measure_last_y);
	dx = (x2 - x1);
	dy = (y2 - y1);
    
	screen.win.lastMeasuredX = dx;
	screen.win.lastMeasuredY = dy;
	callbacks_update_statusbar_measured_distance (dx, dy);
	render_toggle_measure_line();
} /* draw_measure_distance */

/* ------------------------------------------------------ */
void render_selection_layer (void){
	cairo_t *cr;
	
	if (screen.selectionRenderData) 
		cairo_surface_destroy ((cairo_surface_t *) screen.selectionRenderData);
	screen.selectionRenderData = 
		(gpointer) cairo_surface_create_similar ((cairo_surface_t *)screen.windowSurface,
		CAIRO_CONTENT_COLOR_ALPHA, screenRenderInfo.displayWidth,
		screenRenderInfo.displayHeight);
	if (screen.selectionInfo.type != GERBV_SELECTION_EMPTY) {
		cr= cairo_create(screen.selectionRenderData);
		gerbv_render_cairo_set_scale_and_translation(cr, &screenRenderInfo);
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.85);
		/* for now, assume everything in the selection buffer is from one image */
		gerbv_image_t *matchImage;
		int j;
		if (screen.selectionInfo.selectedNodeArray->len > 0) {
			gerbv_selection_item_t sItem = g_array_index (screen.selectionInfo.selectedNodeArray,
					gerbv_selection_item_t, 0);
			matchImage = (gerbv_image_t *) sItem.image;	
			dprintf("    .... calling render_image_to_cairo_target on selection layer...\n");
			for(j = mainProject->last_loaded; j >= 0; j--) {
				if ((mainProject->file[j]) && (mainProject->file[j]->image == matchImage)) {
					draw_image_to_cairo_target (cr, mainProject->file[j]->image,
						1.0/MAX(screenRenderInfo.scaleFactorX,
						screenRenderInfo.scaleFactorY),
						DRAW_SELECTIONS, &screen.selectionInfo, &screenRenderInfo,
						TRUE, mainProject->file[j]->transform, TRUE);
				}
			}
		}
		cairo_destroy (cr);
	}
}

/* ------------------------------------------------------ */
void render_refresh_rendered_image_on_screen (void) {
	GdkCursor *cursor;
	    int i;
	
	g_assert(screenRenderInfo.renderType >= GERBV_RENDER_TYPE_CAIRO_NORMAL);

	dprintf("----> Entering redraw_pixmap...\n");
	cursor = gdk_cursor_new(GDK_WATCH);
	gdk_window_set_cursor(gtk_widget_get_window(screen.drawing_area), cursor);
	gdk_cursor_destroy(cursor);

	/* 
	 * This now allows drawing several layers on top of each other.
	 * Higher layer numbers have higher priority in the Z-order.
	 */
	for(i = mainProject->last_loaded; i >= 0; i--) {
		if (mainProject->file[i]) {
		    cairo_t *cr;
		    if (mainProject->file[i]->privateRenderData) 
			cairo_surface_destroy ((cairo_surface_t *) mainProject->file[i]->privateRenderData);
		    mainProject->file[i]->privateRenderData = 
			(gpointer) cairo_surface_create_similar ((cairo_surface_t *)screen.windowSurface,
			CAIRO_CONTENT_COLOR_ALPHA, screenRenderInfo.displayWidth,
			screenRenderInfo.displayHeight);
		    cr= cairo_create(mainProject->file[i]->privateRenderData );
		    gerbv_render_layer_to_cairo_target (cr, mainProject->file[i], &screenRenderInfo);
		    dprintf("    .... calling render_image_to_cairo_target on layer %d...\n", i);			
		    cairo_destroy (cr);
		}	
	}
	/* render the selection layer */
	render_selection_layer();
	    
	render_recreate_composite_surface ();

	/* remove watch cursor and switch back to normal cursor */
	callbacks_switch_to_correct_cursor ();
	callbacks_force_expose_event_for_screen();
}

/* ------------------------------------------------------ */
void
render_clear_selection_buffer (void){
	if (screen.selectionInfo.type == GERBV_SELECTION_EMPTY)
		return;

	g_array_remove_range (screen.selectionInfo.selectedNodeArray, 0,
		screen.selectionInfo.selectedNodeArray->len);
	screen.selectionInfo.type = GERBV_SELECTION_EMPTY;
	callbacks_update_selected_object_message (FALSE);
}

void
render_remove_selected_objects_belonging_to_layer (gint index) {
	int i;
	
	for (i=screen.selectionInfo.selectedNodeArray->len-1; i>=0; i--) {
		gerbv_selection_item_t sItem = g_array_index (screen.selectionInfo.selectedNodeArray,
				gerbv_selection_item_t, i);

		gerbv_image_t *matchImage = (gerbv_image_t *) sItem.image;	
		if (mainProject->file[index]->image == matchImage) {
			g_array_remove_index (screen.selectionInfo.selectedNodeArray, index);
		}
	}
	callbacks_update_selected_object_message (FALSE);
}

/* ------------------------------------------------------ */
gint
render_create_cairo_buffer_surface () {
	if (screen.bufferSurface) {
		cairo_surface_destroy (screen.bufferSurface);
		screen.bufferSurface = NULL;
	}
	if (!screen.windowSurface)
		return 0;

	screen.bufferSurface= cairo_surface_create_similar ((cairo_surface_t *)screen.windowSurface,
	                                    CAIRO_CONTENT_COLOR, screenRenderInfo.displayWidth,
	                                    screenRenderInfo.displayHeight);
	return 1;
}

/* ------------------------------------------------------ */
void
render_find_selected_objects_and_refresh_display (gint activeFileIndex, gboolean eraseOldSelection){
	/* clear the old selection array if desired */
	if ((eraseOldSelection)&&(screen.selectionInfo.selectedNodeArray->len))
		g_array_remove_range (screen.selectionInfo.selectedNodeArray, 0,
			screen.selectionInfo.selectedNodeArray->len);

	/* make sure we have a bufferSurface...if we start up in FAST mode, we may not
	   have one yet, but we need it for selections */
	if (!render_create_cairo_buffer_surface())
		return;

	/* call draw_image... passing the FILL_SELECTION mode to just search for
	   nets which match the selection, and fill the selection buffer with them */
	cairo_t *cr= cairo_create(screen.bufferSurface);	
	gerbv_render_cairo_set_scale_and_translation(cr,&screenRenderInfo);
	draw_image_to_cairo_target (cr, mainProject->file[activeFileIndex]->image,
		1.0/MAX(screenRenderInfo.scaleFactorX, screenRenderInfo.scaleFactorY),
		FIND_SELECTIONS, &screen.selectionInfo, &screenRenderInfo, TRUE,
		mainProject->file[activeFileIndex]->transform, TRUE);
	cairo_destroy (cr);
	/* if the selection array is empty, switch the "mode" to empty to make it
	   easier to check if it is holding anything */
	if (!screen.selectionInfo.selectedNodeArray->len)
		screen.selectionInfo.type = GERBV_SELECTION_EMPTY;
	/* re-render the selection buffer layer */
	render_selection_layer();
	render_recreate_composite_surface ();
	callbacks_force_expose_event_for_screen();
}

/* ------------------------------------------------------ */
void
render_fill_selection_buffer_from_mouse_click (gint mouseX, gint mouseY, gint activeFileIndex,
		gboolean eraseOldSelection) {
	screen.selectionInfo.lowerLeftX = mouseX;
	screen.selectionInfo.lowerLeftY = mouseY;
	/* no need to populate the upperright coordinates for a point_click */
	screen.selectionInfo.type = GERBV_SELECTION_POINT_CLICK;
	render_find_selected_objects_and_refresh_display (activeFileIndex, eraseOldSelection);
}

/* ------------------------------------------------------ */
void
render_fill_selection_buffer_from_mouse_drag (gint corner1X, gint corner1Y,
	gint corner2X, gint corner2Y, gint activeFileIndex, gboolean eraseOldSelection) {
	/* figure out the lower left corner of the box */
	screen.selectionInfo.lowerLeftX = MIN(corner1X, corner2X);
	screen.selectionInfo.lowerLeftY = MIN(corner1Y, corner2Y);
	/* figure out the upper right corner of the box */
	screen.selectionInfo.upperRightX = MAX(corner1X, corner2X);
	screen.selectionInfo.upperRightY = MAX(corner1Y, corner2Y);
	
	screen.selectionInfo.type = GERBV_SELECTION_DRAG_BOX;
	render_find_selected_objects_and_refresh_display (activeFileIndex, eraseOldSelection);
}

/* ------------------------------------------------------ */
void render_recreate_composite_surface () {
	gint i;
	
	if (!render_create_cairo_buffer_surface())
		return;

	cairo_t *cr= cairo_create(screen.bufferSurface);
	/* fill the background with the appropriate color */
	cairo_set_source_rgba (cr, mainProject->background.red,
		mainProject->background.green,
		mainProject->background.blue,
		mainProject->background.alpha);
	cairo_paint (cr);
	
	for(i = mainProject->last_loaded; i >= 0; i--) {
		if (mainProject->file[i] && mainProject->file[i]->isVisible) {
			cairo_set_source_surface (cr, (cairo_surface_t *) mainProject->file[i]->privateRenderData,
			                              0, 0);
			/* ignore alpha if we are in high-speed render mode */
			if (mainProject->file[i]->color.alpha < 1.0) {
				cairo_paint_with_alpha(cr, mainProject->file[i]->color.alpha);
			}
			else {
				cairo_paint (cr);
			}
		}
	}
	/* render the selection layer at the end */
	if (screen.selectionInfo.type != GERBV_SELECTION_EMPTY) {
		cairo_set_source_surface (cr, (cairo_surface_t *) screen.selectionRenderData,
			                              0, 0);
		cairo_paint_with_alpha (cr,1.0);
	}
	cairo_destroy (cr);
}

/* ------------------------------------------------------ */
void render_project_to_cairo_target (cairo_t *cr) {
	/* fill the background with the appropriate color */
	cairo_set_source_rgba (cr, mainProject->background.red,
		mainProject->background.green,
		mainProject->background.blue,
		mainProject->background.alpha);
	cairo_paint (cr);

	cairo_set_source_surface (cr, (cairo_surface_t *) screen.bufferSurface, 0 , 0);
                                                   
	cairo_paint (cr);
}

void
render_free_screen_resources (void) {
	if (screen.selectionRenderData) 
		cairo_surface_destroy ((cairo_surface_t *)
			screen.selectionRenderData);
	if (screen.bufferSurface)
		cairo_surface_destroy ((cairo_surface_t *)
			screen.bufferSurface);
	if (screen.windowSurface)
		cairo_surface_destroy ((cairo_surface_t *)
			screen.windowSurface);
	if (screen.pixmap) 
		gdk_pixmap_unref(screen.pixmap);
}


/* ------------------------------------------------------------------ */
/*! This fills out the project's Gerber statistics table.
 *  It is called from within callbacks.c when the user
 *  asks for a Gerber report.  */
gerbv_stats_t *
generate_gerber_analysis(void)
{
	int i;
	gerbv_stats_t *stats;
	gerbv_stats_t *instats;

	/* Create new stats structure to hold report for whole project 
	* (i.e. all layers together) */
	stats = gerbv_stats_new();

	/* Loop through open layers and compile statistics by accumulating reports from each layer */
	for (i = 0; i <= mainProject->last_loaded; i++) {
		if (mainProject->file[i] && mainProject->file[i]->isVisible &&
				(mainProject->file[i]->image->layertype == GERBV_LAYERTYPE_RS274X) ) {
			instats = mainProject->file[i]->image->gerbv_stats;
			gerbv_stats_add_layer(stats, instats, i+1);
		}
	}
	return stats;
}


/* ------------------------------------------------------------------ */
/*! This fills out the project's Drill statistics table.
 *  It is called from within callbacks.c when the user
 *  asks for a Drill report.  */
gerbv_drill_stats_t *
generate_drill_analysis(void)
{
	int i;
	gerbv_drill_stats_t *stats;
	gerbv_drill_stats_t *instats;

	stats = gerbv_drill_stats_new();

	/* Loop through open layers and compile statistics by accumulating reports from each layer */
	for(i = mainProject->last_loaded; i >= 0; i--) {
		if (mainProject->file[i] && 
				mainProject->file[i]->isVisible &&
				(mainProject->file[i]->image->layertype == GERBV_LAYERTYPE_DRILL) ) {
			instats = mainProject->file[i]->image->drill_stats;
			/* add this batch of stats.  Send the layer 
			* index for error reporting */
			gerbv_drill_stats_add_layer(stats, instats, i+1);
		}
	}
	return stats;
}

