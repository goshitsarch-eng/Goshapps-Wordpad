#include "rtf.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* ── Tag helpers ── */

void
wordpad_rtf_ensure_tags(GtkTextBuffer *buffer)
{
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);

    if (!gtk_text_tag_table_lookup(table, "bold"))
        gtk_text_buffer_create_tag(buffer, "bold",
                                   "weight", PANGO_WEIGHT_BOLD, NULL);
    if (!gtk_text_tag_table_lookup(table, "italic"))
        gtk_text_buffer_create_tag(buffer, "italic",
                                   "style", PANGO_STYLE_ITALIC, NULL);
    if (!gtk_text_tag_table_lookup(table, "underline"))
        gtk_text_buffer_create_tag(buffer, "underline",
                                   "underline", PANGO_UNDERLINE_SINGLE, NULL);
    if (!gtk_text_tag_table_lookup(table, "align-left"))
        gtk_text_buffer_create_tag(buffer, "align-left",
                                   "justification", GTK_JUSTIFY_LEFT, NULL);
    if (!gtk_text_tag_table_lookup(table, "align-center"))
        gtk_text_buffer_create_tag(buffer, "align-center",
                                   "justification", GTK_JUSTIFY_CENTER, NULL);
    if (!gtk_text_tag_table_lookup(table, "align-right"))
        gtk_text_buffer_create_tag(buffer, "align-right",
                                   "justification", GTK_JUSTIFY_RIGHT, NULL);
    if (!gtk_text_tag_table_lookup(table, "bullet"))
        gtk_text_buffer_create_tag(buffer, "bullet",
                                   "left-margin", 30,
                                   "indent", -15, NULL);
}

GtkTextTag *
wordpad_get_or_create_tag(GtkTextBuffer *buffer, const char *tag_name)
{
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_name);
    if (tag)
        return tag;

    /* font:FontName */
    if (g_str_has_prefix(tag_name, "font:")) {
        const char *font = tag_name + 5;
        tag = gtk_text_buffer_create_tag(buffer, tag_name,
                                         "family", font, NULL);
    }
    /* size:N (in points) */
    else if (g_str_has_prefix(tag_name, "size:")) {
        int pts = atoi(tag_name + 5);
        if (pts < 1) pts = 10;
        tag = gtk_text_buffer_create_tag(buffer, tag_name,
                                         "size", pts * PANGO_SCALE, NULL);
    }
    /* color:#rrggbb */
    else if (g_str_has_prefix(tag_name, "color:")) {
        tag = gtk_text_buffer_create_tag(buffer, tag_name,
                                         "foreground", tag_name + 6, NULL);
    }
    else {
        tag = gtk_text_buffer_create_tag(buffer, tag_name, NULL);
    }

    return tag;
}

/* ── RTF Parser ── */

#define MAX_FONTS  64
#define MAX_COLORS 64
#define MAX_STACK  64

typedef struct {
    gboolean bold;
    gboolean italic;
    gboolean underline;
    int      font_index;
    int      font_size;   /* half-points */
    int      color_index;
    int      alignment;   /* 0=left, 1=center, 2=right */
    gboolean bullet;
} RtfState;

typedef struct {
    GtkTextBuffer *buffer;
    char *data;
    int   pos;
    int   len;

    /* Font table */
    char *fonts[MAX_FONTS];
    int   font_count;

    /* Color table */
    struct { int r, g, b; } colors[MAX_COLORS];
    int color_count;

    /* State stack */
    RtfState stack[MAX_STACK];
    int      stack_depth;
    RtfState state;
} RtfParser;

static int
rtf_peek(RtfParser *p)
{
    return (p->pos < p->len) ? (unsigned char)p->data[p->pos] : -1;
}

static int
rtf_next(RtfParser *p)
{
    return (p->pos < p->len) ? (unsigned char)p->data[p->pos++] : -1;
}

static void
rtf_push(RtfParser *p)
{
    if (p->stack_depth < MAX_STACK)
        p->stack[p->stack_depth++] = p->state;
}

static void
rtf_pop(RtfParser *p)
{
    if (p->stack_depth > 0)
        p->state = p->stack[--p->stack_depth];
}

