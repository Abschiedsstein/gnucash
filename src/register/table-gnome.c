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
 * table-gnome.c
 *
 * FUNCTION:
 * Implements the infrastructure for the displayed table.
 * This is the Gnome implementation.
 *
 * HISTORY:
 * Copyright (c) 1998 Linas Vepstas
 * Copyright (c) 1998 Rob Browning <rlb@cs.utexas.edu>
 * Copyright (c) 1999 Heath Martin <martinh@pegasus.cc.ucf.edu>
 * Copyright (c) 2000 Heath Martin <martinh@pegasus.cc.ucf.edu>
 * Copyright (c) 2000 Dave Peticolas <dave@krondo.com>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gnome.h>
#include <guile/gh.h>

#include "cellblock.h"
#include "global-options.h"
#include "table-allgui.h"
#include "splitreg.h"
#include "util.h"

#include "gnucash-sheet.h"
#include "gnucash-color.h"
#include "gnucash-style.h"


static void
table_destroy_cb(Table *table)
{
        int header_widths[CELL_TYPE_COUNT];
        GnucashSheet *sheet;
        SCM alist;
        int i;

        if (table == NULL)
                return;

        if (table->ui_data == NULL)
                return;

        sheet = GNUCASH_SHEET(table->ui_data);

        for (i = 0; i < CELL_TYPE_COUNT; i++)
                header_widths[i] = -1;

        if (!GTK_OBJECT_DESTROYED(GTK_OBJECT(sheet)))
                gnucash_sheet_get_header_widths (sheet, header_widths);

        alist = SCM_EOL;
        if (gnc_lookup_boolean_option("General", "Save Window Geometry", TRUE))
                for (i = 0; i < CELL_TYPE_COUNT; i++)
                {
                        const char *name;
                        SCM assoc;

                        if (header_widths[i] <= 0)
                                continue;

                        name = xaccSplitRegisterGetCellTypeName (i);
                        assoc = gh_cons (gh_str02scm(name),
                                         gh_int2scm(header_widths[i]));

                        alist = gh_cons (assoc, alist);
                }

        if (!gh_null_p(alist))
                gnc_set_option ("__gui", "reg_column_widths", alist);

        gtk_widget_unref(GTK_WIDGET(sheet));

        table->ui_data = NULL;
}

void
gnc_table_init_gui (gncUIWidget widget, void *data)
{
        int header_widths[CELL_TYPE_COUNT];
        SplitRegister *sr;
        GnucashSheet *sheet;
        GnucashRegister *greg;
        Table *table;
        SCM alist;
        int i;

        g_return_if_fail (widget != NULL);
        g_return_if_fail (GNUCASH_IS_REGISTER (widget));
        g_return_if_fail (data != NULL);

        sr = data;

        greg = GNUCASH_REGISTER(widget);
        sheet = GNUCASH_SHEET(greg->sheet);
        sheet->split_register = sr;
        table = sheet->table;

        table->destroy = table_destroy_cb;
        table->ui_data = sheet;

        gtk_widget_ref (GTK_WIDGET(sheet));

        /* config the cell-block styles */

        gnucash_sheet_set_cursor (sheet, sr->header, GNUCASH_CURSOR_HEADER);
        gnucash_sheet_set_cursor (sheet, sr->single_cursor,
                                  GNUCASH_CURSOR_SINGLE);
        gnucash_sheet_set_cursor (sheet, sr->double_cursor,
                                  GNUCASH_CURSOR_DOUBLE);
        gnucash_sheet_set_cursor (sheet, sr->trans_cursor,
                                  GNUCASH_CURSOR_TRANS);
        gnucash_sheet_set_cursor (sheet, sr->split_cursor,
                                  GNUCASH_CURSOR_SPLIT);

        for (i = 0; i < CELL_TYPE_COUNT; i++)
                header_widths[i] = -1;

        if (gnc_lookup_boolean_option("General", "Save Window Geometry", TRUE))
                alist = gnc_lookup_option ("__gui", "reg_column_widths",
                                           SCM_EOL);
        else
                alist = SCM_EOL;

        while (gh_list_p(alist) && !gh_null_p(alist))
        {
                char *name;
                CellType ctype;
                SCM assoc;

                assoc = gh_car (alist);
                alist = gh_cdr (alist);

                name = gh_scm2newstr(gh_car (assoc), NULL);
                ctype = xaccSplitRegisterGetCellTypeFromName (name);
                if (name)
                        free(name);

                if (ctype == NO_CELL)
                        continue;

                header_widths[ctype] = gh_scm2int(gh_cdr (assoc));
        }

        gnucash_sheet_create_styles (sheet);

        gnucash_sheet_set_header_widths (sheet, header_widths);

        gnucash_sheet_compile_styles (sheet);

        gnc_table_refresh_header (table);

        gnucash_sheet_table_load (sheet);
        gnucash_sheet_cursor_set_from_table (sheet, TRUE);
        gnucash_sheet_redraw_all (sheet);
}

void        
gnc_table_refresh_gui (Table * table)
{
        GnucashSheet *sheet;

        if (!table)
                return;
        if (!table->ui_data)
                return;

        g_return_if_fail (GNUCASH_IS_SHEET (table->ui_data));

        sheet = GNUCASH_SHEET(table->ui_data);

        gnucash_sheet_styles_recompile (sheet);
        gnucash_sheet_table_load (sheet);
        gnucash_sheet_redraw_all (sheet);
}


void        
gnc_table_refresh_cursor_gui (Table * table,
                              CellBlock *curs,
                              PhysicalLocation phys_loc,
                              gboolean do_scroll)
{
        GnucashSheet *sheet;
        PhysicalCell *pcell;
        VirtualCellLocation vcell_loc;

        if (!table || !table->ui_data || !curs)
                return;

        g_return_if_fail (GNUCASH_IS_SHEET (table->ui_data));

        /* if the current cursor is undefined, there is nothing to do. */
        if (gnc_table_physical_cell_out_of_bounds (table, phys_loc))
                return;

        sheet = GNUCASH_SHEET(table->ui_data);

        /* compute the physical bounds of the current cursor */
        pcell = gnc_table_get_physical_cell (table, phys_loc);

        vcell_loc = pcell->virt_loc.vcell_loc;

        gnucash_sheet_cursor_set_from_table (sheet, do_scroll);
        gnucash_sheet_block_set_from_table (sheet, vcell_loc);
        gnucash_sheet_redraw_block (sheet, vcell_loc);
}

/* ================== end of file ======================= */


/*
  Local Variables:
  c-basic-offset: 8
  End:
*/