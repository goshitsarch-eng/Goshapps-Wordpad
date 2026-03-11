#include "mainwindow.h"
#include "toolbar.h"
#include "dialogs.h"
#include "rtf.h"
#include <string.h>

struct _WordpadWindow {
    AdwApplicationWindow parent_instance;

    /* Layout */
    GtkWidget *toolbar_box;
    GtkWidget *format_bar_box;
    GtkWidget *text_view;
    GtkWidget *status_bar;
    GtkWidget *scrolled_window;

    /* Format bar widgets */
    GtkWidget *font_dropdown;
    GtkWidget *size_spin;
    GtkWidget *bold_btn;
    GtkWidget *italic_btn;
    GtkWidget *underline_btn;
    GtkWidget *align_left_btn;
    GtkWidget *align_center_btn;
    GtkWidget *align_right_btn;
    GtkWidget *bullet_btn;
    GtkWidget *color_btn;

    /* State */
    GFile     *current_file;
    char      *filename;
    gboolean   updating_format;

    /* Find/Replace */
    GtkWindow *find_dialog;
    GtkWindow *replace_dialog;
    char      *search_text;
    gboolean   search_case;
    gboolean   search_forward;
};

G_DEFINE_FINAL_TYPE(WordpadWindow, wordpad_window, ADW_TYPE_APPLICATION_WINDOW)

/* ── Accessors ── */

GtkTextView *wordpad_window_get_text_view(WordpadWindow *self)
{ return GTK_TEXT_VIEW(self->text_view); }

GtkTextBuffer *wordpad_window_get_buffer(WordpadWindow *self)
{ return gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->text_view)); }

GFile *wordpad_window_get_file(WordpadWindow *self)
{ return self->current_file; }

const char *wordpad_window_get_filename(WordpadWindow *self)
{ return self->filename; }

gboolean wordpad_window_get_modified(WordpadWindow *self)
{ return gtk_text_buffer_get_modified(wordpad_window_get_buffer(self)); }

GtkWidget *wordpad_window_get_font_dropdown(WordpadWindow *self) { return self->font_dropdown; }
GtkWidget *wordpad_window_get_size_spin(WordpadWindow *self) { return self->size_spin; }
GtkWidget *wordpad_window_get_bold_btn(WordpadWindow *self) { return self->bold_btn; }
GtkWidget *wordpad_window_get_italic_btn(WordpadWindow *self) { return self->italic_btn; }
GtkWidget *wordpad_window_get_underline_btn(WordpadWindow *self) { return self->underline_btn; }
GtkWidget *wordpad_window_get_align_left_btn(WordpadWindow *self) { return self->align_left_btn; }
GtkWidget *wordpad_window_get_align_center_btn(WordpadWindow *self) { return self->align_center_btn; }
GtkWidget *wordpad_window_get_align_right_btn(WordpadWindow *self) { return self->align_right_btn; }
GtkWidget *wordpad_window_get_bullet_btn(WordpadWindow *self) { return self->bullet_btn; }
GtkWidget *wordpad_window_get_color_btn(WordpadWindow *self) { return self->color_btn; }
GtkWidget *wordpad_window_get_toolbar_box(WordpadWindow *self) { return self->toolbar_box; }
GtkWidget *wordpad_window_get_format_bar_box(WordpadWindow *self) { return self->format_bar_box; }
GtkWidget *wordpad_window_get_status_bar(WordpadWindow *self) { return self->status_bar; }
gboolean wordpad_window_get_updating_format(WordpadWindow *self) { return self->updating_format; }

void wordpad_window_set_find_dialog(WordpadWindow *self, GtkWindow *d) { self->find_dialog = d; }
GtkWindow *wordpad_window_get_find_dialog(WordpadWindow *self) { return self->find_dialog; }
void wordpad_window_set_replace_dialog(WordpadWindow *self, GtkWindow *d) { self->replace_dialog = d; }
GtkWindow *wordpad_window_get_replace_dialog(WordpadWindow *self) { return self->replace_dialog; }