static void
apply_state_tags(RtfParser *p, GtkTextIter *start, GtkTextIter *end)
{
    GtkTextBuffer *buf = p->buffer;

    if (p->state.bold)
        gtk_text_buffer_apply_tag_by_name(buf, "bold", start, end);
    if (p->state.italic)
        gtk_text_buffer_apply_tag_by_name(buf, "italic", start, end);
    if (p->state.underline)
        gtk_text_buffer_apply_tag_by_name(buf, "underline", start, end);

    /* Font */
    if (p->state.font_index >= 0 && p->state.font_index < p->font_count &&
        p->fonts[p->state.font_index]) {
        char tag_name[256];
        snprintf(tag_name, sizeof(tag_name), "font:%s", p->fonts[p->state.font_index]);
        wordpad_get_or_create_tag(buf, tag_name);
        gtk_text_buffer_apply_tag_by_name(buf, tag_name, start, end);
    }

    /* Font size (RTF uses half-points, convert to points) */
    if (p->state.font_size > 0) {
        int pts = p->state.font_size / 2;
        if (pts < 1) pts = 1;
        char tag_name[64];
        snprintf(tag_name, sizeof(tag_name), "size:%d", pts);
        wordpad_get_or_create_tag(buf, tag_name);
        gtk_text_buffer_apply_tag_by_name(buf, tag_name, start, end);
    }

    /* Color */
    if (p->state.color_index > 0 && p->state.color_index <= p->color_count) {
        int idx = p->state.color_index - 1;
        char tag_name[64];
        snprintf(tag_name, sizeof(tag_name), "color:#%02x%02x%02x",
                 p->colors[idx].r, p->colors[idx].g, p->colors[idx].b);
        wordpad_get_or_create_tag(buf, tag_name);
        gtk_text_buffer_apply_tag_by_name(buf, tag_name, start, end);
    }

    /* Alignment */
    const char *align_tag = NULL;
    switch (p->state.alignment) {
    case 1: align_tag = "align-center"; break;
    case 2: align_tag = "align-right"; break;
    default: break; /* left is default */
    }
    if (align_tag)
        gtk_text_buffer_apply_tag_by_name(buf, align_tag, start, end);
}

static void
rtf_insert_char(RtfParser *p, gunichar ch)
{
    char utf8[7];
    int n = g_unichar_to_utf8(ch, utf8);
    utf8[n] = '\0';

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(p->buffer, &end);
    int offset = gtk_text_iter_get_offset(&end);
    gtk_text_buffer_insert(p->buffer, &end, utf8, n);

    GtkTextIter start;
    gtk_text_buffer_get_iter_at_offset(p->buffer, &start, offset);
    gtk_text_buffer_get_end_iter(p->buffer, &end);
    apply_state_tags(p, &start, &end);
}

/* Skip over a { ... } group (when we don't care about the content) */
static void
rtf_skip_group(RtfParser *p)
{
    int depth = 1;
    while (depth > 0 && p->pos < p->len) {
        int c = rtf_next(p);
        if (c == '{') depth++;
        else if (c == '}') depth--;
        else if (c == '\\') rtf_next(p); /* skip escaped char */
    }
}

/* Read a control word and optional numeric parameter */
static void
rtf_read_control(RtfParser *p, char *word, int word_size, int *param, gboolean *has_param)
{
    int wi = 0;
    *has_param = FALSE;
    *param = 0;

    while (p->pos < p->len && isalpha(rtf_peek(p)) && wi < word_size - 1) {
        word[wi++] = (char)rtf_next(p);
    }
    word[wi] = '\0';

    /* Check for optional numeric parameter (possibly negative) */
    if (p->pos < p->len && (isdigit(rtf_peek(p)) || rtf_peek(p) == '-')) {
        *has_param = TRUE;
        gboolean neg = FALSE;
        if (rtf_peek(p) == '-') {
            neg = TRUE;
            rtf_next(p);
        }
        while (p->pos < p->len && isdigit(rtf_peek(p))) {
            *param = *param * 10 + (rtf_next(p) - '0');
        }
        if (neg) *param = -*param;
    }

    /* Consume delimiter space */
    if (p->pos < p->len && rtf_peek(p) == ' ')
        rtf_next(p);
}

