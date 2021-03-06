#include <gtk/gtk.h>
#include <stdlib.h>

#include "declarations.h"


extern char root_dir[100];

extern const char *home_icon_path;
extern const char *parent_dir_icon_path;

GtkWidget *root_dir_label;


void on_home_button_clicked(GtkButton *button, gpointer user_data)
{
	set_root_dir(getenv("HOME"));
}

void on_back_button_clicked(GtkButton *button, gpointer user_data)
{
	char *parent_path = get_parent_path(root_dir);
	set_root_dir(parent_path);
}

GtkWidget *create_root_nav_widget()
{
	root_dir_label = gtk_label_new(root_dir);

	//GtkWidget *home_button = gtk_button_new_with_label("~");
	GtkWidget *home_button = gtk_button_new();
	GtkWidget *house_image = gtk_image_new_from_file(home_icon_path);
	gtk_button_set_image(GTK_BUTTON(home_button), house_image);
	g_signal_connect(G_OBJECT(home_button), "clicked", G_CALLBACK(on_home_button_clicked), NULL);

	//GtkWidget *back_button = gtk_button_new_with_label("..");
	GtkWidget *back_button = gtk_button_new();
	GtkWidget *back_image = gtk_image_new_from_file(parent_dir_icon_path);
	gtk_button_set_image(GTK_BUTTON(back_button), back_image);
	g_signal_connect(G_OBJECT(back_button), "clicked", G_CALLBACK(on_back_button_clicked), NULL);
/*
	gtk_widget_set_size_request(home_button, 36, 36);
	gtk_widget_set_size_request(back_button, 36, 36);
*/
	gtk_widget_set_tooltip_text(home_button, "Home Directory");
	gtk_widget_set_tooltip_text(back_button, "Parent Directory");

	add_class(home_button, "home-dir-button");
	add_class(back_button, "parent-dir-button");
	add_class(root_dir_label, "root-dir-label");

	GtkWidget *buttons_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_container_add(GTK_CONTAINER(buttons_container), back_button);
	gtk_container_add(GTK_CONTAINER(buttons_container), home_button);

	GtkWidget *buttons_container_v = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget *margin_top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget *margin_bottom = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_size_request(margin_top, -1, 3);
	gtk_widget_set_size_request(margin_bottom, -1, 3);
	gtk_container_add(GTK_CONTAINER(buttons_container_v), margin_top);
	gtk_container_add(GTK_CONTAINER(buttons_container_v), buttons_container);
	gtk_container_add(GTK_CONTAINER(buttons_container_v), margin_bottom);

	GtkWidget *root_nav_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	GtkWidget *space = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(space, 0, -1);
	gtk_container_add(GTK_CONTAINER(root_nav_container), space);
	gtk_container_add(GTK_CONTAINER(root_nav_container), buttons_container_v);
	gtk_container_add(GTK_CONTAINER(root_nav_container), root_dir_label);

	return root_nav_container;
}


