#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "declarations.h"

/*
We need to run "sudo updatedb" to keep "locate" up-to-date
*/


extern char root_dir[100];
extern GtkWidget *app_window;

GtkWidget *open_window;
GtkWidget *container;
GtkWidget *scrollbars;
GtkWidget *file_list;

int num_items;
int index_selected_item;

const int openfile_dialog_width = 800;
const int openfile_dialog_height = 500;

/*
The idea is that a file-list has a maximum height (in pixels).
If the file-list grows any longer, then we wrap it inside the scrollbars and it would be nice,
if the height of the scrollbars (viewport) is exactly equal to the maximum height of the list.
Its not a big deal, if the height of the scrollbars is a little less than the maximum height of the list, but if the height of the scrollbars is more, we might get a list which is visibly less in height than scrollbars and it doesnt look great.
*/
const int scrolled_window_width = 800; //openfile_dialog_width
const int scrolled_window_height = 500;


void select_filelist_item_at(int index)
{
	assert(file_list);
	GtkListBoxRow *row =
		gtk_list_box_get_row_at_index(GTK_LIST_BOX(file_list), index);
	assert(row);
	gtk_list_box_select_row(GTK_LIST_BOX(file_list), row);
}


void on_openfile_entry_changed
	(GtkEntry *search_entry, gpointer data)
{
	printf("on_openfile_entry_changed()\n");

	if (scrollbars) {
		gtk_widget_destroy(scrollbars);
		scrollbars = NULL;
		file_list = NULL;
		printf("destroying list with scrollbars\n");
	} else if (file_list) {
		gtk_widget_destroy(file_list);
		file_list = NULL;
		printf("destroying list\n");
	} else {
		printf("no widgets to destroy\n");
	}

	num_items = 0;
	index_selected_item = -1;

/*
	GList *l = gtk_container_get_children(GTK_CONTAINER(file_list));
	for (GList *p = l; p != NULL; p = p->next) {
		//GtkLabel* label = (GtkLabel *) gtk_bin_get_child(p->data);
		//gchar *text = gtk_label_get_label(label);
		//puts(text);
		gtk_widget_destroy(GTK_WIDGET(p->data));
	}
*/

	const char *text = gtk_entry_get_text(search_entry);
/*
	if (strlen(text) < 3) {
		gtk_window_resize(GTK_WINDOW(open_window), openfile_dialog_width, 1);
		//gtk_widget_show_all(open_window);
		return;
	}
*/
	if (strlen(text) < 1) {
		gtk_window_resize(GTK_WINDOW(open_window), openfile_dialog_width, 1);
		//gtk_widget_show_all(open_window);
		return;
	}

	struct timeval start, end;
	gettimeofday(&start, NULL);

	char command[1000];
	//snprintf(command, 100, "locate -b %s | grep ^/home/eero/all/text-editor", text);
	//snprintf(command, 100, "locate -b %s | grep ^/home/eero/all", text);
	//snprintf(command, 100, "locate -b %s | grep ^%s/", text, root_dir);
/*
	snprintf(command, 100,
		"grep \"^%s\" /home/eero/all/find-utils/install/bin/textdb | grep \"/%s[^/]*$\"",
		root_dir, text);
*/

	char *escaped_text = str_replace(text, ".", "\\.");
	char *home_dir = getenv("HOME");
	char dbfile_path[100];
	snprintf(dbfile_path, 100, "%s/textdb", home_dir);
	printf("***database file path: %s\n", dbfile_path);
	snprintf(command, 1000,
		"grep \"^%s\" %s | grep \"/[^/]*%s[^/]*$\"",
		root_dir, dbfile_path, escaped_text);
	printf("***command to execute: %s\n", command);

	//#define MAX_RESULTS 10000
	#define MAX_RESULTS 1000000
	char results[MAX_RESULTS];

	FILE* s = NULL;
	if (!(s = popen(command, "r"))) {
		//printf("on_openfile_entry_changed(): error!\n");
		assert(FALSE);
	}

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

	gettimeofday(&end, NULL);
	// I think we are talking about micro- here, not milli-?
	printf("It took %ld milliseconds\n",
		1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec));


	char *p = results;
	char *line;

	int lines = 0;

	while ((line = get_slice_by(&p, '\n')) != NULL) {
		/* @should we get rid of this "if" here for performance reasons? */

		if (!file_list) {
			file_list = gtk_list_box_new();
			add_class(file_list, "openfile-file-list");
		}
		GtkWidget *l = gtk_label_new(line);
		gtk_list_box_insert(GTK_LIST_BOX(file_list), l, -1);
		gtk_widget_set_halign(l, GTK_ALIGN_START);

		num_items += 1;
	
		lines += 1;
	}

	// Select first item
	if (num_items > 0) {
		select_filelist_item_at(0);
		index_selected_item = 0;

		if (num_items > 22) {
			printf("adding scrollbars to the list..\n");
			scrollbars = gtk_scrolled_window_new(NULL, NULL);
			gtk_widget_set_size_request(scrollbars,
				scrolled_window_width, scrolled_window_height);
			gtk_container_add(GTK_CONTAINER(scrollbars), file_list);
			gtk_container_add(GTK_CONTAINER(container), scrollbars);
		} else {
			gtk_container_add(GTK_CONTAINER(container), file_list);
		}
	}

	printf("proccessed %d lines\n", lines);
	if (c == 0)
		printf("proccessed all\n");
	else
		printf("didnt proccess all\n");

	//gtk_widget_set_size_request(open_window, openfile_dialog_width, 1);
	gtk_window_resize(GTK_WINDOW(open_window), openfile_dialog_width, 1);
	gtk_widget_show_all(open_window);

	return;

	// this is always 1 and 1. widget not realized yet?
	GtkAllocation alloc;
	gtk_widget_get_allocation(file_list, &alloc);
	printf("list width: %d, list height: %d\n", alloc.width, alloc.height);
}

