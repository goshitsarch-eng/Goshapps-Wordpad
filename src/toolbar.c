#include "toolbar.h"
#include "rtf.h"
#include <string.h>

/* ── Common fonts for the dropdown ── */
static const char * const FONT_NAMES[] = {
    "Arial", "Courier New", "Georgia", "Helvetica",
    "Liberation Mono", "Liberation Sans", "Liberation Serif",
    "Monospace", "Noto Sans", "Noto Serif",
    "Sans", "Serif", "Times New Roman", "Verdana",
    NULL
};

/* ── Toolbar button helper ── */
static GtkWidget *
make_button(const char *icon_name, const char *tooltip, const char *action)
{
    GtkWidget *btn = gtk_button_new_from_icon_name(icon_name);
    gtk_widget_set_tooltip_text(btn, tooltip);
    gtk_actionable_set_action_name(GTK_ACTIONABLE(btn), action);
    gtk_widget_add_css_class(btn, "flat");
    return btn;
}

static GtkWidget *
make_toggle(const char *icon_name, const char *tooltip)
{
    GtkWidget *btn = gtk_toggle_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(btn), icon_name);
    gtk_widget_set_tooltip_text(btn, tooltip);
    gtk_widget_add_css_class(btn, "flat");
    return btn;
}

static GtkWidget *
make_separator(void)
{
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_start(sep, 2);
    gtk_widget_set_margin_end(sep, 2);
    return sep;
}

/* ── Main Toolbar ── */

GtkWidget *
wordpad_toolbar_create(WordpadWindow *win)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_add_css_class(box, "toolbar");
    gtk_widget_set_margin_start(box, 4);
    gtk_widget_set_margin_end(box, 4);
    gtk_widget_set_margin_top(box, 2);
    gtk_widget_set_margin_bottom(box, 2);

    gtk_box_append(GTK_BOX(box), make_button("document-new-symbolic", "New (Ctrl+N)", "win.new"));
    gtk_box_append(GTK_BOX(box), make_button("document-open-symbolic", "Open (Ctrl+O)", "win.open"));
    gtk_box_append(GTK_BOX(box), make_button("document-save-symbolic", "Save (Ctrl+S)", "win.save"));
    gtk_box_append(GTK_BOX(box), make_button("document-print-symbolic", "Print (Ctrl+P)", "win.print"));
    gtk_box_append(GTK_BOX(box), make_button("document-print-preview-symbolic", "Print Preview", "win.print-preview"));
    gtk_box_append(GTK_BOX(box), make_button("edit-find-symbolic", "Find (Ctrl+F)", "win.find"));

    gtk_box_append(GTK_BOX(box), make_separator());

    gtk_box_append(GTK_BOX(box), make_button("edit-cut-symbolic", "Cut (Ctrl+X)", "win.cut"));
    gtk_box_append(GTK_BOX(box), make_button("edit-copy-symbolic", "Copy (Ctrl+C)", "win.copy"));
    gtk_box_append(GTK_BOX(box), make_button("edit-paste-symbolic", "Paste (Ctrl+V)", "win.paste"));

    gtk_box_append(GTK_BOX(box), make_separator());

    gtk_box_append(GTK_BOX(box), make_button("edit-undo-symbolic", "Undo (Ctrl+Z)", "win.undo"));

    gtk_box_append(GTK_BOX(box), make_separator());

    GtkWidget *dt_btn = make_button("x-office-calendar-symbolic", "Insert Date/Time", "win.insert-datetime");
    gtk_box_append(GTK_BOX(box), dt_btn);

    return box;
}

/* ── Format Bar toggle callbacks ── */

static void
on_bold_toggled(GtkToggleButton *btn, WordpadWindow *win)
{
    if (wordpad_window_get_updating_format(win)) return;
    GtkTextBuffer *buf = wordpad_window_get_buffer(win);
    GtkTextIter start, end;

    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        if (gtk_toggle_button_get_active(btn))
            gtk_text_buffer_apply_tag_by_name(buf, "bold", &start, &end);
        else
            gtk_text_buffer_remove_tag_by_name(buf, "bold", &start, &end);
    }
}

