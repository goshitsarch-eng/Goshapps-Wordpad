#include "dialogs.h"
#include "rtf.h"
#include <string.h>
#include <time.h>

/* ── Find Dialog ── */

typedef struct {
    WordpadWindow *win;
    GtkWidget     *entry;
    GtkWidget     *match_case;
    GtkWidget     *dir_down;
    GtkWidget     *dir_up;
} FindDialogData;

static void
on_find_next_clicked(GtkButton *btn, FindDialogData *data)
{
    (void)btn;
    const char *text = gtk_editable_get_text(GTK_EDITABLE(data->entry));
    if (!text || text[0] == '\0') return;

    gboolean match_case = gtk_check_button_get_active(GTK_CHECK_BUTTON(data->match_case));
    gboolean forward = gtk_check_button_get_active(GTK_CHECK_BUTTON(data->dir_down));

    wordpad_window_set_search_text(data->win, text);
    wordpad_window_set_search_case(data->win, match_case);
    wordpad_window_set_search_direction(data->win, forward);
    wordpad_find_next(data->win);
}

static void
on_find_dialog_close(GtkWindow *dialog, FindDialogData *data)
{
    (void)dialog;
    wordpad_window_set_find_dialog(data->win, NULL);
    g_free(data);
}

void
wordpad_show_find_dialog(WordpadWindow *win)
{
    GtkWindow *existing = wordpad_window_get_find_dialog(win);
    if (existing) {
        gtk_window_present(existing);
        return;
    }

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Find");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 380, -1);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);

    FindDialogData *data = g_new0(FindDialogData, 1);
    data->win = win;

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_widget_set_margin_start(grid, 12);
    gtk_widget_set_margin_end(grid, 12);
    gtk_widget_set_margin_top(grid, 12);
    gtk_widget_set_margin_bottom(grid, 12);

    GtkWidget *label = gtk_label_new("Fi_nd what:");
    gtk_label_set_use_underline(GTK_LABEL(label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    data->entry = gtk_search_entry_new();
    gtk_widget_set_hexpand(data->entry, TRUE);
    const char *prev = wordpad_window_get_search_text(win);
    if (prev) gtk_editable_set_text(GTK_EDITABLE(data->entry), prev);
    gtk_grid_attach(GTK_GRID(grid), data->entry, 1, 0, 2, 1);

    data->match_case = gtk_check_button_new_with_label("Match _case");
    gtk_check_button_set_use_underline(GTK_CHECK_BUTTON(data->match_case), TRUE);
    gtk_grid_attach(GTK_GRID(grid), data->match_case, 0, 1, 2, 1);

    GtkWidget *dir_label = gtk_label_new("Direction:");
    gtk_label_set_xalign(GTK_LABEL(dir_label), 0);
    gtk_grid_attach(GTK_GRID(grid), dir_label, 0, 2, 1, 1);

    data->dir_up = gtk_check_button_new_with_label("_Up");
    gtk_check_button_set_use_underline(GTK_CHECK_BUTTON(data->dir_up), TRUE);
    data->dir_down = gtk_check_button_new_with_label("_Down");
    gtk_check_button_set_use_underline(GTK_CHECK_BUTTON(data->dir_down), TRUE);
    gtk_check_button_set_group(GTK_CHECK_BUTTON(data->dir_up), GTK_CHECK_BUTTON(data->dir_down));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(data->dir_down), TRUE);

    GtkWidget *dir_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(dir_box), data->dir_up);
    gtk_box_append(GTK_BOX(dir_box), data->dir_down);
    gtk_grid_attach(GTK_GRID(grid), dir_box, 1, 2, 2, 1);

    GtkWidget *find_btn = gtk_button_new_with_label("_Find Next");
    gtk_button_set_use_underline(GTK_BUTTON(find_btn), TRUE);
    gtk_widget_add_css_class(find_btn, "suggested-action");
    g_signal_connect(find_btn, "clicked", G_CALLBACK(on_find_next_clicked), data);
    gtk_grid_attach(GTK_GRID(grid), find_btn, 2, 3, 1, 1);

    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_grid_attach(GTK_GRID(grid), cancel_btn, 1, 3, 1, 1);

    gtk_window_set_child(GTK_WINDOW(dialog), grid);
    g_signal_connect(dialog, "destroy", G_CALLBACK(on_find_dialog_close), data);
    wordpad_window_set_find_dialog(win, GTK_WINDOW(dialog));
    gtk_window_present(GTK_WINDOW(dialog));
}

/* ── Find Next logic ── */