void wordpad_window_set_search_text(WordpadWindow *self, const char *text)
{ g_free(self->search_text); self->search_text = g_strdup(text); }
const char *wordpad_window_get_search_text(WordpadWindow *self) { return self->search_text; }
void wordpad_window_set_search_case(WordpadWindow *self, gboolean v) { self->search_case = v; }
gboolean wordpad_window_get_search_case(WordpadWindow *self) { return self->search_case; }
void wordpad_window_set_search_direction(WordpadWindow *self, gboolean v) { self->search_forward = v; }
gboolean wordpad_window_get_search_direction(WordpadWindow *self) { return self->search_forward; }

void
wordpad_window_set_format_widgets(WordpadWindow *self,
                                   GtkWidget *font_dropdown, GtkWidget *size_spin,
                                   GtkWidget *bold_btn, GtkWidget *italic_btn,
                                   GtkWidget *underline_btn,
                                   GtkWidget *align_left, GtkWidget *align_center,
                                   GtkWidget *align_right,
                                   GtkWidget *bullet_btn, GtkWidget *color_btn)
{
    self->font_dropdown = font_dropdown;
    self->size_spin = size_spin;
    self->bold_btn = bold_btn;
    self->italic_btn = italic_btn;
    self->underline_btn = underline_btn;
    self->align_left_btn = align_left;
    self->align_center_btn = align_center;
    self->align_right_btn = align_right;
    self->bullet_btn = bullet_btn;
    self->color_btn = color_btn;
}

/* ── Title ── */

static void
update_title(WordpadWindow *self)
{
    const char *name = self->filename ? self->filename : "Document";
    gboolean modified = wordpad_window_get_modified(self);
    char *title;
    if (modified)
        title = g_strdup_printf("*%s - WordPad", name);
    else
        title = g_strdup_printf("%s - WordPad", name);
    gtk_window_set_title(GTK_WINDOW(self), title);
    g_free(title);
}

/* ── Status Bar ── */

static void
update_status_bar(WordpadWindow *self)
{
    GtkTextBuffer *buf = wordpad_window_get_buffer(self);
    GtkTextMark *insert = gtk_text_buffer_get_insert(buf);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buf, &iter, insert);

    int line = gtk_text_iter_get_line(&iter) + 1;
    int col = gtk_text_iter_get_line_offset(&iter) + 1;

    char status[64];
    snprintf(status, sizeof(status), " Ln %d, Col %d", line, col);
    gtk_label_set_text(GTK_LABEL(self->status_bar), status);
}

/* ── Format State Sync ── */

static const char * const FONT_NAMES[] = {
    "Arial", "Courier New", "Georgia", "Helvetica",
    "Liberation Mono", "Liberation Sans", "Liberation Serif",
    "Monospace", "Noto Sans", "Noto Serif",
    "Sans", "Serif", "Times New Roman", "Verdana",
    NULL
};

void
wordpad_window_update_format_state(WordpadWindow *self)
{
    if (!self->bold_btn) return; /* Format bar not yet created */

    self->updating_format = TRUE;

    GtkTextBuffer *buf = wordpad_window_get_buffer(self);
    GtkTextMark *insert = gtk_text_buffer_get_insert(buf);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buf, &iter, insert);

    /* Move back one char if not at start, to check formatting of char before cursor */
    GtkTextIter check = iter;
    if (!gtk_text_iter_is_start(&check))
        gtk_text_iter_backward_char(&check);

    GSList *tags = gtk_text_iter_get_tags(&check);

    gboolean bold = FALSE, italic = FALSE, underline = FALSE;
    int alignment = 0; /* 0=left, 1=center, 2=right */
    int size = 0;

    for (GSList *l = tags; l; l = l->next) {
        GtkTextTag *tag = l->data;
        char *name = NULL;
        g_object_get(tag, "name", &name, NULL);
        if (!name) continue;

        if (strcmp(name, "bold") == 0) bold = TRUE;
        else if (strcmp(name, "italic") == 0) italic = TRUE;
        else if (strcmp(name, "underline") == 0) underline = TRUE;
        else if (strcmp(name, "align-center") == 0) alignment = 1;
        else if (strcmp(name, "align-right") == 0) alignment = 2;
        else if (g_str_has_prefix(name, "size:")) size = atoi(name + 5);

        g_free(name);
    }
    g_slist_free(tags);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->bold_btn), bold);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->italic_btn), italic);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->underline_btn), underline);

    switch (alignment) {
    case 1: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->align_center_btn), TRUE); break;
    case 2: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->align_right_btn), TRUE); break;
    default: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->align_left_btn), TRUE); break;
    }

    if (size > 0)
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->size_spin), size);

    /* Update font dropdown - font pointer was freed above so we re-check */
    tags = gtk_text_iter_get_tags(&check);
    for (GSList *l = tags; l; l = l->next) {
        GtkTextTag *tag = l->data;
        char *name = NULL;
        g_object_get(tag, "name", &name, NULL);
        if (name && g_str_has_prefix(name, "font:")) {
            const char *fn = name + 5;
            for (int i = 0; FONT_NAMES[i]; i++) {
                if (g_ascii_strcasecmp(FONT_NAMES[i], fn) == 0) {
                    gtk_drop_down_set_selected(GTK_DROP_DOWN(self->font_dropdown), i);
                    break;
                }
            }
            g_free(name);
            break;
        }
        g_free(name);
    }
    g_slist_free(tags);

    self->updating_format = FALSE;
}

