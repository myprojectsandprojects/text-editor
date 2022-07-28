
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "declarations.h"
//#include "tab.h"

GtkWidget 	*search_phrase_entry;
GtkWidget 	*filename_filter_entry;
GtkWidget 	*hidden_files_check_button;
GtkWidget 	*search_results;

GtkWidget 	*search_button;
gulong 		search_handler_id;
gulong 		stop_handler_id;

// this variable is kinda shared between 2 threads
// maybe we should look into thread synchronization?
// pthread_mutex_lock() ?
gboolean stop_search = FALSE;

extern char 	root_dir[100];

extern const char *search_icon_path;

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

void on_list_row_selected(GtkListBox *list, GtkListBoxRow *row, gpointer data)
{
	printf("on_list_row_selected() called..\n");

	/* It seems that when we delete the list-boxes rows (meaning: search results) in on_search_button_clicked() and a list row is currently selected then this handler will run. In that case we dont want to do anything though. */
	if (row == NULL) {
		printf("on_list_row_selected(): nothing selected..\n");
		return;
	}

	GtkWidget *search_result = gtk_bin_get_child(GTK_BIN(row));
	char *file_path = (char *) g_object_get_data(G_OBJECT(search_result), "file-path");
	unsigned long line_num = (unsigned long) g_object_get_data(G_OBJECT(search_result), "line-num"); // void * -> 64 bits, unsigned long -> 64 bits
	GtkWidget *tab = create_tab(file_path);

	/* Well wait for 1 secs, then scroll to the line that contains the search-phrase. We cant do it here immediately. No clue why. @ Need to look for a better way. */
	void **args = (void **) malloc(2 * sizeof(void *));
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
	//gtk_widget_set_halign(container, GTK_ALIGN_CENTER);
	gtk_widget_set_size_request(widget, width, -1);
	return container;
}
/*
gboolean display_search_result(gpointer data) {
	printf("display_search_result()\n");
	GtkWidget *search_result = (GtkWidget *) data;
	gtk_list_box_insert(GTK_LIST_BOX(search_results), search_result, -1);
	gtk_widget_show_all(search_result);
	return FALSE; // dont call again
}*/
/*
gboolean parse_and_display_search_result(gpointer data)
{
	char *line = (char *) data;

	printf("parse_and_display_search_result()\n");

	printf("parse_and_display_search_result(): line: %s\n", line);

	char *file_path = get_slice_by(&line, ':');
	char *line_number = get_slice_by(&line, ':');
	//printf("file path: %s\n", file_path);
	//printf("line number: %s\n", line_number);
	//printf("remainder: %s\n\n", line);
	
	GtkWidget *search_result = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	
	char *file_path_copy = malloc(strlen(file_path) + 1);
	strcpy(file_path_copy, file_path);
	g_object_set_data(G_OBJECT(search_result), "file-path", file_path_copy); //@ free?

	unsigned long line_num = atoi(line_number); // void * -> 64 bits, unsigned long -> 64 bits
	g_object_set_data(G_OBJECT(search_result), "line-num", (void *) line_num);
	
	printf("BEFORE CREATING LABELS\n");
	int root_length = strlen(root_dir);
	GtkWidget *file_path_label = gtk_label_new(&file_path[root_length + 1]);
	char line_text[100];
	char *p = line;
	assert(p != NULL);
	while (*p == ' ' || *p == '\t') ++p;
	//sprintf(line_text, "%s:%s", line_number, p);
	snprintf(line_text, 100, "%s:%s", line_number, p); // C++11
	//GtkWidget *line_number_label = gtk_label_new(line_number);
	GtkWidget *line_text_label = gtk_label_new(line_text);
	printf("AFTER CREATING LABELS\n");

	free(data);

	gtk_widget_set_halign(file_path_label, GTK_ALIGN_START);
	//gtk_widget_set_halign(line_number_label, GTK_ALIGN_START);
	gtk_widget_set_halign(line_text_label, GTK_ALIGN_START);
	
	gtk_container_add(GTK_CONTAINER(search_result), file_path_label);
	//gtk_container_add(GTK_CONTAINER(search_result), line_number_label);
	gtk_container_add(GTK_CONTAINER(search_result), line_text_label);
	
	gtk_style_context_add_class (gtk_widget_get_style_context(file_path_label), "search-result");

	g_timeout_add_seconds(1, display_search_result, (gpointer) search_result);
	return FALSE;
}*/
/*
gboolean display_search_results(gpointer data)
{
	printf("display_search_results()\n");

	char *contents = (char *) data;

	char *line;
	while ((line = get_slice_by(&contents, '\n')) != NULL) {
		printf("line: %s\n", line);
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
		//sprintf(line_text, "%s:%s", line_number, p);
		snprintf(line_text, 100, "%s:%s", line_number, p); // C++11
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

	return FALSE;
}*/