static void
on_italic_toggled(GtkToggleButton *btn, WordpadWindow *win)
{
    if (wordpad_window_get_updating_format(win)) return;
    GtkTextBuffer *buf = wordpad_window_get_buffer(win);
    GtkTextIter start, end;

    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        if (gtk_toggle_button_get_active(btn))
            gtk_text_buffer_apply_tag_by_name(buf, "italic", &start, &end);
        else
            gtk_text_buffer_remove_tag_by_name(buf, "italic", &start, &end);
    }
}

static void
on_underline_toggled(GtkToggleButton *btn, WordpadWindow *win)
{
    if (wordpad_window_get_updating_format(win)) return;
    GtkTextBuffer *buf = wordpad_window_get_buffer(win);
    GtkTextIter start, end;

    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        if (gtk_toggle_button_get_active(btn))
            gtk_text_buffer_apply_tag_by_name(buf, "underline", &start, &end);
        else
            gtk_text_buffer_remove_tag_by_name(buf, "underline", &start, &end);
    }
}

static void
on_align_toggled(GtkToggleButton *btn, WordpadWindow *win)
{
    if (wordpad_window_get_updating_format(win)) return;
    GtkTextBuffer *buf = wordpad_window_get_buffer(win);
    GtkTextIter start, end;

    /* Get paragraph bounds */
    GtkTextMark *insert = gtk_text_buffer_get_insert(buf);
    gtk_text_buffer_get_iter_at_mark(buf, &start, insert);
    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        /* Extend to paragraph boundaries */
        gtk_text_iter_set_line_offset(&start, 0);
        if (!gtk_text_iter_ends_line(&end))
            gtk_text_iter_forward_to_line_end(&end);
        gtk_text_iter_forward_char(&end);
    } else {
        gtk_text_iter_set_line_offset(&start, 0);
        end = start;
        if (!gtk_text_iter_ends_line(&end))
            gtk_text_iter_forward_to_line_end(&end);
        gtk_text_iter_forward_char(&end);
    }

    /* Remove all alignment tags */
    gtk_text_buffer_remove_tag_by_name(buf, "align-left", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buf, "align-center", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buf, "align-right", &start, &end);

    /* Apply the selected one */
    if (btn == GTK_TOGGLE_BUTTON(wordpad_window_get_align_left_btn(win)))
        gtk_text_buffer_apply_tag_by_name(buf, "align-left", &start, &end);
    else if (btn == GTK_TOGGLE_BUTTON(wordpad_window_get_align_center_btn(win)))
        gtk_text_buffer_apply_tag_by_name(buf, "align-center", &start, &end);
    else if (btn == GTK_TOGGLE_BUTTON(wordpad_window_get_align_right_btn(win)))
        gtk_text_buffer_apply_tag_by_name(buf, "align-right", &start, &end);
}

static void
on_bullet_toggled(GtkToggleButton *btn, WordpadWindow *win)
{
    if (wordpad_window_get_updating_format(win)) return;
    GtkTextBuffer *buf = wordpad_window_get_buffer(win);
    GtkTextIter start, end;

    GtkTextMark *insert_mark = gtk_text_buffer_get_insert(buf);
    gtk_text_buffer_get_iter_at_mark(buf, &start, insert_mark);
    gtk_text_iter_set_line_offset(&start, 0);
    end = start;
    if (!gtk_text_iter_ends_line(&end))
        gtk_text_iter_forward_to_line_end(&end);

    if (gtk_toggle_button_get_active(btn)) {
        /* Add bullet */
        gtk_text_buffer_insert(buf, &start, "\xe2\x80\xa2 ", -1); /* "• " */
        /* Re-get bounds after insert */
        gtk_text_buffer_get_iter_at_mark(buf, &start, insert_mark);
        gtk_text_iter_set_line_offset(&start, 0);
        end = start;
        if (!gtk_text_iter_ends_line(&end))
            gtk_text_iter_forward_to_line_end(&end);
        gtk_text_iter_forward_char(&end);
        gtk_text_buffer_apply_tag_by_name(buf, "bullet", &start, &end);
    } else {
        /* Remove bullet prefix if present */
        GtkTextIter line_end = end;
        char *line = gtk_text_buffer_get_text(buf, &start, &line_end, FALSE);
        if (g_str_has_prefix(line, "\xe2\x80\xa2 ")) {
            GtkTextIter del_end = start;
            /* "• " is 4 bytes (3 for bullet + 1 for space) */
            gtk_text_iter_forward_chars(&del_end, 2); /* 2 unicode chars: bullet + space */
            gtk_text_buffer_delete(buf, &start, &del_end);
        }
        g_free(line);
        /* Remove bullet tag */
        gtk_text_buffer_get_iter_at_mark(buf, &start, insert_mark);
        gtk_text_iter_set_line_offset(&start, 0);
        end = start;
        if (!gtk_text_iter_ends_line(&end))
            gtk_text_iter_forward_to_line_end(&end);
        gtk_text_iter_forward_char(&end);
        gtk_text_buffer_remove_tag_by_name(buf, "bullet", &start, &end);
    }
}