/* Parse {\fonttbl ...} */
static void
rtf_parse_fonttbl(RtfParser *p)
{
    int depth = 1;
    int current_font_idx = -1;
    GString *font_name = g_string_new(NULL);

    while (depth > 0 && p->pos < p->len) {
        int c = rtf_next(p);
        if (c == '{') {
            depth++;
            current_font_idx = -1;
            g_string_truncate(font_name, 0);
        } else if (c == '}') {
            depth--;
            if (current_font_idx >= 0 && current_font_idx < MAX_FONTS) {
                /* Remove trailing semicolon and whitespace */
                char *name = g_strstrip(g_strdup(font_name->str));
                int len = (int)strlen(name);
                if (len > 0 && name[len - 1] == ';')
                    name[len - 1] = '\0';
                p->fonts[current_font_idx] = name;
                if (current_font_idx >= p->font_count)
                    p->font_count = current_font_idx + 1;
            }
        } else if (c == '\\') {
            char word[64];
            int param;
            gboolean has_param;
            rtf_read_control(p, word, sizeof(word), &param, &has_param);
            if (strcmp(word, "f") == 0 && has_param)
                current_font_idx = param;
            /* skip other control words in fonttbl */
        } else if (c != '\r' && c != '\n') {
            g_string_append_c(font_name, (char)c);
        }
    }
    g_string_free(font_name, TRUE);
}

/* Parse {\colortbl ...} */
static void
rtf_parse_colortbl(RtfParser *p)
{
    int depth = 1;
    int r = 0, g = 0, b = 0;

    while (depth > 0 && p->pos < p->len) {
        int c = rtf_next(p);
        if (c == '{') { depth++; }
        else if (c == '}') { depth--; }
        else if (c == ';') {
            if (p->color_count < MAX_COLORS) {
                p->colors[p->color_count].r = r;
                p->colors[p->color_count].g = g;
                p->colors[p->color_count].b = b;
                p->color_count++;
            }
            r = g = b = 0;
        } else if (c == '\\') {
            char word[64];
            int param;
            gboolean has_param;
            rtf_read_control(p, word, sizeof(word), &param, &has_param);
            if (strcmp(word, "red") == 0 && has_param) r = param;
            else if (strcmp(word, "green") == 0 && has_param) g = param;
            else if (strcmp(word, "blue") == 0 && has_param) b = param;
        }
    }
}

static void
rtf_parse_body(RtfParser *p)
{
    while (p->pos < p->len) {
        int c = rtf_next(p);

        if (c == '{') {
            rtf_push(p);
            /* Check for special destinations */
            if (rtf_peek(p) == '\\') {
                int save_pos = p->pos;
                rtf_next(p); /* consume backslash */
                char word[64];
                int param;
                gboolean has_param;
                rtf_read_control(p, word, sizeof(word), &param, &has_param);

                if (strcmp(word, "fonttbl") == 0) {
                    rtf_parse_fonttbl(p);
                    rtf_pop(p);
                    continue;
                } else if (strcmp(word, "colortbl") == 0) {
                    rtf_parse_colortbl(p);
                    rtf_pop(p);
                    continue;
                } else if (strcmp(word, "stylesheet") == 0 ||
                           strcmp(word, "info") == 0 ||
                           strcmp(word, "pict") == 0 ||
                           strcmp(word, "object") == 0 ||
                           strcmp(word, "header") == 0 ||
                           strcmp(word, "footer") == 0 ||
                           strcmp(word, "footnote") == 0 ||
                           word[0] == '*') {
                    /* Skip unknown/unsupported destinations */
                    rtf_skip_group(p);
                    rtf_pop(p);
                    continue;
                } else {
                    /* Regular group — process the control word we just read */
                    /* Apply the control word effect */
                    p->pos = save_pos;
                }
            }
        } else if (c == '}') {
            rtf_pop(p);
        } else if (c == '\\') {
            char word[64];
            int param;
            gboolean has_param;

            int next = rtf_peek(p);
            if (next == '\'' ) {
                /* \'hh hex char */
                rtf_next(p);
                char hex[3] = {0};
                if (p->pos < p->len) hex[0] = (char)rtf_next(p);
                if (p->pos < p->len) hex[1] = (char)rtf_next(p);
                unsigned int val = 0;
                sscanf(hex, "%x", &val);
                if (val > 0)
                    rtf_insert_char(p, (gunichar)val);
                continue;
            }
            if (next == '\\' || next == '{' || next == '}') {
                rtf_next(p);
                rtf_insert_char(p, (gunichar)next);
                continue;
            }
            if (next == '\n' || next == '\r') {
                rtf_next(p);
                rtf_insert_char(p, '\n');
                continue;
            }

            rtf_read_control(p, word, sizeof(word), &param, &has_param);

            if (strcmp(word, "par") == 0 || strcmp(word, "line") == 0) {
                rtf_insert_char(p, '\n');
            } else if (strcmp(word, "tab") == 0) {
                rtf_insert_char(p, '\t');
            } else if (strcmp(word, "b") == 0) {
                p->state.bold = !has_param || param != 0;
            } else if (strcmp(word, "i") == 0) {
                p->state.italic = !has_param || param != 0;
            } else if (strcmp(word, "ul") == 0) {
                p->state.underline = !has_param || param != 0;
            } else if (strcmp(word, "ulnone") == 0) {
                p->state.underline = FALSE;
            } else if (strcmp(word, "f") == 0 && has_param) {
                p->state.font_index = param;
            } else if (strcmp(word, "fs") == 0 && has_param) {
                p->state.font_size = param;
            } else if (strcmp(word, "cf") == 0 && has_param) {
                p->state.color_index = param;
            } else if (strcmp(word, "ql") == 0) {
                p->state.alignment = 0;
            } else if (strcmp(word, "qc") == 0) {
                p->state.alignment = 1;
            } else if (strcmp(word, "qr") == 0) {
                p->state.alignment = 2;
            } else if (strcmp(word, "pard") == 0) {
                p->state.bold = FALSE;
                p->state.italic = FALSE;
                p->state.underline = FALSE;
                p->state.font_index = 0;
                p->state.font_size = 0;
                p->state.color_index = 0;
                p->state.alignment = 0;
                p->state.bullet = FALSE;
            } else if (strcmp(word, "pntext") == 0) {
                p->state.bullet = TRUE;
            }
            /* Skip unknown control words */
        } else if (c == '\r' || c == '\n') {
            /* Ignore bare CR/LF in RTF */
        } else {
            rtf_insert_char(p, (gunichar)c);
        }
    }
}

