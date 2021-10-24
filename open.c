#include <gtk/gtk.h>
#include "declarations.h"


extern char root_dir[100];


//GtkWidget *openfile_revealer;
GtkWidget *openfile_entry;


gboolean on_open_window_keypress_event(GtkWidget *open_window, GdkEvent *event, gpointer user_data)
{
	#define ESCAPE 9
	#define UP 111
	#define DOWN 116
	#define ENTER 36

	GdkEventKey *key_event = (GdkEventKey *) event;
	guint16 keycode = key_event->hardware_keycode;

	LOG_MSG("on_open_window_keypress_event(): keycode: %d\n", keycode);
	
	if (keycode == ESCAPE) {
		gtk_widget_destroy(open_window);
	} else if (keycode == ENTER) {
		printf("Enter!!!\n");
		char filepath[100];

		const char *rel_filepath = gtk_entry_get_text(GTK_ENTRY(openfile_entry));
		snprintf(filepath, 100, "%s/%s", root_dir, rel_filepath);
		//@ what if its not a file or doesnt exist...?		
		create_tab(filepath);
	}

	return FALSE;
}


gboolean toggle_openfile(GdkEventKey *key_event)
{
	printf("Hello world!\n");

	GtkWidget *open_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_modal(GTK_WINDOW(open_window), TRUE);
	gtk_window_set_decorated(GTK_WINDOW(open_window), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(open_window), 300, 0);

	/*
	On a keypress event only active window's handler will be called,
	and by making our window modal, we make sure that as long as our window exists,
	its our handler which will be called (never the main window's handler).
	*/
	g_signal_connect(open_window, "key-press-event",
		G_CALLBACK(on_open_window_keypress_event), NULL);

	openfile_entry = gtk_entry_new();
	GtkWidget *window_message = gtk_label_new("This is a window");
	GtkWidget *container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	add_class(openfile_entry, "text-entry-orange");

	gtk_container_add(GTK_CONTAINER(container), openfile_entry);
	gtk_container_add(GTK_CONTAINER(container), window_message);
	gtk_container_add(GTK_CONTAINER(open_window), container);

	gtk_widget_show_all(open_window);

	return TRUE;
}

/*
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
*/
/*
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
*/