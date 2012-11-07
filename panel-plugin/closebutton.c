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
#include <libwnck/libwnck.h>
#include <xfconf/xfconf.h>

#include "closebutton.h"
#include "closebutton-settings.h"

struct _CloseButtonPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _CloseButtonPlugin
{
  XfcePanelPlugin __parent__;

  /* the screen we're showing */
  WnckScreen         *screen;

  /* panel widgets */
  GtkWidget          *button;
  GtkWidget          *icon;

  /* properties */
  gchar              *theme_or_stock_id;

  /* inernal data */
  GdkPixbuf      *pixbuf;
};

enum
{
  PROP_0,
  PROP_THEME,
};

#define DEFAULT_THEME "Default"

static void      closebutton_plugin_construct               (XfcePanelPlugin    *panel_plugin);
static void      closebutton_plugin_free_data               (XfcePanelPlugin    *panel_plugin);
static gboolean  closebutton_plugin_size_changed            (XfcePanelPlugin    *panel_plugin,
                                                             gint                size);
static void      closebutton_plugin_get_property            (GObject            *object,
                                                             guint               prop_id,
                                                             GValue             *value,
                                                             GParamSpec         *pspec);
static void      closebutton_plugin_set_property            (GObject            *object,
                                                             guint               prop_id,
                                                             const GValue       *value,
                                                             GParamSpec         *pspec);
static void      closebutton_plugin_screen_changed          (GtkWidget          *widget,
                                                             GdkScreen           *previous_screen);
static void      closebutton_plugin_active_window_changed   (WnckScreen         *screen,
                                                             WnckWindow          *previous_window,
                                                             CloseButtonPlugin   *plugin);
static void      closebutton_button_clicked                 (GtkWidget          *button,
                                                             CloseButtonPlugin   *plugin);
static void      closebutton_plugin_about                   (XfcePanelPlugin    *panel_plugin);



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (CloseButtonPlugin, closebutton_plugin)



static void
closebutton_plugin_class_init (CloseButtonPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;
  GObjectClass         *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = closebutton_plugin_get_property;
  gobject_class->set_property = closebutton_plugin_set_property;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = closebutton_plugin_construct;
  plugin_class->free_data = closebutton_plugin_free_data;
  plugin_class->size_changed = closebutton_plugin_size_changed;
  plugin_class->configure_plugin = closebutton_plugin_configure_plugin;
  plugin_class->about = closebutton_plugin_about;

  g_object_class_install_property (gobject_class,
                                   PROP_THEME,
                                   g_param_spec_string (PROP_NAME_THEME,
                                                        NULL, NULL,
                                                        DEFAULT_THEME,
                                                        EXO_PARAM_READWRITE));
}



static void
closebutton_plugin_init (CloseButtonPlugin *plugin)
{
  plugin->theme_or_stock_id = g_strdup (DEFAULT_THEME);
  plugin->pixbuf = NULL;

  plugin->button = xfce_panel_create_button ();
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->button);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->button);
  g_signal_connect (G_OBJECT (plugin->button), "clicked",
      G_CALLBACK (closebutton_button_clicked), plugin);

  plugin->icon = xfce_panel_image_new_from_source (NULL);
  gtk_container_add (GTK_CONTAINER (plugin->button), plugin->icon);
  gtk_widget_show (plugin->icon);
}



static WnckWindow*
closebutton_plugin_get_effective_window (CloseButtonPlugin *plugin)
{
  WnckWindow     *window;
  WnckWindowType  type;

  window = NULL;
  if(plugin && plugin->screen)
    {
      window = wnck_screen_get_active_window (plugin->screen);
      if (G_LIKELY (window != NULL))
        {
          /* skip 'fake' windows */
          type = wnck_window_get_window_type (window);
          if (type == WNCK_WINDOW_DESKTOP || type == WNCK_WINDOW_DOCK)
              window = NULL;
        }
    }
  return window;
}