gboolean
wordpad_rtf_load(GtkTextBuffer *buffer, GFile *file, GError **error)
{
    char *contents = NULL;
    gsize length = 0;

    if (!g_file_load_contents(file, NULL, &contents, &length, NULL, error))
        return FALSE;

    wordpad_rtf_ensure_tags(buffer);
    gtk_text_buffer_set_text(buffer, "", 0);

    RtfParser parser = {0};
    parser.buffer = buffer;
    parser.data = contents;
    parser.pos = 0;
    parser.len = (int)length;
    parser.state.font_index = -1;
    parser.state.font_size = 20; /* 10pt default */

    /* Skip initial { and \rtf1 header */
    if (parser.pos < parser.len && parser.data[parser.pos] == '{') {
        parser.pos++;
        if (parser.pos < parser.len && parser.data[parser.pos] == '\\') {
            parser.pos++;
            char word[64];
            int param;
            gboolean has_param;
            rtf_read_control(&parser, word, sizeof(word), &param, &has_param);
            /* Now we're past \rtf1 */
        }
    }

    rtf_parse_body(&parser);

    /* Clean up font table */
    for (int i = 0; i < parser.font_count; i++)
        g_free(parser.fonts[i]);

    g_free(contents);

    gtk_text_buffer_set_modified(buffer, FALSE);
    return TRUE;
}

/* ── RTF Writer ── */

typedef struct {
    char *name;
    int   index;
} FontEntry;

typedef struct {
    int r, g, b;
    int index;
} ColorEntry;

typedef struct {
    GString     *output;
    GtkTextBuffer *buffer;

    FontEntry   fonts[MAX_FONTS];
    int         font_count;

    ColorEntry  colors[MAX_COLORS];
    int         color_count;
} RtfWriter;

static int
writer_get_font_index(RtfWriter *w, const char *font_name)
{
    for (int i = 0; i < w->font_count; i++) {
        if (strcmp(w->fonts[i].name, font_name) == 0)
            return w->fonts[i].index;
    }
    if (w->font_count < MAX_FONTS) {
        int idx = w->font_count;
        w->fonts[w->font_count].name = g_strdup(font_name);
        w->fonts[w->font_count].index = idx;
        w->font_count++;
        return idx;
    }
    return 0;
}

static int
writer_get_color_index(RtfWriter *w, int r, int g, int b)
{
    for (int i = 0; i < w->color_count; i++) {
        if (w->colors[i].r == r && w->colors[i].g == g && w->colors[i].b == b)
            return w->colors[i].index;
    }
    if (w->color_count < MAX_COLORS) {
        int idx = w->color_count + 1; /* RTF color indices are 1-based */
        w->colors[w->color_count].r = r;
        w->colors[w->color_count].g = g;
        w->colors[w->color_count].b = b;
        w->colors[w->color_count].index = idx;
        w->color_count++;
        return idx;
    }
    return 1;
}

