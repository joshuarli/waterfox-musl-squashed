#include <gtk/gtk.h>

static void activate(GtkApplication *app, gpointer user_data) {
  (void)user_data;

  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "wfx gtk popup repro");
  gtk_window_set_default_size(GTK_WINDOW(window), 1024, 768);

  GtkWidget *overlay = gtk_overlay_new();
  gtk_container_add(GTK_CONTAINER(window), overlay);

  GtkWidget *background = gtk_label_new("");
  gtk_widget_set_hexpand(background, TRUE);
  gtk_widget_set_vexpand(background, TRUE);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), background);

  GtkWidget *button = gtk_menu_button_new();
  gtk_button_set_label(GTK_BUTTON(button), "Menu");
  gtk_widget_set_halign(button, GTK_ALIGN_END);
  gtk_widget_set_valign(button, GTK_ALIGN_START);
  gtk_widget_set_margin_top(button, 36);
  gtk_widget_set_margin_end(button, 12);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), button);

  GtkWidget *menu = gtk_menu_new();
  const char *items[] = {"First item", "Second item", "Third item", NULL};
  for (const char **item = items; *item != NULL; item++) {
    GtkWidget *menu_item = gtk_menu_item_new_with_label(*item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  }
  gtk_widget_show_all(menu);
  gtk_menu_button_set_popup(GTK_MENU_BUTTON(button), menu);

  gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
  GtkApplication *app =
      gtk_application_new("net.waterfox.musl.GtkPopupRepro", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