/* ── Signal Handlers ── */

static void
on_mark_set(GtkTextBuffer *buffer, GtkTextIter *location, GtkTextMark *mark,
            WordpadWindow *self)
{
    (void)buffer;
    (void)location;
    if (mark == gtk_text_buffer_get_insert(gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->text_view)))) {
        update_status_bar(self);
        wordpad_window_update_format_state(self);
    }
}

static void
on_modified_changed(GtkTextBuffer *buffer, WordpadWindow *self)
{
    (void)buffer;
    update_title(self);
}

/* ── File Operations ── */

static void
set_file(WordpadWindow *self, GFile *file)
{
    g_clear_object(&self->current_file);
    g_free(self->filename);
    self->filename = NULL;

    if (file) {
        self->current_file = g_object_ref(file);
        self->filename = g_file_get_basename(file);
    }
    update_title(self);
}

static void
do_new_document(WordpadWindow *self)
{
    GtkTextBuffer *buf = wordpad_window_get_buffer(self);
    gtk_text_buffer_set_text(buf, "", 0);
    wordpad_rtf_ensure_tags(buf);
    gtk_text_buffer_set_modified(buf, FALSE);
    set_file(self, NULL);
}

static void
new_proceed(WordpadWindow *win, gboolean proceed, gpointer user_data)
{
    (void)user_data;
    if (proceed) do_new_document(win);
}

static void
action_new(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name;
    (void)parameter;
    WordpadWindow *self = WORDPAD_WINDOW(widget);
    if (wordpad_window_get_modified(self))
        wordpad_prompt_save(self, new_proceed, NULL);
    else
        do_new_document(self);
}

/* Open file callback */
static void
on_open_finish(GObject *source, GAsyncResult *result, gpointer user_data)
{
    WordpadWindow *self = user_data;
    GtkFileDialog *dlg = GTK_FILE_DIALOG(source);

    GError *error = NULL;
    GFile *file = gtk_file_dialog_open_finish(dlg, result, &error);
    if (!file) {
        g_clear_error(&error);
        return;
    }

    GtkTextBuffer *buf = wordpad_window_get_buffer(self);
    char *name = g_file_get_basename(file);
    gboolean ok;

    if (name && g_str_has_suffix(name, ".txt"))
        ok = wordpad_txt_load(buf, file, &error);
    else
        ok = wordpad_rtf_load(buf, file, &error);

    if (ok) {
        set_file(self, file);
        wordpad_rtf_ensure_tags(buf);
    } else {
        g_clear_error(&error);
    }

    g_free(name);
    g_object_unref(file);
}

static void
do_open(WordpadWindow *self)
{
    GtkFileDialog *dlg = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dlg, "Open");

    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);

    GtkFileFilter *rtf_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(rtf_filter, "Rich Text Format (*.rtf)");
    gtk_file_filter_add_pattern(rtf_filter, "*.rtf");
    g_list_store_append(filters, rtf_filter);
    g_object_unref(rtf_filter);

    GtkFileFilter *txt_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(txt_filter, "Text Documents (*.txt)");
    gtk_file_filter_add_pattern(txt_filter, "*.txt");
    g_list_store_append(filters, txt_filter);
    g_object_unref(txt_filter);

    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All Files (*.*)");
    gtk_file_filter_add_pattern(all_filter, "*");
    g_list_store_append(filters, all_filter);
    g_object_unref(all_filter);

    gtk_file_dialog_set_filters(dlg, G_LIST_MODEL(filters));
    g_object_unref(filters);

    gtk_file_dialog_open(dlg, GTK_WINDOW(self), NULL, on_open_finish, self);
    g_object_unref(dlg);
}

