
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "declarations.h"
#include "tab.h"

GtkWidget *search_results;
extern char root_dir[100];

/* callback for g_timeout_add_seconds */
gboolean tab_scroll_to(gpointer data)
{
	printf("tab_scroll_to() called!\n");

	void **args = (void **) data;
	GtkWidget *tab = (GtkWidget *) args[0];
	unsigned long line_num = (unsigned long) args[1]; // void * -> 64 bits, unsigned long -> 64 bits

	GtkTextView *text_view = (GtkTextView *) tab_retrieve_widget(tab, TEXT_VIEW);
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);
	assert(text_view != NULL && GTK_IS_TEXT_VIEW(text_view));
	assert(text_buffer != NULL && GTK_IS_TEXT_BUFFER(text_buffer));
	printf("should scroll to line: %lu\n", line_num);
	
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_line(text_buffer, &iter, line_num - 1);
	gtk_widget_grab_focus(GTK_WIDGET(text_view));
	gtk_text_buffer_place_cursor(text_buffer, &iter);
	gtk_text_view_scroll_to_iter(text_view, &iter, 0.0, FALSE, 0.0, 0.0);

	return FALSE; // dont call again!
	//return TRUE; // call again!
}

void on_search_button_clicked(GtkButton *search_button, gpointer data)
{
	printf("on_search_button_clicked() called..\n");

	GtkSearchEntry *search_entry = (GtkSearchEntry *) data;
	const char *text = gtk_entry_get_text(GTK_ENTRY(search_entry));

	/*
	grep:
		-I -> ignore binary files (?)
		-n -> display line numbers
		-i -> do insensitive search
	*/
	char command[1000]; command[0] = 0;
	sprintf(command, "find %s ! -regex '.*/\\..*' -type f | xargs grep -Ini \"%s\"", root_dir, text);
	printf("command: %s\n", command);
	FILE *fd = popen(command, "r");
	if (fd == NULL) {
		printf("ERROR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		return;
	}

	/*printf("sleeping for 3 secs\n");
	sleep(3);
	printf("done sleeping\n");*/

	long int size = 10000000;
	char *contents = malloc(size+1);
	if (contents == NULL) {
		printf("malloc() failed!!!!!!!!!!!!!!!!!\n");
	}

	long int i = 0;
	while((contents[i] = fgetc(fd)) != -1) {/*printf("%c ", contents[i]);*/ i++;}
	contents[i] = 0;

	printf("Search results:\n");
	printf("%s\n", contents);

	GList *previous_results, *p;
	previous_results = gtk_container_get_children(GTK_CONTAINER(search_results));
	for (p = previous_results; p != NULL; p = p->next) {
		//printf("previous search result...\n");
		gtk_widget_destroy(p->data);
	}
	g_list_free(previous_results); //@ are we freeing everything?

	char *line;
	while ((line = get_slice_by(&contents, '\n')) != NULL) {
		//printf("line: %s\n", line);
		char *file_path = get_slice_by(&line, ':');
		char *line_number = get_slice_by(&line, ':');
		printf("file path: %s\n", file_path);
		printf("line number: %s\n", line_number);
		printf("remainder: %s\n\n", line);

		GtkWidget *search_result = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

		char *file_path_copy = malloc(strlen(file_path) + 1);
		strcpy(file_path_copy, file_path);
		g_object_set_data(G_OBJECT(search_result), "file-path", file_path_copy); //@ free?
		unsigned long line_num = atoi(line_number); // void * -> 64 bits, unsigned long -> 64 bits
		g_object_set_data(G_OBJECT(search_result), "line-num", (void *) line_num);

		int root_length = strlen(root_dir);
		GtkWidget *file_path_label = gtk_label_new(&file_path[root_length + 1]);
		char line_text[100];
		char *p = line;
		assert(p != NULL);
		while (*p == ' ' || *p == '\t') ++p;
		sprintf(line_text, "%s:%s", line_number, p);
		//GtkWidget *line_number_label = gtk_label_new(line_number);
		GtkWidget *line_text_label = gtk_label_new(line_text);

		gtk_widget_set_halign(file_path_label, GTK_ALIGN_START);
		//gtk_widget_set_halign(line_number_label, GTK_ALIGN_START);
		gtk_widget_set_halign(line_text_label, GTK_ALIGN_START);

		gtk_container_add(GTK_CONTAINER(search_result), file_path_label);
		//gtk_container_add(GTK_CONTAINER(search_result), line_number_label);
		gtk_container_add(GTK_CONTAINER(search_result), line_text_label);

		gtk_style_context_add_class (gtk_widget_get_style_context(file_path_label), "search-result");

		gtk_list_box_insert(GTK_LIST_BOX(search_results), search_result, -1);
	}

	gtk_widget_show_all(search_results);

	pclose(fd);
}

void on_list_row_selected(GtkListBox *list, GtkListBoxRow *row, gpointer data)
{
	printf("on_list_row_selected() called..\n");

	/* It seems that when we delete the list-boxes rows (meaning: search results) in on_search_button_clicked() and a list row is currently selected then this handler will run. In that case we dont want to do anything though. */
	if (row == NULL) {
		printf("on_list_row_selected(): nothing selected..\n");
		return;
	}

	GtkWidget *search_result = gtk_bin_get_child(GTK_BIN(row));
	char *file_path = g_object_get_data(G_OBJECT(search_result), "file-path");
	unsigned long line_num = (unsigned long) g_object_get_data(G_OBJECT(search_result), "line-num"); // void * -> 64 bits, unsigned long -> 64 bits
	GtkWidget *tab = create_tab(file_path);

	/* Well wait for 1 secs, then scroll to the line that contains the search-phrase. We cant do it here immediately. No clue why. @ Need to look for a better way. */
	void **args = malloc(2 * sizeof(void *));
	args[0] = (void *) tab;
	args[1] = (void *) line_num;

	g_timeout_add_seconds(1, tab_scroll_to, args);
}


/*
If we add a widget to a vertical-container, the widget will automatically expand to the width of the container. It seems that we can avoid this by wrapping the widget inside a horizontal-container. Then we can specify a fixed minimal-size for the widget.
*/
GtkWidget *widget_with_width(GtkWidget *widget, int width)
{
	GtkWidget *container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add(GTK_CONTAINER(container), widget);
	gtk_widget_set_size_request(widget, width, -1);
	return container;
}

GtkWidget *create_search_in_files_widget()
{
	printf("create_search_in_files_widget() called!\n");

	GtkWidget *search_entry = gtk_search_entry_new();
	GtkWidget *search_button = gtk_button_new_with_label("Search in Files");
	
	g_signal_connect(search_button, "clicked", G_CALLBACK(on_search_button_clicked), search_entry);

	search_results = gtk_list_box_new();
	//g_signal_connect(list, "row-selected", G_CALLBACK(on_list_row_selected), NULL);
	g_signal_connect(search_results, "row-activated", G_CALLBACK(on_list_row_selected), NULL);

	GtkWidget *container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	gtk_container_add(GTK_CONTAINER(container), widget_with_width(search_entry, 250));
	gtk_container_add(GTK_CONTAINER(container), widget_with_width(search_button, 250));
	gtk_container_add(GTK_CONTAINER(container), search_results);

	return container;
}