static WnckWindow*
closebutton_plugin_set_icon (CloseButtonPlugin *plugin, gboolean force_reload)
{
  WnckWindow        *window;
  GtkIconTheme      *icon_theme;
  gchar             *filename;
  gint               size, stockindex;
  XfcePanelPlugin   *panel_plugin = XFCE_PANEL_PLUGIN (plugin);
  XfcePanelImage    *icon = XFCE_PANEL_IMAGE (plugin->icon);

  g_return_if_fail (XFCE_IS_PANEL_IMAGE (icon));
  g_return_if_fail (XFCE_IS_CLOSEBUTTON_PLUGIN (plugin));

  window = closebutton_plugin_get_effective_window (plugin);
  if (window != NULL)
    {
      if (force_reload || plugin->pixbuf == NULL)
        {
          if (plugin->pixbuf)
            {
              g_object_unref(G_OBJECT(plugin->pixbuf));
              plugin->pixbuf = NULL;
            }
          stockindex = closebutton_plugin_find_in_stock (
                           plugin->theme_or_stock_id, FALSE);
          size = xfce_panel_plugin_get_size(XFCE_PANEL_PLUGIN (plugin));
          if (stockindex >= 0)
              plugin->pixbuf = xfce_panel_pixbuf_from_source (plugin->theme_or_stock_id, NULL, size);
          else
            {
              /* set icon from theme directory */
              filename = g_strdup_printf("%s" G_DIR_SEPARATOR_S
                                         "%s" G_DIR_SEPARATOR_S
                                         "closebutton.png",
                                         THEMESDIR,
                                         plugin->theme_or_stock_id);
              plugin->pixbuf = gdk_pixbuf_new_from_file_at_scale(filename,
                                                                 size,
                                                                 size,
                                                                 TRUE,
                                                                 NULL);
              g_free(filename);
            }
        }
      xfce_panel_image_set_from_pixbuf (icon, plugin->pixbuf);
      if (G_UNLIKELY(plugin->pixbuf == NULL))
          plugin_warn (plugin, "No pixbuf loaded for %s!", plugin->theme_or_stock_id);
    }
  else
      /* no icon is shown right now */
      xfce_panel_image_set_from_source (icon, NULL);

  return window;
}



static void
closebutton_plugin_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  CloseButtonPlugin *plugin = XFCE_CLOSEBUTTON_PLUGIN (object);

  switch (prop_id)
    {
    case PROP_THEME:
      g_value_set_string (value, exo_str_is_empty (plugin->theme_or_stock_id) ?
          DEFAULT_THEME : plugin->theme_or_stock_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
closebutton_plugin_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  CloseButtonPlugin *plugin = XFCE_CLOSEBUTTON_PLUGIN (object);
  XfcePanelPlugin   *panel_plugin = XFCE_PANEL_PLUGIN (object);
  gchar*             str_old;
  gboolean           bool_old, needs_redraw, icon_changed;

  g_return_if_fail (XFCE_IS_CLOSEBUTTON_PLUGIN (plugin));
  needs_redraw = FALSE;
  icon_changed = FALSE;
  switch (prop_id)
    {
    case PROP_THEME:
      str_old = plugin->theme_or_stock_id;
      plugin->theme_or_stock_id = g_value_dup_string (value);
      needs_redraw = icon_changed = 
          g_strcmp0 (str_old, plugin->theme_or_stock_id) != 0;
      g_free (str_old);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  if(needs_redraw)
      closebutton_plugin_set_icon (plugin, icon_changed);
}


static void
closebutton_plugin_screen_changed (GtkWidget *widget,
                                   GdkScreen *previous_screen)
{
  CloseButtonPlugin *plugin = XFCE_CLOSEBUTTON_PLUGIN (widget);
  GdkScreen         *screen;
  WnckScreen        *wnck_screen;

  /* get the wnck screen */
  screen = gtk_widget_get_screen (widget);
  g_return_if_fail (GDK_IS_SCREEN (screen));
  wnck_screen = wnck_screen_get (gdk_screen_get_number (screen));
  g_return_if_fail (WNCK_IS_SCREEN (wnck_screen));

  /* leave when we same wnck screen was picked */
  if (plugin->screen == wnck_screen)
    return;

  if (G_UNLIKELY (plugin->screen != NULL))
    {
      /* disconnect from the previous screen */
      g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->screen),
          closebutton_plugin_active_window_changed, plugin);
    }

  /* set the new screen */
  plugin->screen = wnck_screen;

  /* connect signal to monitor this screen */
  g_signal_connect (G_OBJECT (plugin->screen), "active-window-changed",
      G_CALLBACK (closebutton_plugin_active_window_changed), plugin);

}


static void
closebutton_plugin_active_window_changed (WnckScreen        *screen,
                                          WnckWindow        *previous_window,
                                          CloseButtonPlugin *plugin)
{
  gchar          *tooltip;
  WnckWindow     *window;
  XfcePanelImage *icon = XFCE_PANEL_IMAGE (plugin->icon);

  g_return_if_fail (XFCE_IS_CLOSEBUTTON_PLUGIN (plugin));
  g_return_if_fail (WNCK_IS_SCREEN (screen));
  g_return_if_fail (plugin->screen == screen);
  g_return_if_fail (XFCE_IS_PANEL_IMAGE (icon));

  window = closebutton_plugin_set_icon (plugin, FALSE);
  if (window != NULL)
    {
      tooltip = g_strdup_printf(_("Close \"%s\""),
                                wnck_window_get_name (window));
      gtk_widget_set_tooltip_text (GTK_WIDGET (icon),
                                   tooltip);
      g_free(tooltip);
    }
  else
      gtk_widget_set_tooltip_text (GTK_WIDGET (icon), NULL);
}



