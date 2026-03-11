#pragma once

#include "mainwindow.h"

void wordpad_show_find_dialog(WordpadWindow *win);
void wordpad_show_replace_dialog(WordpadWindow *win);
void wordpad_show_datetime_dialog(WordpadWindow *win);
void wordpad_show_paragraph_dialog(WordpadWindow *win);
void wordpad_show_font_dialog(WordpadWindow *win);
void wordpad_show_about_dialog(WordpadWindow *win);

/* Prompt to save before destructive action. Calls callback with TRUE if OK to proceed. */
typedef void (*WordpadSaveCallback)(WordpadWindow *win, gboolean proceed, gpointer user_data);
void wordpad_prompt_save(WordpadWindow *win, WordpadSaveCallback callback, gpointer user_data);

/* Find next from stored search parameters */
void wordpad_find_next(WordpadWindow *win);
