
INCLUDES = \
	-I$(top_srcdir) \
	-DPACKAGE_LOCALE_DIR=\"$(localedir) \
	-DTHEMESDIR=\"$(datadir)/xfce4/closebutton/themes\" \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	$(PLATFORM_CPPFLAGS)

plugindir = $(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = libclosebutton.la

libclosebutton_built_sources = \
	closebutton-dialog_ui.h

libclosebutton_la_SOURCES = \
	closebutton.c \
	closebutton.h \
	closebutton-settings.c \
	closebutton-settings.h

libclosebutton_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GOBJECT_CFLAGS) \
	$(EXO_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBWNCK_CFLAGS) \
	$(PLATFORM_CFLAGS)

libclosebutton_la_LIBADD = \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(GOBJECT_LIBS) \
	$(EXO_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4PANEL_LIBS) \
	$(XFCONF_LIBS) \
	$(LIBWNCK_LIBS)

libclosebutton_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

#
# Desktop file
#
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_in_files = closebutton.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =  \
	closebutton-dialog.glade \
	$(desktop_in_files)

DISTCLEANFILES = $(desktop_DATA)

if MAINTAINER_MODE
BUILT_SOURCES = \
	$(libclosebutton_built_sources)

DISTCLEANFILES += \
	$(libclosebutton_built_sources)

closebutton-dialog_ui.h: closebutton-dialog.glade
	$(AM_V_GEN) xdt-csource --static --strip-comments --strip-content --name=closebutton_dialog_ui $< >$@
endif