/* Pre-scan buffer to build font and color tables */
static void
writer_scan_tags(RtfWriter *w)
{
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(w->buffer);
    (void)table;

    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(w->buffer, &iter);

    /* Always include a default font */
    writer_get_font_index(w, "Arial");

    while (!gtk_text_iter_is_end(&iter)) {
        GSList *tags = gtk_text_iter_get_tags(&iter);
        for (GSList *l = tags; l; l = l->next) {
            GtkTextTag *tag = l->data;
            char *name = NULL;
            g_object_get(tag, "name", &name, NULL);
            if (!name) continue;

            if (g_str_has_prefix(name, "font:")) {
                writer_get_font_index(w, name + 5);
            } else if (g_str_has_prefix(name, "color:")) {
                const char *hex = name + 6;
                if (hex[0] == '#' && strlen(hex) == 7) {
                    unsigned int rv, gv, bv;
                    sscanf(hex + 1, "%02x%02x%02x", &rv, &gv, &bv);
                    writer_get_color_index(w, (int)rv, (int)gv, (int)bv);
                }
            }
            g_free(name);
        }
        g_slist_free(tags);
        gtk_text_iter_forward_char(&iter);
    }
}

static void
writer_emit_header(RtfWriter *w)
{
    g_string_append(w->output, "{\\rtf1\\ansi\\deff0\n");

    /* Font table */
    g_string_append(w->output, "{\\fonttbl");
    for (int i = 0; i < w->font_count; i++) {
        g_string_append_printf(w->output, "{\\f%d %s;}",
                               w->fonts[i].index, w->fonts[i].name);
    }
    g_string_append(w->output, "}\n");

    /* Color table */
    g_string_append(w->output, "{\\colortbl;");
    for (int i = 0; i < w->color_count; i++) {
        g_string_append_printf(w->output, "\\red%d\\green%d\\blue%d;",
                               w->colors[i].r, w->colors[i].g, w->colors[i].b);
    }
    g_string_append(w->output, "}\n");
}

static void
writer_emit_char_formatting(RtfWriter *w, GSList *tags)
{
    gboolean bold = FALSE, italic = FALSE, underline = FALSE;
    int size = 0;
    int color_idx = 0;
    int alignment = -1;

    for (GSList *l = tags; l; l = l->next) {
        GtkTextTag *tag = l->data;
        char *name = NULL;
        g_object_get(tag, "name", &name, NULL);
        if (!name) continue;

        if (strcmp(name, "bold") == 0) bold = TRUE;
        else if (strcmp(name, "italic") == 0) italic = TRUE;
        else if (strcmp(name, "underline") == 0) underline = TRUE;
        else if (g_str_has_prefix(name, "size:")) size = atoi(name + 5);
        else if (g_str_has_prefix(name, "color:")) {
            const char *hex = name + 6;
            if (hex[0] == '#' && strlen(hex) == 7) {
                unsigned int rv, gv, bv;
                sscanf(hex + 1, "%02x%02x%02x", &rv, &gv, &bv);
                color_idx = writer_get_color_index(w, (int)rv, (int)gv, (int)bv);
            }
        }
        else if (strcmp(name, "align-center") == 0) alignment = 1;
        else if (strcmp(name, "align-right") == 0) alignment = 2;
        else if (strcmp(name, "align-left") == 0) alignment = 0;

        /* We need to be careful: name was allocated by g_object_get but
           the pointers we saved (font) point into it. We'll use them before
           this function returns, so we can free at the end of the loop
           iteration ONLY if we didn't save a pointer. For simplicity,
           we accept a small leak here and free properly below. */
        g_free(name);
    }

    if (bold) g_string_append(w->output, "\\b");
    if (italic) g_string_append(w->output, "\\i");
    if (underline) g_string_append(w->output, "\\ul");

    /* Font - re-lookup since we freed name above */
    for (GSList *l = tags; l; l = l->next) {
        GtkTextTag *tag = l->data;
        char *name = NULL;
        g_object_get(tag, "name", &name, NULL);
        if (name && g_str_has_prefix(name, "font:")) {
            int fi = writer_get_font_index(w, name + 5);
            g_string_append_printf(w->output, "\\f%d", fi);
            g_free(name);
            break;
        }
        g_free(name);
    }

    if (size > 0)
        g_string_append_printf(w->output, "\\fs%d", size * 2);
    if (color_idx > 0)
        g_string_append_printf(w->output, "\\cf%d", color_idx);
    if (alignment == 1)
        g_string_append(w->output, "\\qc");
    else if (alignment == 2)
        g_string_append(w->output, "\\qr");

    g_string_append_c(w->output, ' ');
}