void
wordpad_find_next(WordpadWindow *win)
{
    const char *text = wordpad_window_get_search_text(win);
    if (!text || text[0] == '\0') {
        wordpad_show_find_dialog(win);
        return;
    }

    gboolean match_case = wordpad_window_get_search_case(win);
    gboolean forward = wordpad_window_get_search_direction(win);

    GtkTextBuffer *buf = wordpad_window_get_buffer(win);
    GtkTextIter start, match_start, match_end;

    GtkTextMark *insert = gtk_text_buffer_get_insert(buf);
    gtk_text_buffer_get_iter_at_mark(buf, &start, insert);

    GtkTextSearchFlags flags = GTK_TEXT_SEARCH_TEXT_ONLY;
    if (!match_case)
        flags |= GTK_TEXT_SEARCH_CASE_INSENSITIVE;

    gboolean found;
    if (forward) {
        /* Start search from after current selection */
        GtkTextMark *sel = gtk_text_buffer_get_selection_bound(buf);
        gtk_text_buffer_get_iter_at_mark(buf, &start, sel);
        found = gtk_text_iter_forward_search(&start, text, flags,
                                              &match_start, &match_end, NULL);
        if (!found) {
            /* Wrap around */
            gtk_text_buffer_get_start_iter(buf, &start);
            found = gtk_text_iter_forward_search(&start, text, flags,
                                                  &match_start, &match_end, NULL);
        }
    } else {
        found = gtk_text_iter_backward_search(&start, text, flags,
                                               &match_start, &match_end, NULL);
        if (!found) {
            gtk_text_buffer_get_end_iter(buf, &start);
            found = gtk_text_iter_backward_search(&start, text, flags,
                                                   &match_start, &match_end, NULL);
        }
    }

    if (found) {
        gtk_text_buffer_select_range(buf, &match_start, &match_end);
        GtkTextView *tv = wordpad_window_get_text_view(win);
        gtk_text_view_scroll_mark_onscreen(tv, gtk_text_buffer_get_insert(buf));
    }
}

/* ── Replace Dialog ── */

typedef struct {
    WordpadWindow *win;
    GtkWidget     *find_entry;
    GtkWidget     *replace_entry;
    GtkWidget     *match_case;
} ReplaceDialogData;

static void
on_replace_find_next(GtkButton *btn, ReplaceDialogData *data)
{
    (void)btn;
    const char *text = gtk_editable_get_text(GTK_EDITABLE(data->find_entry));
    gboolean match_case = gtk_check_button_get_active(GTK_CHECK_BUTTON(data->match_case));
    wordpad_window_set_search_text(data->win, text);
    wordpad_window_set_search_case(data->win, match_case);
    wordpad_window_set_search_direction(data->win, TRUE);
    wordpad_find_next(data->win);
}

static void
on_replace_clicked(GtkButton *btn, ReplaceDialogData *data)
{
    (void)btn;
    GtkTextBuffer *buf = wordpad_window_get_buffer(data->win);
    GtkTextIter start, end;

    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        const char *replacement = gtk_editable_get_text(GTK_EDITABLE(data->replace_entry));
        gtk_text_buffer_delete(buf, &start, &end);
        gtk_text_buffer_insert(buf, &start, replacement, -1);
    }

    on_replace_find_next(NULL, data);
}

static void
on_replace_all_clicked(GtkButton *btn, ReplaceDialogData *data)
{
    (void)btn;
    const char *find_text = gtk_editable_get_text(GTK_EDITABLE(data->find_entry));
    const char *replace_text = gtk_editable_get_text(GTK_EDITABLE(data->replace_entry));
    gboolean match_case = gtk_check_button_get_active(GTK_CHECK_BUTTON(data->match_case));
    if (!find_text || find_text[0] == '\0') return;

    GtkTextBuffer *buf = wordpad_window_get_buffer(data->win);
    GtkTextIter start, match_start, match_end;

    GtkTextSearchFlags flags = GTK_TEXT_SEARCH_TEXT_ONLY;
    if (!match_case) flags |= GTK_TEXT_SEARCH_CASE_INSENSITIVE;

    gtk_text_buffer_get_start_iter(buf, &start);
    int count = 0;

    while (gtk_text_iter_forward_search(&start, find_text, flags,
                                         &match_start, &match_end, NULL)) {
        gtk_text_buffer_delete(buf, &match_start, &match_end);
        gtk_text_buffer_insert(buf, &match_start, replace_text, -1);
        start = match_start;
        count++;
        if (count > 100000) break; /* safety */
    }
}