gboolean display_search_results(gpointer data)
{
	printf("display_search_results()\n");

	// we also have a global variable called search results..
	GtkWidget **list = (GtkWidget **) data;

	int i;
	for (i = 0; list[i] != NULL; ++i) {
		gtk_list_box_insert(GTK_LIST_BOX(search_results), list[i], -1);
	}

	gtk_widget_show_all(search_results);

	g_signal_handler_unblock(search_button, search_handler_id);
	g_signal_handler_block(search_button, stop_handler_id);
	gtk_button_set_label(GTK_BUTTON(search_button), "Search");

	free(list);

	return FALSE; // dont call again
}

GtkWidget **create_search_result_widgets(char *contents)
{
	printf("create_search_result_widgets()\n");

	#define MAX_RESULTS 4299 // maximum number of results to display
	//printf("sizeof(GtkWidget *): %lu\n", sizeof(GtkWidget *)); // 8
	#define N_BYTES (MAX_RESULTS * sizeof(GtkWidget *))
	//printf("N_BYTES: %lu\n", N_BYTES);
	// we also have a global variable called search results..
	GtkWidget **search_results;
	search_results = (GtkWidget **) malloc(N_BYTES + sizeof(GtkWidget *)); //@ dynamic array?

	char *line;
	int i = 0;
	while ((line = get_slice_by(&contents, '\n')) != NULL) {
		
		if (!(i < MAX_RESULTS)) break;

		#define MAX_LINE 100
		char sample[MAX_LINE];
		snprintf(sample, MAX_LINE, "%s", line);
		//printf("create_search_result_widgets(): line: %s\n", line);

		char *file_path = get_slice_by(&line, ':');
		char *line_number = get_slice_by(&line, ':');
		//printf("file path: %s\n", file_path);
		//printf("line number: %s\n", line_number);
		//printf("remainder: %s\n\n", line);

		/*assert(file_path);
		assert(line_number);
		assert(line);*/

		if (!file_path || !line_number || !line) {
			printf("create_search_result_widgets(): Unrecognized line format: %s\n", sample);
			continue;
		}

		GtkWidget *search_result = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

		char *file_path_copy = (char *) malloc(strlen(file_path) + 1);
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
		//sprintf(line_text, "%s:%s", line_number, p);
		snprintf(line_text, 100, "%s:%s", line_number, p); // C++11
		//GtkWidget *line_number_label = gtk_label_new(line_number);
		GtkWidget *line_text_label = gtk_label_new(line_text);

		gtk_widget_set_halign(file_path_label, GTK_ALIGN_START);
		//gtk_widget_set_halign(line_number_label, GTK_ALIGN_START);
		gtk_widget_set_halign(line_text_label, GTK_ALIGN_START);

		gtk_container_add(GTK_CONTAINER(search_result), file_path_label);
		//gtk_container_add(GTK_CONTAINER(search_result), line_number_label);
		gtk_container_add(GTK_CONTAINER(search_result), line_text_label);

		//gtk_style_context_add_class (gtk_widget_get_style_context(file_path_label), "search-result");
		add_class(file_path_label, "search-result");

		search_results[i] = search_result;
		i += 1;
	}

	search_results[i] = NULL;

	return search_results;
}