gboolean on_open_window_keypress_event(GtkWidget *open_window,
	GdkEvent *event, gpointer user_data)
{
	#define ESCAPE 9
	#define UP 111
	#define DOWN 116
	#define ENTER 36

	GdkEventKey *key_event = (GdkEventKey *) event;
	guint16 keycode = key_event->hardware_keycode;

	//LOG_MSG("on_open_window_keypress_event(): keycode: %d\n", keycode);

	if (keycode == ESCAPE) {
		printf("on_open_window_keypress_event(): escape-key pressed..\n");
		gtk_widget_destroy(open_window);
		scrollbars = NULL;
		file_list = NULL;
	} else if (keycode == ENTER) {
		printf("on_open_window_keypress_event(): enter-key pressed..\n");

		//		char filepath[100];
		//
		//		const char *rel_filepath = gtk_entry_get_text(GTK_ENTRY(openfile_entry));
		//		snprintf(filepath, 100, "%s/%s", root_dir, rel_filepath);
		//		//@ what if its not a file or doesnt exist...?		
		//		create_tab(filepath);

		if (num_items == 0) return TRUE;

		GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(file_list));
		GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
		const char *label_text = gtk_label_get_label(GTK_LABEL(label));

		printf("\t-> selected item: %s\n", label_text);

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
			scrollbars = NULL;
			file_list = NULL;
		} else if (S_ISDIR(file_info.st_mode)) {
			set_root_dir(label_text);
			gtk_widget_destroy(open_window);
			scrollbars = NULL;
			file_list = NULL;
		} else {
			printf("on_open_window_keypress_event(): %s is unknown.\n", label_text);
		}

		return TRUE;
	}
	/* We may want to overwrite the default handlers provided by the ListBox widget for up and down keys... */
	else if (keycode == UP || keycode == DOWN) {
		//printf("on_open_window_keypress_event(): up-key pressed..\n");

		if (num_items == 0) return TRUE;

		if (keycode == UP) {
			index_selected_item -= 1;
		} else {
			index_selected_item += 1;
		}

		if (index_selected_item < 0) {
			index_selected_item = num_items - 1;
		} else if (index_selected_item > num_items - 1) {
			index_selected_item = 0;
		}

		select_filelist_item_at(index_selected_item);
		//printf("on_open_window_keypress_event(): index: %d\n", index_selected_item);

		if (!scrollbars) {
			printf("on_open_window_keypress_event(): DOWN: no scrollbars\n");
			return TRUE;
		}

		GtkAdjustment *a = gtk_list_box_get_adjustment(GTK_LIST_BOX(file_list));
		gdouble value = gtk_adjustment_get_value(a);
		gdouble upper = gtk_adjustment_get_upper(a);
		gdouble lower = gtk_adjustment_get_lower(a);
		gdouble page_size = gtk_adjustment_get_page_size(a);
		printf("value: %f, upper: %f, lower: %f, page-size: %f\n",
			value, upper, lower, page_size);

		//gtk_adjustment_set_value(a, upper - page_size); // scroll to the very end?
		//gtk_adjustment_set_value(a, value + 10); // scroll down by 10 pixels?
		//gtk_list_box_set_adjustment(GTK_LIST_BOX(file_list), a);

		GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(file_list), index_selected_item);
		GtkAllocation alloc;
		gtk_widget_get_allocation(GTK_WIDGET(row), &alloc);
		printf("width: %d, height: %d, x: %d, y: %d\n",
			alloc.width, alloc.height, alloc.x, alloc.y);

		if (alloc.y >= value && alloc.y + alloc.height <= value + page_size) {
			printf("selected item is visible..\n");
		} else {
			printf("selected item is NOT visible..\n");

			if (alloc.y >= value) {
				// scrolling downwards
				gtk_adjustment_set_value(a, alloc.y - (page_size - alloc.height));
			} else {
				// scrolling upwards
				gtk_adjustment_set_value(a, alloc.y);
			}
			gtk_list_box_set_adjustment(GTK_LIST_BOX(file_list), a);
		}

		return TRUE; // dont call the default handler

	}

	return FALSE;
}