static void
open_proceed(WordpadWindow *win, gboolean proceed, gpointer user_data)
{
    (void)user_data;
    if (proceed) do_open(win);
}

static void
action_open(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name;
    (void)parameter;
    WordpadWindow *self = WORDPAD_WINDOW(widget);
    if (wordpad_window_get_modified(self))
        wordpad_prompt_save(self, open_proceed, NULL);
    else
        do_open(self);
}

/* Save */
static void
do_save_to_file(WordpadWindow *self, GFile *file)
{
    GtkTextBuffer *buf = wordpad_window_get_buffer(self);
    char *name = g_file_get_basename(file);
    GError *error = NULL;
    gboolean ok;

    if (name && g_str_has_suffix(name, ".txt"))
        ok = wordpad_txt_save(buf, file, &error);
    else
        ok = wordpad_rtf_save(buf, file, &error);

    if (ok) {
        set_file(self, file);
    } else {
        g_clear_error(&error);
    }
    g_free(name);
}

static void
on_save_as_finish(GObject *source, GAsyncResult *result, gpointer user_data)
{
    WordpadWindow *self = user_data;
    GtkFileDialog *dlg = GTK_FILE_DIALOG(source);

    GError *error = NULL;
    GFile *file = gtk_file_dialog_save_finish(dlg, result, &error);
    if (!file) {
        g_clear_error(&error);
        return;
    }

    do_save_to_file(self, file);
    g_object_unref(file);
}

static void
do_save_as(WordpadWindow *self)
{
    GtkFileDialog *dlg = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dlg, "Save As");

    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);

    GtkFileFilter *rtf_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(rtf_filter, "Rich Text Format (*.rtf)");
    gtk_file_filter_add_pattern(rtf_filter, "*.rtf");
    g_list_store_append(filters, rtf_filter);
    g_object_unref(rtf_filter);

    GtkFileFilter *txt_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(txt_filter, "Text Documents (*.txt)");
    gtk_file_filter_add_pattern(txt_filter, "*.txt");
    g_list_store_append(filters, txt_filter);
    g_object_unref(txt_filter);

    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All Files (*.*)");
    gtk_file_filter_add_pattern(all_filter, "*");
    g_list_store_append(filters, all_filter);
    g_object_unref(all_filter);

    gtk_file_dialog_set_filters(dlg, G_LIST_MODEL(filters));
    g_object_unref(filters);

    if (self->current_file)
        gtk_file_dialog_set_initial_file(dlg, self->current_file);

    gtk_file_dialog_save(dlg, GTK_WINDOW(self), NULL, on_save_as_finish, self);
    g_object_unref(dlg);
}

static void
action_save(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name;
    (void)parameter;
    WordpadWindow *self = WORDPAD_WINDOW(widget);
    if (self->current_file)
        do_save_to_file(self, self->current_file);
    else
        do_save_as(self);
}

static void
action_save_as(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name;
    (void)parameter;
    do_save_as(WORDPAD_WINDOW(widget));
}

/* ── Edit Actions ── */

static void
action_undo(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    GtkTextBuffer *buf = wordpad_window_get_buffer(WORDPAD_WINDOW(widget));
    if (gtk_text_buffer_get_can_undo(buf))
        gtk_text_buffer_undo(buf);
}

static void
action_redo(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    GtkTextBuffer *buf = wordpad_window_get_buffer(WORDPAD_WINDOW(widget));
    if (gtk_text_buffer_get_can_redo(buf))
        gtk_text_buffer_redo(buf);
}

static void
action_cut(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    WordpadWindow *self = WORDPAD_WINDOW(widget);
    GdkClipboard *cb = gdk_display_get_clipboard(gtk_widget_get_display(widget));
    GtkTextBuffer *buf = wordpad_window_get_buffer(self);
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        char *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
        gdk_clipboard_set_text(cb, text);
        g_free(text);
        gtk_text_buffer_delete(buf, &start, &end);
    }
}

