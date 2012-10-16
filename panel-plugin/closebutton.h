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

#ifndef __XFCE_CLOSEBUTTON_PLUGIN_H__
#define __XFCE_CLOSEBUTTON_PLUGIN_H__

#include <gtk/gtk.h>

#define PROP_NAME_THEME                 "theme"
#define PROP_NAME_COLLAPSE_NO_WINDOW    "collapse-no-window"
#define PROP_NAME_BLOCK_AUTOHIDE        "block-autohide"

#define plugin_warn(plugin, text, ...) \
    g_warning ( "Panel plugin \"%s\": " text, xfce_panel_plugin_get_display_name (XFCE_PANEL_PLUGIN (plugin)), ##__VA_ARGS__)

G_BEGIN_DECLS

typedef struct _CloseButtonPluginClass CloseButtonPluginClass;
typedef struct _CloseButtonPlugin      CloseButtonPlugin;

#define XFCE_TYPE_CLOSEBUTTON_PLUGIN            (closebutton_plugin_get_type ())
#define XFCE_CLOSEBUTTON_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_CLOSEBUTTON_PLUGIN, CloseButtonPlugin))
#define XFCE_CLOSEBUTTON_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_CLOSEBUTTON_PLUGIN, CloseButtonPluginClass))
#define XFCE_IS_CLOSEBUTTON_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_CLOSEBUTTON_PLUGIN))
#define XFCE_IS_CLOSEBUTTON_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_CLOSEBUTTON_PLUGIN))
#define XFCE_CLOSEBUTTON_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_CLOSEBUTTON_PLUGIN, CloseButtonPluginClass))

GType closebutton_plugin_get_type      (void) G_GNUC_CONST;

void  closebutton_plugin_register_type (XfcePanelTypeModule *type_module);

G_END_DECLS

#endif /* !__XFCE_CLOSEBUTTON_PLUGIN_H__ */
