#include <gtk/gtk.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "declarations.h"


extern char root_dir[100];

GtkWidget *open_window;
GtkWidget *container;
GtkWidget *openfile_entry;
GtkWidget *scrollbars;
GtkWidget *file_paths;

gboolean multiple_matches;
int selected_item;

const int openfile_dialog_width = 800;
const int openfile_dialog_height = 500;


/*@ It might very well be that reading the pipe is not the most sensible way to do this
from the point of view of performance. */
void on_openfile_entry_changed(GtkEntry *search_entry, gpointer data)
{
	
	LOG_MSG("on_openfile_entry_changed()\n");


	/*
	gtk_bin_get_child(GTK_BIN(scrollbars)) returns GTK_VIEWPORT
	file_paths is a GTK_BOX
	*/

	if (file_paths) {
		GtkWidget *child = gtk_bin_get_child(GTK_BIN(scrollbars));
		gtk_widget_destroy(child);
		//gtk_widget_destroy(file_paths);
		file_paths = NULL;
	}

/*
	GtkContainer *paths_list = GTK_CONTAINER(data);
	GList *l = gtk_container_get_children(paths_list);
	for (GList *p = l; p != NULL; p = p->next) {
		//GtkLabel* label = (GtkLabel *) p->data;
		//gchar *text = gtk_label_get_label(label);
		//puts(text);
		gtk_widget_destroy(GTK_WIDGET(p->data));
	}
*/

	multiple_matches = FALSE;
	selected_item = -1;

	const char *text = gtk_entry_get_text(search_entry);
/*
	if (strlen(text) < 3) {
		//gtk_window_resize(GTK_WINDOW(open_window), openfile_dialog_width, 1);
		gtk_widget_show_all(open_window);
		return;
	}
*/
	printf("on_openfile_entry_changed(): %s\n", text);

	char command[100];
	//snprintf(command, 100, "locate -b %s | grep ^/home/eero/all/text-editor", text);
	//snprintf(command, 100, "locate -b %s | grep ^/home/eero/all", text);
	snprintf(command, 100, "locate -b %s | grep ^%s/", text, root_dir);


	//#define MAX_RESULTS 10000
	#define MAX_RESULTS 1000000
	char results[MAX_RESULTS];

	FILE* s = popen(command, "r");
	if (s) {
		int i = 0;
		while ((results[i] = fgetc(s)) != EOF) {
			i += 1;
			if (i > MAX_RESULTS - 1) break;
		}

		int c = 0;
		while (fgetc(s) != EOF) {
			//printf("%d\n", c);
			c += 1;
		}

		results[i] = 0;
		pclose(s);

		//printf("results: %s\n", results);

		file_paths = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_container_add(GTK_CONTAINER(scrollbars), file_paths);


		char *p = results;
		char *line;

		int lines = 0;

		line = get_slice_by(&p, '\n');
		if (line) {
			GtkWidget *l = gtk_label_new(line);
			gtk_container_add(GTK_CONTAINER(file_paths), l);
			
			gtk_widget_set_halign(l, GTK_ALIGN_START);
			add_class(l, "openfile-dialog-label-selected");

			selected_item = 0;

			lines += 1;
		}

		while ((line = get_slice_by(&p, '\n')) != NULL) {
			GtkWidget *l = gtk_label_new(line);
			gtk_container_add(GTK_CONTAINER(file_paths), l);
			
			gtk_widget_set_halign(l, GTK_ALIGN_START);
			add_class(l, "openfile-dialog-label");

			multiple_matches = TRUE;

			lines += 1;
		}

		printf("proccessed %d lines\n", lines);
		if (c == 0)
			printf("proccessed all\n");
		else
			printf("didnt proccess all\n");

		//gtk_window_resize(GTK_WINDOW(open_window), openfile_dialog_width, 1);
		gtk_widget_show_all(open_window);

	} else {
		//printf("on_openfile_entry_changed(): error!\n");
		assert(FALSE);
	}
}


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
		file_paths = NULL;
	} else if (keycode == ENTER) {
/*
		char filepath[100];

		const char *rel_filepath = gtk_entry_get_text(GTK_ENTRY(openfile_entry));
		snprintf(filepath, 100, "%s/%s", root_dir, rel_filepath);
		//@ what if its not a file or doesnt exist...?		
		create_tab(filepath);
*/

		if (selected_item == -1) {
			printf("on_open_window_keypress_event(): no selected item..\n");
			return TRUE;
		}

		GList *l = gtk_container_get_children(GTK_CONTAINER(file_paths));
		assert(l);
		GList *i = g_list_nth(l, selected_item);
		assert(i);
		
		GtkWidget *label = GTK_WIDGET(i->data);
		const char *label_text = gtk_label_get_label(GTK_LABEL(label));

		printf("on_open_window_keypress_event(): selected item: %s\n", label_text);

		struct stat file_info;
		if (lstat(label_text, &file_info) == -1) {
			printf("on_open_window_keypress_event(): lstat() error!\n");
			return TRUE;
		}
		
		if (S_ISREG(file_info.st_mode)) {
			char *copy = malloc(strlen(label_text) + 1);
			strcpy(copy, label_text);
			create_tab(copy);
			gtk_widget_destroy(open_window);
			file_paths = NULL;
		} else if (S_ISDIR(file_info.st_mode)) {
			set_root_dir(label_text);
			gtk_widget_destroy(open_window);
			file_paths = NULL;
		} else {
			printf("on_open_window_keypress_event(): %s is unknown.\n", label_text);
		}

		return TRUE;

	} else if (keycode == UP) {
		printf("up-key pressed..\n");
	} else if (keycode == DOWN) {
		printf("down-key pressed..\n");

		if (multiple_matches) {
			printf("we have multiple matches..\n");

			GList *l = gtk_container_get_children(GTK_CONTAINER(file_paths));
			assert(l);
			GList *i = g_list_nth(l, selected_item);
			assert(i);
			
			GtkWidget *label = GTK_WIDGET(i->data);
			remove_class(label, "openfile-dialog-label-selected");
			add_class(label, "openfile-dialog-label");

			GList *next_i = i->next;
			//assert(next_i);
			if (next_i) {
				GtkWidget *next_label = GTK_WIDGET(next_i->data);
				remove_class(next_label, "openfile-dialog-label");
				add_class(next_label, "openfile-dialog-label-selected");
				selected_item += 1;
			} else {
				GtkWidget *next_label = GTK_WIDGET(l->data);
				remove_class(next_label, "openfile-dialog-label");
				add_class(next_label, "openfile-dialog-label-selected");
				selected_item = 0;
			}

		} else {
			printf("we do not have multiple matches..\n");
		}

		return TRUE;
	}

	return FALSE;
}


