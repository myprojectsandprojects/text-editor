#include <gtk/gtk.h>

#include "declarations.h"

extern char root_dir[100];

GtkWidget *root_dir_label;

void on_home_button_clicked(GtkButton *button, gpointer user_data)
{
	set_root_dir("/home/eero");
}

void on_back_button_clicked(GtkButton *button, gpointer user_data)
{
	char *parent_path = get_parent_path(root_dir);
	set_root_dir(parent_path);
}

GtkWidget *create_root_nav_widget()
{
	root_dir_label = gtk_label_new(root_dir);

	GtkWidget *home_button = gtk_button_new();
	GtkWidget *home_image = gtk_image_new_from_file("icons/colors/home.png");
	gtk_button_set_image(GTK_BUTTON(home_button), home_image);
	g_signal_connect(G_OBJECT(home_button), "clicked", G_CALLBACK(on_home_button_clicked), NULL);

	GtkWidget *back_button = gtk_button_new();
	GtkWidget *back_image = gtk_image_new_from_file("icons/colors/back.png");
	gtk_button_set_image(GTK_BUTTON(back_button), back_image);
	g_signal_connect(G_OBJECT(back_button), "clicked", G_CALLBACK(on_back_button_clicked), NULL);

	GtkWidget *buttons_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add(GTK_CONTAINER(buttons_container), back_button);
	gtk_container_add(GTK_CONTAINER(buttons_container), home_button);

	GtkWidget *root_nav_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_container_add(GTK_CONTAINER(root_nav_container), buttons_container);
	gtk_container_add(GTK_CONTAINER(root_nav_container), root_dir_label);

	return root_nav_container;
}


