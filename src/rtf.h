#pragma once

#include <gtk/gtk.h>

gboolean wordpad_rtf_load(GtkTextBuffer *buffer, GFile *file, GError **error);
gboolean wordpad_rtf_save(GtkTextBuffer *buffer, GFile *file, GError **error);
gboolean wordpad_txt_load(GtkTextBuffer *buffer, GFile *file, GError **error);
gboolean wordpad_txt_save(GtkTextBuffer *buffer, GFile *file, GError **error);

/* Ensure standard formatting tags exist in the buffer's tag table */
void wordpad_rtf_ensure_tags(GtkTextBuffer *buffer);

/* Get or create a dynamic tag (e.g. "font:Arial", "size:12", "color:#rrggbb") */
GtkTextTag *wordpad_get_or_create_tag(GtkTextBuffer *buffer, const char *tag_name);