static void
closebutton_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  XfconfChannel        *channel;
  gchar                *property;
  GObject              *object = G_OBJECT(panel_plugin);
  GError               *error = NULL;

  if (G_UNLIKELY (!xfconf_init (&error)))
    {
      g_critical ("Failed to initialize Xfconf: %s", error->message);
      g_error_free (error);
      return;
    }
  /* bind properties to xfconf */
  channel = xfconf_channel_get (XFCE_PANEL_CHANNEL_NAME);
  g_return_if_fail (XFCONF_IS_CHANNEL (channel));
  property = g_strconcat (xfce_panel_plugin_get_property_base (panel_plugin), "/" PROP_NAME_THEME, NULL);
  xfconf_g_property_bind (channel, property, G_TYPE_STRING, object, PROP_NAME_THEME);
  g_free (property);

  /* show configure/about */
  xfce_panel_plugin_menu_show_configure (panel_plugin);
  xfce_panel_plugin_menu_show_about (panel_plugin);

  xfce_panel_plugin_set_small (panel_plugin, TRUE);

  /* monitor screen changes */
  g_signal_connect (object, "screen-changed",
      G_CALLBACK (closebutton_plugin_screen_changed), NULL);

  /* initialize the screen */
  closebutton_plugin_screen_changed (GTK_WIDGET (panel_plugin), NULL);

  gtk_widget_show (XFCE_CLOSEBUTTON_PLUGIN (panel_plugin)->button);
}



static void
closebutton_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  CloseButtonPlugin *plugin = XFCE_CLOSEBUTTON_PLUGIN (panel_plugin);

  /* disconnect screen changed signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (plugin),
          closebutton_plugin_screen_changed, NULL);

  /* disconnect from the screen */
  if (G_LIKELY (plugin->screen != NULL))
    {
      /* disconnect from the screen */
      g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->screen),
          closebutton_plugin_active_window_changed, plugin);

      plugin->screen = NULL;
    }
  if (G_LIKELY (plugin->pixbuf != NULL))
    {
      g_object_unref(G_OBJECT(plugin->pixbuf));
      plugin->pixbuf = NULL;
    }
  xfconf_shutdown();
}



static gboolean
closebutton_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                                 gint             size)
{
  CloseButtonPlugin *plugin = XFCE_CLOSEBUTTON_PLUGIN (panel_plugin);
  g_return_if_fail (XFCE_IS_CLOSEBUTTON_PLUGIN (plugin));

#if LIBXFCE4PANEL_CHECK_VERSION(4,9,0)
  size /= xfce_panel_plugin_get_nrows (panel_plugin);
#endif

  gtk_widget_set_size_request (GTK_WIDGET (plugin), size, size);
  return TRUE;
}



static void
closebutton_button_clicked (GtkWidget        *button,
                            CloseButtonPlugin *plugin)
{
  WnckWindow *window;
  guint32 timestamp;

  GdkEvent       *event;
  guint32         event_time;

  g_return_if_fail (XFCE_IS_CLOSEBUTTON_PLUGIN (plugin));
  g_return_if_fail (GTK_IS_BUTTON (button));
  g_return_if_fail (plugin->screen != NULL);

  /* get a copy of the event causing the menu item to activate */
  event = gtk_get_current_event ();
  event_time = gdk_event_get_time (event);

  /* close the window */
  window = closebutton_plugin_get_effective_window (plugin);
  if (G_LIKELY (window != NULL))
    wnck_window_close (window,
                       event_time);

  if (G_LIKELY (event != NULL))
    gdk_event_free (event);
}



static void
closebutton_plugin_about (XfcePanelPlugin    *panel_plugin)
{
   GdkPixbuf *icon;
   const gchar *auth[] = {
      "Andreas Müller <schnitzeltony@googlemail.com>", NULL };
   icon = xfce_panel_pixbuf_from_source("xfce4-closebutton-plugin", NULL, 32);
   gtk_show_about_dialog(NULL,
      "logo", icon,
      "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
      "version", PACKAGE_VERSION,
      "program-name", PACKAGE_NAME,
      "comments", _("A plugin implementing closebutton for application with focus"),
      "website", "http://goodies.xfce.org/projects/panel-plugins/xfce4-closebutton-plugin",
      "copyright", _("Copyright (c) 2012\n"),
      "authors", auth, NULL);

   if(icon)
      g_object_unref(G_OBJECT(icon));
}