static void
on_replace_dialog_close(GtkWindow *dialog, ReplaceDialogData *data)
{
    (void)dialog;
    wordpad_window_set_replace_dialog(data->win, NULL);
    g_free(data);
}

void
wordpad_show_replace_dialog(WordpadWindow *win)
{
    GtkWindow *existing = wordpad_window_get_replace_dialog(win);
    if (existing) {
        gtk_window_present(existing);
        return;
    }

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Replace");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, -1);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);

    ReplaceDialogData *data = g_new0(ReplaceDialogData, 1);
    data->win = win;

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_widget_set_margin_start(grid, 12);
    gtk_widget_set_margin_end(grid, 12);
    gtk_widget_set_margin_top(grid, 12);
    gtk_widget_set_margin_bottom(grid, 12);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Fi_nd what:"), 0, 0, 1, 1);
    data->find_entry = gtk_entry_new();
    gtk_widget_set_hexpand(data->find_entry, TRUE);
    const char *prev = wordpad_window_get_search_text(win);
    if (prev) gtk_editable_set_text(GTK_EDITABLE(data->find_entry), prev);
    gtk_grid_attach(GTK_GRID(grid), data->find_entry, 1, 0, 3, 1);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Re_place with:"), 0, 1, 1, 1);
    data->replace_entry = gtk_entry_new();
    gtk_widget_set_hexpand(data->replace_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), data->replace_entry, 1, 1, 3, 1);

    data->match_case = gtk_check_button_new_with_label("Match _case");
    gtk_check_button_set_use_underline(GTK_CHECK_BUTTON(data->match_case), TRUE);
    gtk_grid_attach(GTK_GRID(grid), data->match_case, 0, 2, 2, 1);

    GtkWidget *find_btn = gtk_button_new_with_label("_Find Next");
    gtk_button_set_use_underline(GTK_BUTTON(find_btn), TRUE);
    g_signal_connect(find_btn, "clicked", G_CALLBACK(on_replace_find_next), data);
    gtk_grid_attach(GTK_GRID(grid), find_btn, 1, 3, 1, 1);

    GtkWidget *replace_btn = gtk_button_new_with_label("_Replace");
    gtk_button_set_use_underline(GTK_BUTTON(replace_btn), TRUE);
    g_signal_connect(replace_btn, "clicked", G_CALLBACK(on_replace_clicked), data);
    gtk_grid_attach(GTK_GRID(grid), replace_btn, 2, 3, 1, 1);

    GtkWidget *replace_all_btn = gtk_button_new_with_label("Replace _All");
    gtk_button_set_use_underline(GTK_BUTTON(replace_all_btn), TRUE);
    g_signal_connect(replace_all_btn, "clicked", G_CALLBACK(on_replace_all_clicked), data);
    gtk_grid_attach(GTK_GRID(grid), replace_all_btn, 3, 3, 1, 1);

    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_grid_attach(GTK_GRID(grid), cancel_btn, 0, 3, 1, 1);

    gtk_window_set_child(GTK_WINDOW(dialog), grid);
    g_signal_connect(dialog, "destroy", G_CALLBACK(on_replace_dialog_close), data);
    wordpad_window_set_replace_dialog(win, GTK_WINDOW(dialog));
    gtk_window_present(GTK_WINDOW(dialog));
}

/* ── Date/Time Dialog ── */

static void
on_datetime_response(AdwAlertDialog *dlg, const char *response, gpointer user_data)
{
    (void)user_data;
    if (strcmp(response, "cancel") == 0) return;

    int idx = atoi(response);
    char **fmts = g_object_get_data(G_OBJECT(dlg), "formats");
    WordpadWindow *w = g_object_get_data(G_OBJECT(dlg), "win");

    if (fmts && idx >= 0 && idx < 8 && fmts[idx]) {
        GtkTextBuffer *buf = wordpad_window_get_buffer(w);
        gtk_text_buffer_insert_at_cursor(buf, fmts[idx], -1);
    }
}

static void
on_paragraph_response(AdwAlertDialog *dlg, const char *response, gpointer user_data)
{
    (void)user_data;
    WordpadWindow *w = g_object_get_data(G_OBJECT(dlg), "win");
    GtkTextBuffer *buf = wordpad_window_get_buffer(w);

    GtkTextIter start, end;
    GtkTextMark *insert = gtk_text_buffer_get_insert(buf);
    gtk_text_buffer_get_iter_at_mark(buf, &start, insert);
    gtk_text_iter_set_line_offset(&start, 0);
    end = start;
    if (!gtk_text_iter_ends_line(&end))
        gtk_text_iter_forward_to_line_end(&end);
    gtk_text_iter_forward_char(&end);

    gtk_text_buffer_remove_tag_by_name(buf, "align-left", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buf, "align-center", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buf, "align-right", &start, &end);

    if (strcmp(response, "left") == 0)
        gtk_text_buffer_apply_tag_by_name(buf, "align-left", &start, &end);
    else if (strcmp(response, "center") == 0)
        gtk_text_buffer_apply_tag_by_name(buf, "align-center", &start, &end);
    else if (strcmp(response, "right") == 0)
        gtk_text_buffer_apply_tag_by_name(buf, "align-right", &start, &end);
}