gboolean
wordpad_rtf_save(GtkTextBuffer *buffer, GFile *file, GError **error)
{
    RtfWriter writer = {0};
    writer.output = g_string_new(NULL);
    writer.buffer = buffer;

    writer_scan_tags(&writer);
    writer_emit_header(&writer);

    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(buffer, &iter);

    /* Track previous tags to detect changes */
    GSList *prev_tags = NULL;
    gboolean first = TRUE;

    while (!gtk_text_iter_is_end(&iter)) {
        GSList *tags = gtk_text_iter_get_tags(&iter);

        /* Check if tags changed */
        gboolean tags_changed = first;
        if (!first) {
            /* Simple comparison: if list lengths differ or any tag differs */
            GSList *a = prev_tags, *b = tags;
            while (a && b) {
                if (a->data != b->data) { tags_changed = TRUE; break; }
                a = a->next; b = b->next;
            }
            if (a || b) tags_changed = TRUE;
        }

        if (tags_changed) {
            if (!first) {
                /* Reset formatting */
                g_string_append(writer.output, "\\plain ");
            }
            writer_emit_char_formatting(&writer, tags);
            first = FALSE;
        }

        gunichar ch = gtk_text_iter_get_char(&iter);
        if (ch == '\n') {
            g_string_append(writer.output, "\\par\n");
        } else if (ch == '\t') {
            g_string_append(writer.output, "\\tab ");
        } else if (ch == '\\') {
            g_string_append(writer.output, "\\\\");
        } else if (ch == '{') {
            g_string_append(writer.output, "\\{");
        } else if (ch == '}') {
            g_string_append(writer.output, "\\}");
        } else if (ch > 127) {
            /* Encode as Unicode */
            g_string_append_printf(writer.output, "\\u%d?", (int)ch);
        } else {
            g_string_append_c(writer.output, (char)ch);
        }

        g_slist_free(prev_tags);
        prev_tags = tags;
        gtk_text_iter_forward_char(&iter);
    }
    g_slist_free(prev_tags);

    g_string_append(writer.output, "}\n");

    gboolean result = g_file_replace_contents(file,
                                               writer.output->str,
                                               writer.output->len,
                                               NULL, FALSE,
                                               G_FILE_CREATE_NONE,
                                               NULL, NULL, error);

    /* Clean up */
    for (int i = 0; i < writer.font_count; i++)
        g_free(writer.fonts[i].name);
    g_string_free(writer.output, TRUE);

    if (result)
        gtk_text_buffer_set_modified(buffer, FALSE);

    return result;
}

/* ── Plain text ── */

gboolean
wordpad_txt_load(GtkTextBuffer *buffer, GFile *file, GError **error)
{
    char *contents = NULL;
    gsize length = 0;

    if (!g_file_load_contents(file, NULL, &contents, &length, NULL, error))
        return FALSE;

    /* Validate UTF-8, convert if needed */
    if (!g_utf8_validate(contents, (gssize)length, NULL)) {
        gsize bytes_written;
        char *utf8 = g_locale_to_utf8(contents, (gssize)length,
                                       NULL, &bytes_written, NULL);
        g_free(contents);
        if (utf8) {
            contents = utf8;
            length = bytes_written;
        } else {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                        "File is not valid UTF-8 text");
            return FALSE;
        }
    }

    gtk_text_buffer_set_text(buffer, contents, (int)length);
    g_free(contents);
    gtk_text_buffer_set_modified(buffer, FALSE);
    return TRUE;
}

gboolean
wordpad_txt_save(GtkTextBuffer *buffer, GFile *file, GError **error)
{
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    gboolean result = g_file_replace_contents(file, text, strlen(text),
                                               NULL, FALSE,
                                               G_FILE_CREATE_NONE,
                                               NULL, NULL, error);
    g_free(text);

    if (result)
        gtk_text_buffer_set_modified(buffer, FALSE);

    return result;
}