static void
action_copy(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    WordpadWindow *self = WORDPAD_WINDOW(widget);
    GdkClipboard *cb = gdk_display_get_clipboard(gtk_widget_get_display(widget));
    GtkTextBuffer *buf = wordpad_window_get_buffer(self);
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        char *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
        gdk_clipboard_set_text(cb, text);
        g_free(text);
    }
}

static void
on_paste_text_ready(GObject *source, GAsyncResult *result, gpointer user_data)
{
    WordpadWindow *self = user_data;
    GdkClipboard *cb = GDK_CLIPBOARD(source);
    GError *error = NULL;
    char *text = gdk_clipboard_read_text_finish(cb, result, &error);
    if (text) {
        GtkTextBuffer *buf = wordpad_window_get_buffer(self);
        gtk_text_buffer_insert_at_cursor(buf, text, -1);
        g_free(text);
    }
    g_clear_error(&error);
}

static void
action_paste(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    WordpadWindow *self = WORDPAD_WINDOW(widget);
    GdkClipboard *cb = gdk_display_get_clipboard(gtk_widget_get_display(widget));
    gdk_clipboard_read_text_async(cb, NULL, on_paste_text_ready, self);
}

static void
action_clear(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    GtkTextBuffer *buf = wordpad_window_get_buffer(WORDPAD_WINDOW(widget));
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end))
        gtk_text_buffer_delete(buf, &start, &end);
}

static void
action_select_all(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    GtkTextBuffer *buf = wordpad_window_get_buffer(WORDPAD_WINDOW(widget));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);
    gtk_text_buffer_select_range(buf, &start, &end);
}

/* ── Format Actions ── */

static void
toggle_format_tag(WordpadWindow *self, const char *tag_name)
{
    GtkTextBuffer *buf = wordpad_window_get_buffer(self);
    GtkTextIter start, end;

    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        /* Check if tag is active at start of selection */
        GtkTextTag *tag = gtk_text_tag_table_lookup(
            gtk_text_buffer_get_tag_table(buf), tag_name);
        if (tag && gtk_text_iter_has_tag(&start, tag))
            gtk_text_buffer_remove_tag_by_name(buf, tag_name, &start, &end);
        else
            gtk_text_buffer_apply_tag_by_name(buf, tag_name, &start, &end);
    }
}

static void
action_bold(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    toggle_format_tag(WORDPAD_WINDOW(widget), "bold");
    wordpad_window_update_format_state(WORDPAD_WINDOW(widget));
}

static void
action_italic(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    toggle_format_tag(WORDPAD_WINDOW(widget), "italic");
    wordpad_window_update_format_state(WORDPAD_WINDOW(widget));
}

static void
action_underline(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    toggle_format_tag(WORDPAD_WINDOW(widget), "underline");
    wordpad_window_update_format_state(WORDPAD_WINDOW(widget));
}

static void
apply_alignment(WordpadWindow *self, const char *align_tag)
{
    GtkTextBuffer *buf = wordpad_window_get_buffer(self);
    GtkTextIter start, end;

    GtkTextMark *insert = gtk_text_buffer_get_insert(buf);
    gtk_text_buffer_get_iter_at_mark(buf, &start, insert);

    if (!gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        end = start;
    }

    gtk_text_iter_set_line_offset(&start, 0);
    if (!gtk_text_iter_ends_line(&end))
        gtk_text_iter_forward_to_line_end(&end);
    gtk_text_iter_forward_char(&end);

    gtk_text_buffer_remove_tag_by_name(buf, "align-left", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buf, "align-center", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buf, "align-right", &start, &end);
    gtk_text_buffer_apply_tag_by_name(buf, align_tag, &start, &end);
    wordpad_window_update_format_state(self);
}

static void action_align_left(GtkWidget *w, const char *a, GVariant *p)
{ (void)a; (void)p; apply_alignment(WORDPAD_WINDOW(w), "align-left"); }

static void action_align_center(GtkWidget *w, const char *a, GVariant *p)
{ (void)a; (void)p; apply_alignment(WORDPAD_WINDOW(w), "align-center"); }

