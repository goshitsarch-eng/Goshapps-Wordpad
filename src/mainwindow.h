#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define WORDPAD_TYPE_WINDOW (wordpad_window_get_type())
G_DECLARE_FINAL_TYPE(WordpadWindow, wordpad_window, WORDPAD, WINDOW, AdwApplicationWindow)

WordpadWindow *wordpad_window_new(AdwApplication *app);

/* Accessors used by toolbar / dialogs */
GtkTextView   *wordpad_window_get_text_view(WordpadWindow *self);
GtkTextBuffer *wordpad_window_get_buffer(WordpadWindow *self);
GFile         *wordpad_window_get_file(WordpadWindow *self);
const char    *wordpad_window_get_filename(WordpadWindow *self);
gboolean       wordpad_window_get_modified(WordpadWindow *self);

/* Format bar widgets (for state sync) */
GtkWidget *wordpad_window_get_font_dropdown(WordpadWindow *self);
GtkWidget *wordpad_window_get_size_spin(WordpadWindow *self);
GtkWidget *wordpad_window_get_bold_btn(WordpadWindow *self);
GtkWidget *wordpad_window_get_italic_btn(WordpadWindow *self);
GtkWidget *wordpad_window_get_underline_btn(WordpadWindow *self);
GtkWidget *wordpad_window_get_align_left_btn(WordpadWindow *self);
GtkWidget *wordpad_window_get_align_center_btn(WordpadWindow *self);
GtkWidget *wordpad_window_get_align_right_btn(WordpadWindow *self);
GtkWidget *wordpad_window_get_bullet_btn(WordpadWindow *self);
GtkWidget *wordpad_window_get_color_btn(WordpadWindow *self);

/* Layout containers (for view toggle) */
GtkWidget *wordpad_window_get_toolbar_box(WordpadWindow *self);
GtkWidget *wordpad_window_get_format_bar_box(WordpadWindow *self);
GtkWidget *wordpad_window_get_status_bar(WordpadWindow *self);

/* Store format bar widgets (called from toolbar.c) */
void wordpad_window_set_format_widgets(WordpadWindow *self,
                                       GtkWidget *font_dropdown,
                                       GtkWidget *size_spin,
                                       GtkWidget *bold_btn,
                                       GtkWidget *italic_btn,
                                       GtkWidget *underline_btn,
                                       GtkWidget *align_left,
                                       GtkWidget *align_center,
                                       GtkWidget *align_right,
                                       GtkWidget *bullet_btn,
                                       GtkWidget *color_btn);

/* Update format bar state from cursor position */
void wordpad_window_update_format_state(WordpadWindow *self);

/* Flag to suppress format bar signal feedback loops */
gboolean wordpad_window_get_updating_format(WordpadWindow *self);

/* Find state */
void wordpad_window_set_find_dialog(WordpadWindow *self, GtkWindow *dialog);
GtkWindow *wordpad_window_get_find_dialog(WordpadWindow *self);
void wordpad_window_set_replace_dialog(WordpadWindow *self, GtkWindow *dialog);
GtkWindow *wordpad_window_get_replace_dialog(WordpadWindow *self);

/* Store last search parameters */
void wordpad_window_set_search_text(WordpadWindow *self, const char *text);
const char *wordpad_window_get_search_text(WordpadWindow *self);
void wordpad_window_set_search_case(WordpadWindow *self, gboolean match_case);
gboolean wordpad_window_get_search_case(WordpadWindow *self);
void wordpad_window_set_search_direction(WordpadWindow *self, gboolean forward);
gboolean wordpad_window_get_search_direction(WordpadWindow *self);

G_END_DECLS