static void
on_font_changed(GtkDropDown *dropdown, GParamSpec *pspec, WordpadWindow *win)
{
    (void)pspec;
    if (wordpad_window_get_updating_format(win)) return;

    guint idx = gtk_drop_down_get_selected(dropdown);
    if (idx == GTK_INVALID_LIST_POSITION) return;
    const char *font_name = FONT_NAMES[idx];
    if (!font_name) return;

    GtkTextBuffer *buf = wordpad_window_get_buffer(win);
    GtkTextIter start, end;

    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        /* Remove existing font tags */
        GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buf);
        for (int i = 0; FONT_NAMES[i]; i++) {
            char tag_name[256];
            snprintf(tag_name, sizeof(tag_name), "font:%s", FONT_NAMES[i]);
            GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_name);
            if (tag)
                gtk_text_buffer_remove_tag(buf, tag, &start, &end);
        }

        char tag_name[256];
        snprintf(tag_name, sizeof(tag_name), "font:%s", font_name);
        wordpad_get_or_create_tag(buf, tag_name);
        gtk_text_buffer_apply_tag_by_name(buf, tag_name, &start, &end);
    }
}

static void
on_size_changed(GtkSpinButton *spin, WordpadWindow *win)
{
    if (wordpad_window_get_updating_format(win)) return;

    int size = gtk_spin_button_get_value_as_int(spin);
    if (size < 1) return;

    GtkTextBuffer *buf = wordpad_window_get_buffer(win);
    GtkTextIter start, end;

    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        /* Remove existing size tags (common sizes) */
        for (int s = 1; s <= 200; s++) {
            char tag_name[64];
            snprintf(tag_name, sizeof(tag_name), "size:%d", s);
            GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buf);
            GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_name);
            if (tag)
                gtk_text_buffer_remove_tag(buf, tag, &start, &end);
        }

        char tag_name[64];
        snprintf(tag_name, sizeof(tag_name), "size:%d", size);
        wordpad_get_or_create_tag(buf, tag_name);
        gtk_text_buffer_apply_tag_by_name(buf, tag_name, &start, &end);
    }
}

static void
on_color_set(GtkColorDialogButton *btn, GParamSpec *pspec, WordpadWindow *win)
{
    (void)pspec;
    if (wordpad_window_get_updating_format(win)) return;

    const GdkRGBA *rgba = gtk_color_dialog_button_get_rgba(btn);
    char hex[8];
    snprintf(hex, sizeof(hex), "#%02x%02x%02x",
             (int)(rgba->red * 255), (int)(rgba->green * 255), (int)(rgba->blue * 255));

    GtkTextBuffer *buf = wordpad_window_get_buffer(win);
    GtkTextIter start, end;

    if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        char tag_name[64];
        snprintf(tag_name, sizeof(tag_name), "color:%s", hex);
        wordpad_get_or_create_tag(buf, tag_name);
        gtk_text_buffer_apply_tag_by_name(buf, tag_name, &start, &end);
    }
}

/* ── Format Bar ── */

