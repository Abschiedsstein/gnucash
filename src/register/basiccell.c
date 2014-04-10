/********************************************************************\
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652       *
 * Boston, MA  02111-1307,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/

/*
 * FILE:
 * basiccell.c
 *
 * FUNCTION: 
 * Implements the base class for the cell handler object.
 * See the header file for additional documentation.
 *
 * HISTORY:
 * Copyright (c) 1998 Linas Vepstas
 * Copyright (c) 2000 Dave Peticolas <dave@krondo.com>
 */

#include <stdlib.h>
#include <string.h>

#include "basiccell.h"


/* ===================================================== */

BasicCell * xaccMallocBasicCell (void)
{
   BasicCell * cell;

   cell = g_new(BasicCell, 1);

   xaccInitBasicCell (cell);

   return cell;
}

/* ===================================================== */

static char *
BasicCellHelpValue(BasicCell *cell)
{
  if ((cell->value != NULL) && (cell->value[0] != 0))
    return g_strdup(cell->value);

  if (cell->blank_help != NULL)
    return g_strdup(cell->blank_help);

  return NULL;
}

/* ===================================================== */

void xaccInitBasicCell (BasicCell *cell)
{
   cell->input_output = XACC_CELL_ALLOW_ALL;
   cell->changed = 0;

   cell->bg_color = 0xffffff;  /* white */
   cell->fg_color = 0x0;       /* black */
   cell->use_bg_color = 0;     /* ignore the color */
   cell->use_fg_color = 0;     /* ignore the color */

   cell->value = NULL;
   cell->blank_help = NULL;
   cell->set_value = NULL;
   cell->enter_cell = NULL;
   cell->modify_verify = NULL;
   cell->direct_update = NULL;
   cell->leave_cell = NULL;
   cell->realize = NULL;
   cell->move = NULL;
   cell->destroy = NULL;
   cell->get_help_value = BasicCellHelpValue;

   cell->gui_private = NULL;
}

/* ===================================================== */

void xaccDestroyBasicCell (BasicCell *cell)
{
   /* give any gui elements a chance to clean up */
   if (cell->destroy)
     (*(cell->destroy)) (cell);

   /* free up data strings */
   g_free (cell->value);
   cell->value = NULL;

   g_free (cell->blank_help);
   cell->blank_help = NULL;

   /* help prevent access to freed memory */
   xaccInitBasicCell (cell);

   /* free the object itself */
   g_free (cell);
}

/* ===================================================== */

void xaccSetBasicCellValue (BasicCell *cell, const char *val)
{
   CellSetValueFunc cb;

   cb = cell->set_value;
   if (cb) {
      /* avoid recursion by disabling the  
       * callback while it's being called. */
      cell->set_value = NULL;
      cb (cell, val);
      cell->set_value = cb;
   }
   else {
     g_free (cell->value);
      if (val)
        cell->value = g_strdup (val);
      else
        cell->value = g_strdup ("");
   }
}

/* ===================================================== */

void
xaccSetBasicCellBlankHelp (BasicCell *cell, const char *blank_help)
{
  if (cell == NULL)
    return;

  if (cell->blank_help != NULL)
    g_free(cell->blank_help);

  if (blank_help == NULL)
    cell->blank_help = NULL;
  else
    cell->blank_help = g_strdup(blank_help);
}

/* ===================================================== */

char *
xaccBasicCellGetHelp (BasicCell *cell)
{
  if (cell == NULL)
    return NULL;

  if (cell->get_help_value == NULL)
    return NULL;

  return cell->get_help_value(cell);
}

/* ===================================================== */

void
xaccBasicCellSetChanged (BasicCell *cell, gboolean changed)
{
  if (cell == NULL)
    return;

  cell->changed = changed ? GNC_CELL_CHANGED : 0;
}

/* ================== end of file ====================== */