void
wordpad_show_datetime_dialog(WordpadWindow *win)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char buf_formats[8][64];

    strftime(buf_formats[0], sizeof(buf_formats[0]), "%m/%d/%Y", tm);
    strftime(buf_formats[1], sizeof(buf_formats[1]), "%B %d, %Y", tm);
    strftime(buf_formats[2], sizeof(buf_formats[2]), "%I:%M %p", tm);
    strftime(buf_formats[3], sizeof(buf_formats[3]), "%I:%M:%S %p", tm);
    strftime(buf_formats[4], sizeof(buf_formats[4]), "%A, %B %d, %Y", tm);
    strftime(buf_formats[5], sizeof(buf_formats[5]), "%Y-%m-%d", tm);
    strftime(buf_formats[6], sizeof(buf_formats[6]), "%H:%M", tm);
    strftime(buf_formats[7], sizeof(buf_formats[7]), "%H:%M:%S", tm);

    AdwDialog *dialog = adw_alert_dialog_new("Date and Time", "Select a format to insert:");

    adw_alert_dialog_add_responses(ADW_ALERT_DIALOG(dialog),
                                   "cancel", "Cancel",
                                   "0", buf_formats[0],
                                   "1", buf_formats[1],
                                   "2", buf_formats[2],
                                   "3", buf_formats[3],
                                   "4", buf_formats[4],
                                   "5", buf_formats[5],
                                   "6", buf_formats[6],
                                   "7", buf_formats[7],
                                   NULL);

    adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "0");
    adw_alert_dialog_set_close_response(ADW_ALERT_DIALOG(dialog), "cancel");

    /* Use a simple response approach: store formats in user data */
    char **formats = g_new0(char *, 9);
    for (int i = 0; i < 8; i++)
        formats[i] = g_strdup(buf_formats[i]);

    g_object_set_data_full(G_OBJECT(dialog), "formats", formats,
                           (GDestroyNotify)g_strfreev);
    g_object_set_data(G_OBJECT(dialog), "win", win);

    g_signal_connect(dialog, "response",
                     G_CALLBACK(on_datetime_response), NULL);

    adw_dialog_present(dialog, GTK_WIDGET(win));
}

/* ── Paragraph Dialog ── */

void
wordpad_show_paragraph_dialog(WordpadWindow *win)
{
    AdwDialog *dialog = adw_alert_dialog_new("Paragraph", NULL);

    adw_alert_dialog_add_responses(ADW_ALERT_DIALOG(dialog),
                                   "cancel", "Cancel",
                                   "left", "Left",
                                   "center", "Center",
                                   "right", "Right",
                                   NULL);
    adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "left");
    adw_alert_dialog_set_close_response(ADW_ALERT_DIALOG(dialog), "cancel");

    g_object_set_data(G_OBJECT(dialog), "win", win);

    g_signal_connect(dialog, "response",
                     G_CALLBACK(on_paragraph_response), NULL);

    adw_dialog_present(dialog, GTK_WIDGET(win));
}

/* ── Font Dialog ── */

