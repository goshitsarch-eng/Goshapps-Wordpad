#include "mainwindow.h"
#include <string.h>

static void
on_activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    /* Load menu model from menus.ui */
    /* Find menus.ui relative to executable or source dir */
    const char *paths[] = {
        "src/menus.ui",
        "../src/menus.ui",
        NULL
    };

    GtkBuilder *builder = NULL;
    for (int i = 0; paths[i]; i++) {
        builder = gtk_builder_new_from_file(paths[i]);
        if (builder) {
            GObject *obj = gtk_builder_get_object(builder, "menubar");
            if (obj) break;
            g_object_unref(builder);
            builder = NULL;
        }
    }

    if (!builder) {
        /* Try absolute path based on compile location */
        builder = gtk_builder_new();
    }

    GMenuModel *menubar = NULL;
    GObject *obj = gtk_builder_get_object(builder, "menubar");
    if (obj)
        menubar = G_MENU_MODEL(obj);

    if (menubar)
        gtk_application_set_menubar(app, menubar);

    gtk_application_window_set_show_menubar(
        GTK_APPLICATION_WINDOW(wordpad_window_new(ADW_APPLICATION(app))), TRUE);

    /* Present the window */
    GtkWindow *win = gtk_application_get_active_window(app);
    gtk_window_present(win);

    g_object_unref(builder);
}

int
main(int argc, char *argv[])
{
    g_autoptr(AdwApplication) app = adw_application_new(
        "com.goshapps.wordpad", G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    /* Keyboard shortcuts */
    const char *new_accels[] = {"<Control>n", NULL};
    const char *open_accels[] = {"<Control>o", NULL};
    const char *save_accels[] = {"<Control>s", NULL};
    const char *save_as_accels[] = {"<Control><Shift>s", NULL};
    const char *print_accels[] = {"<Control>p", NULL};
    const char *undo_accels[] = {"<Control>z", NULL};
    const char *redo_accels[] = {"<Control>y", NULL};
    const char *cut_accels[] = {"<Control>x", NULL};
    const char *copy_accels[] = {"<Control>c", NULL};
    const char *paste_accels[] = {"<Control>v", NULL};
    const char *select_all_accels[] = {"<Control>a", NULL};
    const char *find_accels[] = {"<Control>f", NULL};
    const char *find_next_accels[] = {"F3", NULL};
    const char *replace_accels[] = {"<Control>h", NULL};
    const char *bold_accels[] = {"<Control>b", NULL};
    const char *italic_accels[] = {"<Control>i", NULL};
    const char *underline_accels[] = {"<Control>u", NULL};
    const char *align_left_accels[] = {"<Control>l", NULL};
    const char *align_center_accels[] = {"<Control>e", NULL};
    const char *align_right_accels[] = {"<Control>r", NULL};

    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.new", new_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.open", open_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.save", save_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.save-as", save_as_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.print", print_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.undo", undo_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.redo", redo_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.cut", cut_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.copy", copy_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.paste", paste_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.select-all", select_all_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.find", find_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.find-next", find_next_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.replace", replace_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.toggle-bold", bold_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.toggle-italic", italic_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.toggle-underline", underline_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.align-left", align_left_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.align-center", align_center_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.align-right", align_right_accels);

    return g_application_run(G_APPLICATION(app), argc, argv);
}
