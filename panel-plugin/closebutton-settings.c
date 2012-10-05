/*
 * Copyright (C) 2012 Andreas Müller <schnitzeltony@googlemail.com>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <exo/exo.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

#include "closebutton.h"
#include "closebutton-settings.h"
#include "closebutton-dialog_ui.h"

struct gtk_theme_entry
{
  gchar* stockid;
  gchar* displayname;
};

static const struct gtk_theme_entry gtk_theme_entries[] = {
  { "window-close",         N_("Standard close icon") },
  { "process-stop",         N_("Standard cancel icon") },
  { "application-exit",     N_("Standard exit icon") },
};




/* 
 * We are not priviledged to live in xfce4-panel and therefore cannot use
 * libpanel-common. So we need an almost identical copy of 
 * panel_utils_builder_new here.
 */
static GtkBuilder *
closebutton_builder_new (XfcePanelPlugin  *panel_plugin,
                         const gchar      *buffer,
                         gsize             length,
                         GObject         **dialog_return)
{
  GError      *error = NULL;
  GtkBuilder  *builder;
  GObject     *dialog, *button;

  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (panel_plugin));

  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, buffer, length, &error))
    {
      dialog = gtk_builder_get_object (builder, "dialog");
      if (G_LIKELY (dialog != NULL))
        {
          g_object_weak_ref (G_OBJECT (dialog),
                             (GWeakNotify) g_object_unref, builder);
          xfce_panel_plugin_take_window (panel_plugin, GTK_WINDOW (dialog));

          xfce_panel_plugin_block_menu (panel_plugin);
          g_object_weak_ref (G_OBJECT (dialog),
                             (GWeakNotify) xfce_panel_plugin_unblock_menu,
                             panel_plugin);

          button = gtk_builder_get_object (builder, "close-button");
          if (G_LIKELY (button != NULL))
            g_signal_connect_swapped (G_OBJECT (button), "clicked",
                G_CALLBACK (gtk_widget_destroy), dialog);

          if (G_LIKELY (dialog_return != NULL))
            *dialog_return = dialog;

          return builder;
        }
      else
          g_set_error_literal (&error, 0, 0, "No widget with the name \"dialog\" found");
    }

  g_critical ("Faild to construct the builder for plugin %s-%d: %s.",
              xfce_panel_plugin_get_name (panel_plugin),
              xfce_panel_plugin_get_unique_id (panel_plugin),
              error->message);
  g_error_free (error);
  g_object_unref (G_OBJECT (builder));

  return NULL;
}



/*
 * The string in 'newentry' are added to 'combo' at 'position'. If 'newentry'
 * matches 'property', the entry is selected.
 */
static void
closebutton_plugin_combo_addselect (GtkComboBox *combo, gchar *newentry,
                                    gint *position, gchar *property)
{
  gchar           *compare;

  g_return_if_fail (combo != NULL);
  g_return_if_fail (newentry != NULL);
  g_return_if_fail (position != NULL);
  g_return_if_fail (property != NULL);

  gtk_combo_box_insert_text (combo, *position, newentry);
  /* stock icons */
  if (*position < G_N_ELEMENTS(gtk_theme_entries))
      compare = gtk_theme_entries[*position].stockid;
  /* themes */
  else
      compare = newentry;
  /* found? -> select */
  if (g_strcmp0 (compare, property) == 0)
      gtk_combo_box_set_active (combo, *position);
  /* prepare next */
  (*position)++;
}



/*
 * Check if a string can be found in our stock id array 'gtk_theme_entries'. If
 * 'for_display' is TRUE, the strings displayed (in combobox) are compared
 * otherwise the property stored by xfconf.
 */
gint
closebutton_plugin_find_in_stock (gchar *find, gboolean for_display)
{
  gint             stockindex;

  for (stockindex = 0; stockindex < G_N_ELEMENTS(gtk_theme_entries); 
      stockindex++)
    {
      if (g_strcmp0 (find, 
                         for_display ? 
                             gtk_theme_entries[stockindex].displayname :
                             gtk_theme_entries[stockindex].stockid) == 0)
          return stockindex;
    }
  return -1;
}



/*
 * Combobox change signal handler
 */
static void
closebutton_plugin_combo_changed(GtkComboBox *combo,
                                 XfcePanelPlugin *panel_plugin)
{
  gchar            *selection, *property;
  gint              stockindex;

  selection = gtk_combo_box_get_active_text (combo);
  if (G_LIKELY (selection != NULL))
    {
      /* found in stock table: store id */
      if ((stockindex = closebutton_plugin_find_in_stock(selection, TRUE)) >= 0)
          property = gtk_theme_entries[stockindex].stockid;
      /* themes are stored as is */
      else
          property = selection;
      g_object_set (G_OBJECT (panel_plugin),
                    PROPERTY_NAME_THEME, property,
                    NULL);
    }
}


/*
 * XfcePanelPlugin handler displaying settings dialog
 */
void
closebutton_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  GtkBuilder        *builder;
  GObject           *dialog;
  gint              stockindex, comboposition;
  GtkComboBox       *combo;
  GDir              *themedir;
  gchar             *property, *themename;

  /* setup the dialog */
  if (xfce_titled_dialog_get_type () == 0)
      return;
  builder = closebutton_builder_new (panel_plugin, closebutton_dialog_ui,
                                     closebutton_dialog_ui_length, &dialog);
  if (G_UNLIKELY (builder == NULL))
      return;
  /* get current theme */
  g_object_get (G_OBJECT (panel_plugin),
                PROPERTY_NAME_THEME, &property,
                NULL);
  /* prepare combo: fill/select/signal */
  combo = GTK_COMBO_BOX (gtk_builder_get_object (builder, "theme"));
  if (G_LIKELY (combo != NULL))
    {
      comboposition = 0;
      /* 1. stock icons */
      for (stockindex = 0; stockindex < G_N_ELEMENTS(gtk_theme_entries); 
           stockindex++)
        closebutton_plugin_combo_addselect (
            combo, gettext (gtk_theme_entries[stockindex].displayname),
            &comboposition, property);
      /* 2. my themes */
      themedir = g_dir_open (THEMESDIR, 0, NULL);              
      if (G_LIKELY (themedir != NULL))
        {
          while((themename = g_dir_read_name (themedir)) != NULL)
              closebutton_plugin_combo_addselect (combo, themename,
                                                  &comboposition, property);
          g_dir_close(themedir);
        }
      g_signal_connect (G_OBJECT (combo), "changed",
          G_CALLBACK (closebutton_plugin_combo_changed), panel_plugin);
    }
  gtk_widget_show (GTK_WIDGET (dialog));
}