void *run_command_2(void* command)
{
	int ret;
	GtkWidget **search_results; // we also have a global variable called search results..
	long int size;
	char *output;

	printf("run_command_2()\n");

	FILE *fd = popen((char *) command, "r");
	if (fd == NULL) {
		printf("run_command(): error opening pipe!\n");

		goto wrap_up;
	}

	size = 10000000;
	output = (char *) malloc(size+1); //@ dynamic buffer

	long int i;
	for (i = 0; ((output[i] = fgetc(fd)) != EOF); ++i) {
		if (stop_search == TRUE) {
			stop_search = FALSE;
			free(output);
			goto close_pipe_and_wrap_up;
		}
	}
	output[i] = 0;

	//printf("run_command(): command output: %s\n", output);

	//g_timeout_add_seconds(1, display_search_results, (gpointer) output);
	//display_search_results((gpointer) output);

	search_results = create_search_result_widgets(output);

	// display_search_results() is the part that might take away the UI
	// searching itself operates happily on the background
	//@ while being a monstrous memory hog somehow (?)
	// memory usage doesnt seem to have an effect on performance though
	g_timeout_add_seconds(1, display_search_results, search_results);

	printf("run_command(): done searching, wrapping up\n");

close_pipe_and_wrap_up:
	printf("run_command(): close_pipe_and_wrap_up\n");	
	ret = pclose(fd);
	printf("pclose() returned %d\n", ret);

wrap_up:
	/*g_signal_handler_unblock(search_button, search_handler_id);
	g_signal_handler_block(search_button, stop_handler_id);
	gtk_button_set_label(GTK_BUTTON(search_button), "Search");*/
	
	return (void *) 0;
}
/*
void *run_command(void* command)
{
		FILE *fd = popen((char *) command, "r");
		if (fd == NULL) {
			printf("run_command(): error opening pipe!\n");

			g_signal_handler_unblock(search_button, search_handler_id);
			g_signal_handler_block(search_button, stop_handler_id);
			gtk_button_set_label(GTK_BUTTON(search_button), "Search");

			return (void *) 0;
		}

		char *r;
		while (1) {
			if (stop_search) {
				stop_search = FALSE;
				break;
			}

			char *line = malloc(600);
			r = fgets(line, 600, fd);
			if (r == NULL) break;
			//printf("LENGTH: %d\n", strlen(line));

			// sanity-check the line
			int i, count = 0;
			for (i = 0; line[i] != 0; ++i) {
				if (line[i] == ':') count += 1;
				if (count >= 2) break;
			}
			if (count < 2) {
				printf("run_command(): discarding line: %s\n", line);
				continue; // discard the line
			}

			parse_and_display_search_result((gpointer) line);
			//g_timeout_add_seconds(1, display_search_result, (gpointer) line);
		}

wrap_up:
		printf("run_command(): done searching, wrapping up\n");

		g_signal_handler_unblock(search_button, search_handler_id);
		g_signal_handler_block(search_button, stop_handler_id);
		gtk_button_set_label(GTK_BUTTON(search_button), "Search");

		int ret = pclose(fd);
		printf("pclose() returned %d\n", ret);

		return (void *) 0;
}*/

void search_handler(GtkButton *button, gpointer data)
{
	printf("search_handler()\n");

	// These strings "must not be freed, modified or stored":
	const char *search_phrase		= gtk_entry_get_text(GTK_ENTRY(search_phrase_entry));
	const char *patterns_given 	= gtk_entry_get_text(GTK_ENTRY(filename_filter_entry));

	gboolean ignore_hidden = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hidden_files_check_button));


	printf("search_handler(): search phrase: %s\n", search_phrase);
	printf("search_handler(): file name patterns: %s\n", patterns_given);
	printf("search_handler(): ignore hidden files: %d\n", ignore_hidden);

	printf("search_handler(): about to delete previous search results..\n");
	// deleting previous results can be extremely slow
	GList *previous_results, *p;
	previous_results = gtk_container_get_children(GTK_CONTAINER(search_results));
	for (p = previous_results; p != NULL; p = p->next) {
		//printf("previous search result...\n");
		gtk_widget_destroy(GTK_WIDGET(p->data));
	}
	g_list_free(previous_results); //@ are we freeing everything?
	printf("search_handler(): done deleting previous search results..\n");

	if (search_phrase[0] == 0) {
		printf("search_handler(): no search phrase -> do nothing\n");
		return;
	}

	/*
	find -name "*.c" -and -name "*search*"
	find -name "*.c" -or -name "*search*"
	find -name "*.c" -or -not -name "*search*"
	find -name "*.c" -and -not -name "*search*"
	*/

	char patterns[100];
	patterns[0] = 0;

	char **words = slice_by(patterns_given, ' ');

	int i = 0;
	for (; words[i] != NULL; ++i) {
		if (strcmp(words[i], "||") == 0) {
			sprintf(patterns, "%s -or", patterns);
		} else if (strcmp(words[i], "&&") == 0) {
			sprintf(patterns, "%s -and", patterns);
		} else {
			if (words[i][0] == '!') {
				sprintf(patterns, "%s -not -name \"%s\"", patterns, &words[i][1]);
			} else {
				sprintf(patterns, "%s -name \"%s\"", patterns, words[i]);
			}
		}
		free(words[i]);
	}
	free(words);

	/*
	grep:
		-I -> ignore binary files (?)
		-n -> display line numbers
		-i -> do insensitive search
		-H -> display filename even if only 1 argument given
		-F -> do not interpret the search string as a regular expression
	*/