gboolean display_openfile_dialog(GdkEventKey *key_event)
{
	printf("display_openfile_dialog()\n");

	open_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_modal(GTK_WINDOW(open_window), TRUE);
	gtk_window_set_decorated(GTK_WINDOW(open_window), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(open_window),
		openfile_dialog_width, 1);
	add_class(open_window, "openfile-dialog");

	/* Position relative to app window: */
	gint x, y;
	GdkWindow *gdk_app_window = gtk_widget_get_window(app_window);
	gdk_window_get_origin(gdk_app_window, &x, &y);
	//printf("\t-> x: %d, y: %d\n", x, y);
	gtk_window_move(GTK_WINDOW(open_window), x + 100, y + 100);

	/*
	On a keypress event only active window's handler will be called,
	and by making our window modal, we make sure that as long as our window exists,
	its our handler which will be called (never the main window's handler).
	*/
	g_signal_connect(open_window, "key-press-event",
		G_CALLBACK(on_open_window_keypress_event), NULL);

	
	GtkWidget *openfile_entry = gtk_entry_new();
	add_class(openfile_entry, "openfile-entry");

	g_signal_connect(openfile_entry, "changed",
		G_CALLBACK(on_openfile_entry_changed), NULL);


	//file_list = gtk_list_box_new();
	//add_class(file_list, "openfile-file-list");


	/*
	gtk_scrolled_window_set_max_content_height() is available since 3.22
	but this seems to be exactly what we want:
	we want the list of filenames to grow to a certain length,
	but from then on we want the scrollbars.
	*/

	//GtkWidget *scrollbars = gtk_scrolled_window_new(NULL, NULL);
	//gtk_widget_set_vexpand(scrollbars, TRUE);
	//gtk_widget_set_size_request(scrollbars, 0, 0);
/*
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollbars),
		GTK_POLICY_NEVER, GTK_POLICY_NEVER);
*/
	container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	gtk_container_add(GTK_CONTAINER(open_window), container);
	gtk_container_add(GTK_CONTAINER(container), openfile_entry);
	//gtk_container_add(GTK_CONTAINER(container), scrollbars);

	gtk_widget_show_all(open_window);

	return TRUE;
}