GtkWidget *
wordpad_format_bar_create(WordpadWindow *win)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_add_css_class(box, "toolbar");
    gtk_widget_set_margin_start(box, 4);
    gtk_widget_set_margin_end(box, 4);
    gtk_widget_set_margin_top(box, 2);
    gtk_widget_set_margin_bottom(box, 2);

    /* Font dropdown */
    GtkStringList *font_model = gtk_string_list_new(FONT_NAMES);
    GtkWidget *font_dropdown = gtk_drop_down_new(G_LIST_MODEL(font_model), NULL);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(font_dropdown), 0); /* Arial */
    gtk_widget_set_size_request(font_dropdown, 160, -1);
    gtk_widget_set_tooltip_text(font_dropdown, "Font");
    g_signal_connect(font_dropdown, "notify::selected", G_CALLBACK(on_font_changed), win);
    gtk_box_append(GTK_BOX(box), font_dropdown);

    /* Font size */
    GtkAdjustment *adj = gtk_adjustment_new(10, 1, 200, 1, 10, 0);
    GtkWidget *size_spin = gtk_spin_button_new(adj, 1, 0);
    gtk_widget_set_size_request(size_spin, 60, -1);
    gtk_widget_set_tooltip_text(size_spin, "Font Size");
    g_signal_connect(size_spin, "value-changed", G_CALLBACK(on_size_changed), win);
    gtk_box_append(GTK_BOX(box), size_spin);

    /* Bold / Italic / Underline */
    GtkWidget *bold_btn = make_toggle("format-text-bold-symbolic", "Bold (Ctrl+B)");
    g_signal_connect(bold_btn, "toggled", G_CALLBACK(on_bold_toggled), win);
    gtk_box_append(GTK_BOX(box), bold_btn);

    GtkWidget *italic_btn = make_toggle("format-text-italic-symbolic", "Italic (Ctrl+I)");
    g_signal_connect(italic_btn, "toggled", G_CALLBACK(on_italic_toggled), win);
    gtk_box_append(GTK_BOX(box), italic_btn);

    GtkWidget *underline_btn = make_toggle("format-text-underline-symbolic", "Underline (Ctrl+U)");
    g_signal_connect(underline_btn, "toggled", G_CALLBACK(on_underline_toggled), win);
    gtk_box_append(GTK_BOX(box), underline_btn);

    /* Color button */
    GtkColorDialog *color_dialog = gtk_color_dialog_new();
    GtkWidget *color_btn = gtk_color_dialog_button_new(color_dialog);
    gtk_widget_set_tooltip_text(color_btn, "Text Color");
    GdkRGBA black = {0, 0, 0, 1};
    gtk_color_dialog_button_set_rgba(GTK_COLOR_DIALOG_BUTTON(color_btn), &black);
    g_signal_connect(color_btn, "notify::rgba", G_CALLBACK(on_color_set), win);
    gtk_box_append(GTK_BOX(box), color_btn);

    gtk_box_append(GTK_BOX(box), make_separator());

    /* Alignment */
    GtkWidget *align_left = make_toggle("format-justify-left-symbolic", "Left (Ctrl+L)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(align_left), TRUE);
    g_signal_connect(align_left, "toggled", G_CALLBACK(on_align_toggled), win);
    gtk_box_append(GTK_BOX(box), align_left);

    GtkWidget *align_center = make_toggle("format-justify-center-symbolic", "Center (Ctrl+E)");
    gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(align_center), GTK_TOGGLE_BUTTON(align_left));
    g_signal_connect(align_center, "toggled", G_CALLBACK(on_align_toggled), win);
    gtk_box_append(GTK_BOX(box), align_center);

    GtkWidget *align_right = make_toggle("format-justify-right-symbolic", "Right (Ctrl+R)");
    gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(align_right), GTK_TOGGLE_BUTTON(align_left));
    g_signal_connect(align_right, "toggled", G_CALLBACK(on_align_toggled), win);
    gtk_box_append(GTK_BOX(box), align_right);

    gtk_box_append(GTK_BOX(box), make_separator());

    /* Bullet toggle */
    GtkWidget *bullet_btn = make_toggle("view-list-symbolic", "Bullets");
    g_signal_connect(bullet_btn, "toggled", G_CALLBACK(on_bullet_toggled), win);
    gtk_box_append(GTK_BOX(box), bullet_btn);

    /* Store widget references in window */
    wordpad_window_set_format_widgets(win,
                                      font_dropdown, size_spin,
                                      bold_btn, italic_btn, underline_btn,
                                      align_left, align_center, align_right,
                                      bullet_btn, color_btn);

    return box;
}