static void action_align_right(GtkWidget *w, const char *a, GVariant *p)
{ (void)a; (void)p; apply_alignment(WORDPAD_WINDOW(w), "align-right"); }

/* ── Dialog Actions ── */

static void action_find(GtkWidget *w, const char *a, GVariant *p)
{ (void)a; (void)p; wordpad_show_find_dialog(WORDPAD_WINDOW(w)); }

static void action_find_next(GtkWidget *w, const char *a, GVariant *p)
{ (void)a; (void)p; wordpad_find_next(WORDPAD_WINDOW(w)); }

static void action_replace(GtkWidget *w, const char *a, GVariant *p)
{ (void)a; (void)p; wordpad_show_replace_dialog(WORDPAD_WINDOW(w)); }

static void action_insert_datetime(GtkWidget *w, const char *a, GVariant *p)
{ (void)a; (void)p; wordpad_show_datetime_dialog(WORDPAD_WINDOW(w)); }

static void action_font(GtkWidget *w, const char *a, GVariant *p)
{ (void)a; (void)p; wordpad_show_font_dialog(WORDPAD_WINDOW(w)); }

static void action_paragraph(GtkWidget *w, const char *a, GVariant *p)
{ (void)a; (void)p; wordpad_show_paragraph_dialog(WORDPAD_WINDOW(w)); }

static void action_about(GtkWidget *w, const char *a, GVariant *p)
{ (void)a; (void)p; wordpad_show_about_dialog(WORDPAD_WINDOW(w)); }

/* ── View Toggle Actions ── */

static void
action_toggle_toolbar(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void)parameter;
    WordpadWindow *self = user_data;
    GVariant *state = g_action_get_state(G_ACTION(action));
    gboolean visible = !g_variant_get_boolean(state);
    g_simple_action_set_state(action, g_variant_new_boolean(visible));
    gtk_widget_set_visible(self->toolbar_box, visible);
    g_variant_unref(state);
}

static void
action_toggle_format_bar(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void)parameter;
    WordpadWindow *self = user_data;
    GVariant *state = g_action_get_state(G_ACTION(action));
    gboolean visible = !g_variant_get_boolean(state);
    g_simple_action_set_state(action, g_variant_new_boolean(visible));
    gtk_widget_set_visible(self->format_bar_box, visible);
    g_variant_unref(state);
}

static void
action_toggle_status_bar(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void)parameter;
    WordpadWindow *self = user_data;
    GVariant *state = g_action_get_state(G_ACTION(action));
    gboolean visible = !g_variant_get_boolean(state);
    g_simple_action_set_state(action, g_variant_new_boolean(visible));
    gtk_widget_set_visible(self->status_bar, visible);
    g_variant_unref(state);
}

/* ── Print ── */

static void
on_draw_page(GtkPrintOperation *op, GtkPrintContext *ctx, int page_nr, gpointer user_data)
{
    (void)op;
    WordpadWindow *self = user_data;
    cairo_t *cr = gtk_print_context_get_cairo_context(ctx);
    double width = gtk_print_context_get_width(ctx);
    double height = gtk_print_context_get_height(ctx);
    (void)height;

    GtkTextBuffer *buf = wordpad_window_get_buffer(self);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);
    char *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);

    PangoLayout *layout = gtk_print_context_create_pango_layout(ctx);
    pango_layout_set_width(layout, (int)(width * PANGO_SCALE));
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

    /* Simple: print all text on page 0 */
    if (page_nr == 0) {
        PangoFontDescription *desc = pango_font_description_from_string("Sans 10");
        pango_layout_set_font_description(layout, desc);
        pango_layout_set_text(layout, text, -1);

        cairo_move_to(cr, 0, 0);
        pango_cairo_show_layout(cr, layout);

        pango_font_description_free(desc);
    }

    g_object_unref(layout);
    g_free(text);
}

static void
action_print(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    WordpadWindow *self = WORDPAD_WINDOW(widget);
    GtkPrintOperation *op = gtk_print_operation_new();
    gtk_print_operation_set_n_pages(op, 1);
    g_signal_connect(op, "draw-page", G_CALLBACK(on_draw_page), self);
    gtk_print_operation_run(op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                            GTK_WINDOW(self), NULL);
    g_object_unref(op);
}

