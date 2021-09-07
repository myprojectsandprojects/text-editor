#include <gtk/gtk.h>
#include "declarations.h"


GtkWidget *openfile_revealer;
GtkWidget *openfile_entry;


gboolean toggle_openfile(GdkEventKey *key_event)
{
	printf("toggle_openfile()\n");

	if (!gtk_revealer_get_reveal_child(GTK_REVEALER(openfile_revealer))) { // not revealed
		gtk_revealer_set_reveal_child(GTK_REVEALER(openfile_revealer), TRUE);
		gtk_widget_grab_focus(openfile_entry);
	} else if(gtk_widget_is_focus(openfile_entry)) { // revealed & focus
		gtk_revealer_set_reveal_child(GTK_REVEALER(openfile_revealer), FALSE);
		//gtk_widget_grab_focus(text_view);
	} else {
		gtk_widget_grab_focus(openfile_entry);
	}

	return TRUE;
}


GtkWidget *create_openfile_widget(void)
{
	printf("create_openfile_widget()\n");

	openfile_revealer = gtk_revealer_new();
	openfile_entry = gtk_entry_new();
//	gtk_revealer_set_transition_type(GTK_REVEALER(openfile_revealer),
//		GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);

	gtk_widget_set_name(openfile_entry, "openfile-entry");
	add_class(openfile_entry, "text-entry-orange");
	
	gtk_container_add(GTK_CONTAINER(openfile_revealer), openfile_entry);

	return openfile_revealer;
}