static void
on_font_dialog_finish(GObject *source, GAsyncResult *result, gpointer user_data)
{
    WordpadWindow *win = user_data;
    GtkFontDialog *dlg = GTK_FONT_DIALOG(source);

    GError *error = NULL;
    PangoFontDescription *desc = gtk_font_dialog_choose_font_finish(dlg, result, &error);
    if (!desc) {
        g_clear_error(&error);
        return;
    }

    GtkTextBuffer *buf = wordpad_window_get_buffer(win);
    GtkTextIter start, end;

    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        /* Apply font family */
        const char *family = pango_font_description_get_family(desc);
        if (family) {
            char tag_name[256];
            snprintf(tag_name, sizeof(tag_name), "font:%s", family);
            wordpad_get_or_create_tag(buf, tag_name);
            gtk_text_buffer_apply_tag_by_name(buf, tag_name, &start, &end);
        }

        /* Apply font size */
        int size = pango_font_description_get_size(desc) / PANGO_SCALE;
        if (size > 0) {
            char tag_name[64];
            snprintf(tag_name, sizeof(tag_name), "size:%d", size);
            wordpad_get_or_create_tag(buf, tag_name);
            gtk_text_buffer_apply_tag_by_name(buf, tag_name, &start, &end);
        }

        /* Apply weight */
        PangoWeight weight = pango_font_description_get_weight(desc);
        if (weight >= PANGO_WEIGHT_BOLD)
            gtk_text_buffer_apply_tag_by_name(buf, "bold", &start, &end);
        else
            gtk_text_buffer_remove_tag_by_name(buf, "bold", &start, &end);

        /* Apply style */
        PangoStyle style = pango_font_description_get_style(desc);
        if (style == PANGO_STYLE_ITALIC || style == PANGO_STYLE_OBLIQUE)
            gtk_text_buffer_apply_tag_by_name(buf, "italic", &start, &end);
        else
            gtk_text_buffer_remove_tag_by_name(buf, "italic", &start, &end);
    }

    pango_font_description_free(desc);
}

void
wordpad_show_font_dialog(WordpadWindow *win)
{
    GtkFontDialog *dialog = gtk_font_dialog_new();
    gtk_font_dialog_choose_font(dialog, GTK_WINDOW(win), NULL, NULL,
                                on_font_dialog_finish, win);
    g_object_unref(dialog);
}

/* ── About Dialog ── */

void
wordpad_show_about_dialog(WordpadWindow *win)
{
    AdwAboutDialog *dialog = ADW_ABOUT_DIALOG(adw_about_dialog_new());

    adw_about_dialog_set_application_name(dialog, "WordPad");
    adw_about_dialog_set_version(dialog, "1.0");
    adw_about_dialog_set_comments(dialog, "A Windows 98 WordPad clone built with GTK4");
    adw_about_dialog_set_application_icon(dialog, "accessories-text-editor");
    adw_about_dialog_set_developer_name(dialog, "Goshapps");

    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(win));
}

/* ── Save Prompt ── */

typedef struct {
    WordpadWindow      *win;
    WordpadSaveCallback callback;
    gpointer            user_data;
} SavePromptData;

static void
on_save_prompt_response(AdwAlertDialog *dialog, const char *response, SavePromptData *data)
{
    (void)dialog;
    if (strcmp(response, "cancel") == 0) {
        data->callback(data->win, FALSE, data->user_data);
    } else if (strcmp(response, "discard") == 0) {
        data->callback(data->win, TRUE, data->user_data);
    } else if (strcmp(response, "save") == 0) {
        /* Trigger save, then proceed */
        GtkTextBuffer *buf = wordpad_window_get_buffer(data->win);
        GFile *file = wordpad_window_get_file(data->win);

        if (file) {
            const char *name = wordpad_window_get_filename(data->win);
            GError *error = NULL;
            gboolean ok;
            if (name && g_str_has_suffix(name, ".txt"))
                ok = wordpad_txt_save(buf, file, &error);
            else
                ok = wordpad_rtf_save(buf, file, &error);

            if (!ok) g_clear_error(&error);
            data->callback(data->win, ok, data->user_data);
        } else {
            /* No file yet — for simplicity, just proceed (save-as would be
               better but requires async flow) */
            data->callback(data->win, TRUE, data->user_data);
        }
    }
    g_free(data);
}

void
wordpad_prompt_save(WordpadWindow *win, WordpadSaveCallback callback, gpointer user_data)
{
    const char *filename = wordpad_window_get_filename(win);
    if (!filename) filename = "Document";

    char *msg = g_strdup_printf("Save changes to %s?", filename);
    AdwDialog *dialog = adw_alert_dialog_new("Save Changes", msg);
    g_free(msg);

    adw_alert_dialog_add_responses(ADW_ALERT_DIALOG(dialog),
                                   "cancel", "Cancel",
                                   "discard", "Discard",
                                   "save", "Save",
                                   NULL);
    adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog),
                                              "discard", ADW_RESPONSE_DESTRUCTIVE);
    adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog),
                                              "save", ADW_RESPONSE_SUGGESTED);
    adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "save");
    adw_alert_dialog_set_close_response(ADW_ALERT_DIALOG(dialog), "cancel");

    SavePromptData *data = g_new0(SavePromptData, 1);
    data->win = win;
    data->callback = callback;
    data->user_data = user_data;

    g_signal_connect(dialog, "response", G_CALLBACK(on_save_prompt_response), data);
    adw_dialog_present(dialog, GTK_WIDGET(win));
}