/* ── Quit / Close ── */

static void
quit_proceed(WordpadWindow *win, gboolean proceed, gpointer user_data)
{
    (void)user_data;
    if (proceed)
        gtk_window_destroy(GTK_WINDOW(win));
}

static void
action_quit(GtkWidget *widget, const char *action_name, GVariant *parameter)
{
    (void)action_name; (void)parameter;
    WordpadWindow *self = WORDPAD_WINDOW(widget);
    if (wordpad_window_get_modified(self))
        wordpad_prompt_save(self, quit_proceed, NULL);
    else
        gtk_window_destroy(GTK_WINDOW(self));
}

static gboolean
on_close_request(GtkWindow *window, gpointer user_data)
{
    (void)user_data;
    WordpadWindow *self = WORDPAD_WINDOW(window);
    if (wordpad_window_get_modified(self)) {
        wordpad_prompt_save(self, quit_proceed, NULL);
        return TRUE; /* prevent default close */
    }
    return FALSE; /* allow close */
}

/* ── Stub actions (unimplemented features) ── */

static void action_noop(GtkWidget *w, const char *a, GVariant *p)
{ (void)w; (void)a; (void)p; }

/* ── Class Init ── */

static void
wordpad_window_dispose(GObject *object)
{
    WordpadWindow *self = WORDPAD_WINDOW(object);
    g_clear_object(&self->current_file);
    g_free(self->filename);
    self->filename = NULL;
    g_free(self->search_text);
    self->search_text = NULL;
    G_OBJECT_CLASS(wordpad_window_parent_class)->dispose(object);
}

static void
wordpad_window_class_init(WordpadWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = wordpad_window_dispose;

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    /* Install actions */
    gtk_widget_class_install_action(widget_class, "win.new", NULL, action_new);
    gtk_widget_class_install_action(widget_class, "win.open", NULL, action_open);
    gtk_widget_class_install_action(widget_class, "win.save", NULL, action_save);
    gtk_widget_class_install_action(widget_class, "win.save-as", NULL, action_save_as);
    gtk_widget_class_install_action(widget_class, "win.print", NULL, action_print);
    gtk_widget_class_install_action(widget_class, "win.print-preview", NULL, action_noop);
    gtk_widget_class_install_action(widget_class, "win.page-setup", NULL, action_noop);
    gtk_widget_class_install_action(widget_class, "win.send", NULL, action_noop);
    gtk_widget_class_install_action(widget_class, "win.quit", NULL, action_quit);

    gtk_widget_class_install_action(widget_class, "win.undo", NULL, action_undo);
    gtk_widget_class_install_action(widget_class, "win.redo", NULL, action_redo);
    gtk_widget_class_install_action(widget_class, "win.cut", NULL, action_cut);
    gtk_widget_class_install_action(widget_class, "win.copy", NULL, action_copy);
    gtk_widget_class_install_action(widget_class, "win.paste", NULL, action_paste);
    gtk_widget_class_install_action(widget_class, "win.paste-special", NULL, action_noop);
    gtk_widget_class_install_action(widget_class, "win.clear", NULL, action_clear);
    gtk_widget_class_install_action(widget_class, "win.find", NULL, action_find);
    gtk_widget_class_install_action(widget_class, "win.find-next", NULL, action_find_next);
    gtk_widget_class_install_action(widget_class, "win.replace", NULL, action_replace);
    gtk_widget_class_install_action(widget_class, "win.select-all", NULL, action_select_all);
    gtk_widget_class_install_action(widget_class, "win.links", NULL, action_noop);
    gtk_widget_class_install_action(widget_class, "win.object-properties", NULL, action_noop);
    gtk_widget_class_install_action(widget_class, "win.object", NULL, action_noop);

    gtk_widget_class_install_action(widget_class, "win.insert-datetime", NULL, action_insert_datetime);
    gtk_widget_class_install_action(widget_class, "win.insert-object", NULL, action_noop);

    gtk_widget_class_install_action(widget_class, "win.font", NULL, action_font);
    gtk_widget_class_install_action(widget_class, "win.toggle-bullets", NULL, action_noop);
    gtk_widget_class_install_action(widget_class, "win.paragraph", NULL, action_paragraph);
    gtk_widget_class_install_action(widget_class, "win.tabs", NULL, action_noop);
    gtk_widget_class_install_action(widget_class, "win.options", NULL, action_noop);

    gtk_widget_class_install_action(widget_class, "win.toggle-bold", NULL, action_bold);
    gtk_widget_class_install_action(widget_class, "win.toggle-italic", NULL, action_italic);
    gtk_widget_class_install_action(widget_class, "win.toggle-underline", NULL, action_underline);
    gtk_widget_class_install_action(widget_class, "win.align-left", NULL, action_align_left);
    gtk_widget_class_install_action(widget_class, "win.align-center", NULL, action_align_center);
    gtk_widget_class_install_action(widget_class, "win.align-right", NULL, action_align_right);

    gtk_widget_class_install_action(widget_class, "win.about", NULL, action_about);
}