//	char command[1000];
	char *command = (char *) malloc(1000);
	command[0] = 0;

	sprintf(command,
		"find %s -type f%s%s | xargs -d '\n' grep -InHF \"%s\"",
		root_dir, (ignore_hidden) ? " -not -wholename \"*/.*\"": "", patterns, search_phrase);
	printf("search_handler(): command to execute: %s\n", command);

	pthread_t id;
	pthread_create(&id, NULL, run_command_2, command);

	g_signal_handler_unblock(search_button, stop_handler_id);
	g_signal_handler_block(search_button, search_handler_id);
	gtk_button_set_label(GTK_BUTTON(search_button), "Stop");
}

void stop_handler(GtkButton *button, gpointer data)
{
	printf("stop_handler()\n");
	g_signal_handler_unblock(search_button, search_handler_id);
	g_signal_handler_block(search_button, stop_handler_id);
	gtk_button_set_label(GTK_BUTTON(search_button), "Search");
	stop_search = TRUE;
}

/*
void on_search_button_clicked_test(GtkButton *button, gpointer data) {
	printf("Search button clicked!\n");
	
	void *sleep_4_16_secs(void* data) {
	printf("going to sleep for 16 secs..\n");
	sleep(16);
		printf("done sleeping\n");
		gtk_button_set_label(GTK_BUTTON(search_button), "Search");
	}
	
	pthread_t id;
	pthread_create(&id, NULL, sleep_4_16_secs, NULL);
	
	gtk_button_set_label(GTK_BUTTON(search_button), "Stop");
}
*/
GtkWidget *create_search_in_files_widget()
{
	LOG_MSG("create_search_in_files_widget() called!\n");

	search_phrase_entry = gtk_search_entry_new();
	filename_filter_entry = gtk_entry_new();
	hidden_files_check_button = gtk_check_button_new_with_label("ignore hidden");

	search_button = gtk_button_new_with_label("Search");
/*
	search_button = gtk_button_new();
	GtkWidget *search_image = gtk_image_new_from_file(search_icon_path);
	gtk_button_set_image(GTK_BUTTON(search_button), search_image);
*/

	add_class(search_phrase_entry, "search-in-files-phrase-entry");
	add_class(filename_filter_entry, "search-in-files-pattern-entry");
	add_class(search_button, "search-in-files-button");
	//add_class(hidden_files_check_button, "search-in-files-check-button");


	//gtk_entry_set_text(GTK_ENTRY(filename_filter_entry), "*");
	
	//g_signal_connect(search_button, "clicked", G_CALLBACK(on_search_button_clicked), NULL);
	//g_signal_connect(search_button, "clicked", G_CALLBACK(on_search_button_clicked_test), NULL);
	search_handler_id = g_signal_connect(search_button, "clicked", G_CALLBACK(search_handler), NULL);
	stop_handler_id = g_signal_connect(search_button, "clicked", G_CALLBACK(stop_handler), NULL);
	g_signal_handler_block(search_button, stop_handler_id);

	search_results = gtk_list_box_new();
	gtk_widget_set_vexpand(search_results, TRUE);
	//g_signal_connect(list, "row-selected", G_CALLBACK(on_list_row_selected), NULL);
	g_signal_connect(search_results, "row-activated", G_CALLBACK(on_list_row_selected), NULL);

	add_class(search_results, "search-results");


	/*@ I have no idea what is going on here: */

	GtkWidget *container_v = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	GtkWidget *space1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(space1, -1, 5);
	gtk_container_add(GTK_CONTAINER(container_v), space1);
	gtk_container_add(GTK_CONTAINER(container_v), widget_with_width(search_phrase_entry, 400));
	gtk_container_add(GTK_CONTAINER(container_v), widget_with_width(filename_filter_entry, 400));
	gtk_container_add(GTK_CONTAINER(container_v), hidden_files_check_button);
	GtkWidget *space2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(space2, -1, 10);
	gtk_container_add(GTK_CONTAINER(container_v), space2);
	gtk_container_add(GTK_CONTAINER(container_v), widget_with_width(search_button, 125));
	GtkWidget *space3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(space3, -1, 5);
	gtk_container_add(GTK_CONTAINER(container_v), space3);

	GtkWidget *container_h = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_container_add(GTK_CONTAINER(container_h), container_v);
	gtk_widget_set_halign(container_h, GTK_ALIGN_CENTER);

	GtkWidget *background = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(background), container_h);
	add_class(background, "search-in-files");

	GtkWidget *container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_widget_set_hexpand(container, TRUE);
	//gtk_container_add(GTK_CONTAINER(container), container_h);
	gtk_container_add(GTK_CONTAINER(container), background);

	GtkWidget *scrollbars = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrollbars), search_results);
	gtk_container_add(GTK_CONTAINER(container), scrollbars);

	return container;
}