gboolean toggle_openfile(GdkEventKey *key_event)
{
	printf("Hello world!\n");

	open_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_modal(GTK_WINDOW(open_window), TRUE);
	gtk_window_set_decorated(GTK_WINDOW(open_window), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(open_window),
		openfile_dialog_width, openfile_dialog_height);

	add_class(open_window, "openfile-dialog");

	/*
	On a keypress event only active window's handler will be called,
	and by making our window modal, we make sure that as long as our window exists,
	its our handler which will be called (never the main window's handler).
	*/
	g_signal_connect(open_window, "key-press-event",
		G_CALLBACK(on_open_window_keypress_event), NULL);

	container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	
	openfile_entry = gtk_entry_new();
	add_class(openfile_entry, "openfile-entry");
	g_signal_connect(openfile_entry, "changed",
		G_CALLBACK(on_openfile_entry_changed), NULL);


	/*
	gtk_scrolled_window_set_max_content_height() is available since 3.22
	but this seems to be exactly what we want:
	we want the list of filenames to grow to a certain length,
	but from then on we want the scrollbars.
	*/

	scrollbars = gtk_scrolled_window_new(NULL, NULL);
	//gtk_widget_set_size_request(scrollbars, 500, 500);
	gtk_widget_set_vexpand(scrollbars, TRUE);

	gtk_container_add(GTK_CONTAINER(open_window), container);
	gtk_container_add(GTK_CONTAINER(container), openfile_entry);
	gtk_container_add(GTK_CONTAINER(container), scrollbars);
	
	//gtk_widget_set_vexpand(scroll, TRUE);

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