static void
wordpad_window_init(WordpadWindow *self)
{
    self->search_forward = TRUE;

    /* Main vertical layout */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(self), vbox);

    /* Menu bar — load from menus.ui */
    /* We'll set the menu model from the app in main.c instead */

    /* Toolbar */
    self->toolbar_box = wordpad_toolbar_create(self);
    gtk_box_append(GTK_BOX(vbox), self->toolbar_box);

    /* Format bar */
    self->format_bar_box = wordpad_format_bar_create(self);
    gtk_box_append(GTK_BOX(vbox), self->format_bar_box);

    /* Text area */
    self->scrolled_window = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(self->scrolled_window, TRUE);
    gtk_widget_set_hexpand(self->scrolled_window, TRUE);

    self->text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(self->text_view), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(self->text_view), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(self->text_view), 8);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(self->text_view), 4);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(self->text_view), 4);

    /* Enable undo */
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->text_view));
    gtk_text_buffer_set_enable_undo(buf, TRUE);

    /* Ensure formatting tags */
    wordpad_rtf_ensure_tags(buf);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(self->scrolled_window), self->text_view);
    gtk_box_append(GTK_BOX(vbox), self->scrolled_window);

    /* Status bar */
    self->status_bar = gtk_label_new(" Ln 1, Col 1");
    gtk_label_set_xalign(GTK_LABEL(self->status_bar), 0);
    gtk_widget_set_margin_start(self->status_bar, 8);
    gtk_widget_set_margin_end(self->status_bar, 8);
    gtk_widget_set_margin_top(self->status_bar, 2);
    gtk_widget_set_margin_bottom(self->status_bar, 2);
    gtk_box_append(GTK_BOX(vbox), self->status_bar);

    /* Connect signals */
    g_signal_connect(buf, "mark-set", G_CALLBACK(on_mark_set), self);
    g_signal_connect(buf, "modified-changed", G_CALLBACK(on_modified_changed), self);
    g_signal_connect(self, "close-request", G_CALLBACK(on_close_request), NULL);

    /* View toggle actions (stateful) */
    GSimpleAction *show_toolbar = g_simple_action_new_stateful(
        "show-toolbar", NULL, g_variant_new_boolean(TRUE));
    g_signal_connect(show_toolbar, "activate", G_CALLBACK(action_toggle_toolbar), self);
    g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(show_toolbar));

    GSimpleAction *show_format_bar = g_simple_action_new_stateful(
        "show-format-bar", NULL, g_variant_new_boolean(TRUE));
    g_signal_connect(show_format_bar, "activate", G_CALLBACK(action_toggle_format_bar), self);
    g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(show_format_bar));

    GSimpleAction *show_ruler = g_simple_action_new_stateful(
        "show-ruler", NULL, g_variant_new_boolean(FALSE));
    g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(show_ruler));

    GSimpleAction *show_status_bar = g_simple_action_new_stateful(
        "show-status-bar", NULL, g_variant_new_boolean(TRUE));
    g_signal_connect(show_status_bar, "activate", G_CALLBACK(action_toggle_status_bar), self);
    g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(show_status_bar));

    /* Window defaults */
    gtk_window_set_default_size(GTK_WINDOW(self), 800, 600);
    update_title(self);
}

WordpadWindow *
wordpad_window_new(AdwApplication *app)
{
    return g_object_new(WORDPAD_TYPE_WINDOW,
                        "application", app,
                        NULL);
}
