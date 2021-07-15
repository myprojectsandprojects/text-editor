
//@ bug: large file + small window size (also toggling the sidebar) crashes application with an X Server message if adding text_view_container to scrolled window.
//@ bug: highlighting: double quotes inside block-comments
//@ bug: highlighting: backslash inside a string
//@ bug: highlighting: line comment & preprocessor directive
//@ bug: changing css of search results from another editor may crash the application... (try to time the procedure-call: g_timeout_add_seconds())


// bash commands: locate [pattern], sudo updatedb (fast)

// plausable feature: searching limited to a specific scope somehow? like only inside a function...

// in progress: obviously search-in-files should open & scroll
// plausable feature: search-in-files: ui should be responsive while searching, sort results, group results under filename, search depth, filter by filename
// plausable feature: easy way to find & open files by name... in search-in-files we could filter by file name while leaving the search phrase empty (it makes sense to think about an empty string this way) and the results would be just clickable file names? use command entry somehow?

// plausable feature: sort files & dirs in the file-browser? group by type?
// plausable feature: keeping file browser up-to-date with fs-changes

// plausable feature: can we hide the notebook entirely when maxing the search results window? or at least hide it when the notebook cant be made smaller? 

// plausable feature: we have ctrl + <left/right>, what about ctrl + <up/down>?
// plausable feature: indent when opening a line
// plausable feature: delete end of line
// plausable feature: select a whole line when double clicking on a wrapped line

// plausable feature: tabs, file changes on disk...
// plausable feature: opening & highlighting large files is really slow and ui becomes unresponsive (but it eventually manages to do it)

// in progress: highlighting... 4 different languages and how to support that? no-highlighting option

// plausable feature: conf-file 4 key-combinations

// plausable feature: logging messages which have at least 2 possible levels associated with them, so that we can disable all logging messages except the ones we are currently interested in. otherwise its very hard to spot important messages.

// UI: from some point onwards opening new tabs starts resizing the window.


#include <gtk/gtk.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <limits.h> // for NAME_MAX

#include "declarations.h"
#include "tab.h"

GtkWidget *window;
GtkWidget *notebook;
GtkWidget *sidebar_container;
GtkWidget *file_browser; // GtkTreeView

//GtkWidget *root_selection;
gulong root_selection_changed_id;
int root_selection_index; // file-browser needs access to it // index of the last item in root-selection. we need to know that because each time user selects a new root by double-clicking on a directory well set the last item to that directory.

/*
This is like a root directory of our project (or workspace or)
*/
char root_dir[100]; // file-browser and find-in-files modules need access to it

extern GtkWidget *root_dir_label;

void set_root_dir(const char *path)
{
	assert(path != NULL);
	assert(strlen(path) < 100);
	assert(strlen(path) > 0);

	strcpy(root_dir, path);
	
	gtk_label_set_text(GTK_LABEL(root_dir_label), root_dir);
	gtk_tree_view_set_model(GTK_TREE_VIEW(file_browser), GTK_TREE_MODEL(create_tree_store()));
}

/*void add_highlighting(GtkTextBuffer *text_buffer) {
	g_print("add_highlighting()\n");
	
	GtkTextIter i, j;
	gtk_text_buffer_get_start_iter(text_buffer, &i);
	if(gtk_text_iter_is_end(&i) == TRUE) {
		return;
	}

	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(text_buffer, &start, &end);

	GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);
	gint size = gtk_text_tag_table_get_size(table);
	//g_print("add_highlighting: table size: %d\n", size);
	if(size == 0) {
		gtk_text_buffer_create_tag(text_buffer, "comment", "foreground", "forestgreen", "style", PANGO_STYLE_ITALIC, NULL);
		//gtk_text_buffer_create_tag(text_buffer, "keyword", "weight", PANGO_WEIGHT_BOLD, NULL);
		gtk_text_buffer_create_tag(text_buffer, "keyword", "foreground", "#d6c6ae", NULL);
		gtk_text_buffer_create_tag(text_buffer, "string", "foreground", "gray", NULL);
		gtk_text_buffer_create_tag(text_buffer, "number-literal", "foreground", "lightseagreen", NULL);
	} else {
		gtk_text_buffer_remove_all_tags(text_buffer, &start, &end); // Doesnt remove tags themselves, just the highlighting.
	}

	gboolean doing_something = FALSE;
	gboolean doing_str = FALSE;
	gboolean doing_comment = FALSE;

	do {
		gunichar c = gtk_text_iter_get_char(&i);
		//g_print("character: %c (%d)\n", c, c);
		if(c == '/') {
			gtk_text_iter_forward_char(&i);
			c = gtk_text_iter_get_char(&i);
			if(c == '*') {
				if(doing_something == FALSE) { // We want to begin a comment, but can we?
					doing_comment = TRUE;
					doing_something = TRUE;
					j = i;
					continue;
				}
			}
			if(c == '/') { // -> comment out the rest of the line
				if(doing_something == FALSE) {
					j = i;
					gtk_text_iter_forward_line(&i);
					gtk_text_buffer_apply_tag_by_name(text_buffer, "comment", &j, &i);
					gtk_text_iter_backward_char(&i);
					continue;
				}
			}
			gtk_text_iter_backward_char(&i);
			c = gtk_text_iter_get_char(&i);
		}
		if(c == '*') {
			gtk_text_iter_forward_char(&i);
			c = gtk_text_iter_get_char(&i);
			if(c == '/') {
				if(doing_comment == TRUE) { // We want to end a comment, but is there a comment to end?
					doing_comment = FALSE;
					doing_something = FALSE;
					gtk_text_buffer_apply_tag_by_name(text_buffer, "comment", &j, &i);
					continue;
				}
			}
		}
		if(c == '\"') {
			gtk_text_iter_backward_char(&i);
			if(gtk_text_iter_get_char(&i) == '\\') {
				gtk_text_iter_forward_char(&i);
				continue;
			}
			gtk_text_iter_forward_char(&i);
			if(doing_str == TRUE) {
				doing_str = FALSE;
				doing_something = FALSE;
				gtk_text_buffer_apply_tag_by_name(text_buffer, "string", &j, &i);
				continue;
			} else if (doing_something == FALSE) {
				j = i; doing_str = TRUE; doing_something = TRUE;
			}
		}
		if(g_unichar_isdigit(c) == TRUE) { // Could be a number-literal.
			if(doing_something == FALSE) { // Not inside a comment or a string.
				j = i;
				while(gtk_text_iter_forward_char(&i) == TRUE) {
					c = gtk_text_iter_get_char(&i);
					if(g_unichar_isdigit(c) == FALSE && c != '.' && c != 'x') break;
				}
				gtk_text_buffer_apply_tag_by_name(text_buffer, "number-literal", &j, &i);
			}
		}
		if(g_unichar_isalpha(c) == TRUE || c == '_') { // Could be an identifier.
			if(doing_something == FALSE) { // Not inside a comment or a string.
				char *identifier; //@ free?
				j = i;
				while(gtk_text_iter_forward_char(&i) == TRUE) {
					c = gtk_text_iter_get_char(&i);
					if(g_unichar_isalnum(c) == FALSE && c != '_') break;
				}
				identifier = gtk_text_buffer_get_text(text_buffer, &j, &i, FALSE); //@ ...is kinda costly?
				//g_print("add_highlighting: identifier: %s\n", identifier);

				// Maybe our identifier is a keyword?
				char *keywords[] = {"if", "else", "return", "for", "while", "break", "continue", NULL};
				int k;
				for(k = 0; keywords[k] != NULL; ++k) {
					if(strcmp(identifier, keywords[k]) == 0) {
						gtk_text_buffer_apply_tag_by_name(text_buffer, "keyword", &j, &i);
						break;
					}
				}

				gtk_text_iter_backward_char(&i);
				free(identifier);
			}
		}
	} while(gtk_text_iter_forward_char(&i) == TRUE);
}*/

void tab_set_unsaved_changes_to(GtkWidget *tab, gboolean unsaved_changes)
{
	g_print("tab_set_unsaved_changes_to() called!\n");

	gpointer data = g_object_get_data(G_OBJECT(tab), "tab-info");
	assert(data != NULL);
	struct TabInfo *tab_info = (struct TabInfo *) data;

	tab_info->unsaved_changes = unsaved_changes;

	GtkWidget *title_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	GtkWidget *title_label = gtk_label_new(tab_info->title);
	gtk_container_add(GTK_CONTAINER(title_widget), title_label);

	if (unsaved_changes == TRUE) {
		GtkWidget *title_image = gtk_image_new_from_file("icons/exclamation-mark-small.png");
		gtk_container_add(GTK_CONTAINER(title_widget), title_image);
	}

	gtk_widget_show_all(title_widget);

	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), tab, title_widget);
}

void text_buffer_changed(GtkTextBuffer *text_buffer, gpointer user_data)
{
	printf("buffer changed!\n");

	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	assert(page != -1);
	/*if (page == -1) {
		// buffers contents changed, but we have no "current page"... is this possible?
		return;
	}*/
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);

	struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	if (tab_info->unsaved_changes == FALSE)
		tab_set_unsaved_changes_to(tab, TRUE);
}

/* Could use retrieve_widget functions instead of passing in the pointer as an argument to get the label... */
void text_buffer_cursor_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data)
{
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(object);
	int position, line_number;
	g_object_get(G_OBJECT(text_buffer), "cursor-position", &position, NULL);
	GtkTextIter i;
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(text_buffer), &i, position);
	line_number = gtk_text_iter_get_line(&i);

	char buffer[100];
	sprintf(buffer, "%d", line_number + 1);
	GtkLabel *label = (GtkLabel *) user_data;
	gtk_label_set_text(label, buffer);

	/* Line highlighting -- it messes up code highlighting. */
	/*GtkTextIter start, end, start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
	gtk_text_buffer_remove_tag_by_name(text_buffer, "line-highlight", &start_buffer, &end_buffer);
	gtk_text_iter_set_line_offset(&i, 0);
	start = i;
	gtk_text_iter_forward_line(&i);
	//gtk_text_iter_forward_char(&i);
	//gtk_text_iter_backward_char(&i);
	end = i;
	//printf("applying the tag: %d, %d\n", gtk_text_iter_get_offset(&start), gtk_text_iter_get_offset(&end));
	gtk_text_buffer_apply_tag_by_name(text_buffer, "line-highlight", &start, &end);*/
}

//@ empty string

// Copy/paste/cut pure raw text -- no GTK "tag-magic"
// We would like to remove default handlers entirely, but I dont know how to do it right now.
void text_view_copy_clipboard(GtkTextView *text_view, gpointer user_data)
{
	GtkTextIter start, end;
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);
	gtk_text_buffer_get_selection_bounds(text_buffer, &start, &end); // gboolean
	char *text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);

	GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clip, text, -1);
	
	g_signal_stop_emission_by_name(text_view, "copy-clipboard");
}

void text_view_cut_clipboard(GtkTextView *text_view, gpointer user_data)
{
	GtkTextIter start, end;
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);
	gtk_text_buffer_get_selection_bounds(text_buffer, &start, &end); // gboolean
	char *text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);

	GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clip, text, -1);
	gtk_text_buffer_delete(text_buffer, &start, &end);

	g_signal_stop_emission_by_name(text_view, "cut-clipboard");
}

//@ should replace selected 
void text_view_paste_clipboard(GtkTextView *text_view, gpointer user_data)
{
	g_print("paste-clipboard!!!\n");

	GtkTextIter start, end;
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);
	GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

	gtk_text_buffer_get_selection_bounds(text_buffer, &start, &end); // gboolean
	char *text = gtk_clipboard_wait_for_text(clip); //@ free?
	gtk_text_buffer_delete(text_buffer, &start, &end);
	gtk_text_buffer_insert(text_buffer, &start, text, -1); // call to delete should invalidate the iterators? apparently not.
	
	g_signal_stop_emission_by_name(text_view, "paste-clipboard");
}

void on_button_clicked(GtkButton *button, gpointer data)
{
	void **pointers = (void **) data;
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) pointers[0];
	GtkEntry *entry1 = (GtkEntry *) pointers[1];
	GtkEntry *entry2 = (GtkEntry *) pointers[2];
	const char *text1 = gtk_entry_get_text(entry1);
	const char *text2 = gtk_entry_get_text(entry2);
	printf("Should search for \"%s\" and replace with \"%s\"\n", text1, text2);
	
	GtkTextIter start, end;
	gtk_text_buffer_get_selection_bounds(text_buffer, &start, &end);
	/*char *selected_text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
	printf("...in: \"%s\"\n", selected_text);*/

	//GtkTextMark *selection_start = gtk_text_buffer_create_mark(text_buffer, "selection-start", &start, FALSE);
	GtkTextMark *selection_end = gtk_text_buffer_create_mark(text_buffer, "selection-end", &end, FALSE);
	GtkTextMark *search_start = gtk_text_buffer_create_mark(text_buffer, NULL, &start, FALSE);

	GtkTextIter match_start, match_end;
	while (gtk_text_iter_forward_search(&start, text1, GTK_TEXT_SEARCH_CASE_INSENSITIVE, &match_start, &match_end, &end)) {
		gtk_text_buffer_move_mark(text_buffer, search_start, &match_end);
		gtk_text_buffer_delete(text_buffer, &match_start, &match_end);
		gtk_text_buffer_insert(text_buffer, &match_start, text2, -1);
		//gtk_text_buffer_get_iter_at_mark(text_buffer, &start, selection_start);
		gtk_text_buffer_get_iter_at_mark(text_buffer, &start, search_start);
		gtk_text_buffer_get_iter_at_mark(text_buffer, &end, selection_end);
	}

	gtk_text_buffer_delete_mark(text_buffer, search_start);
	gtk_text_buffer_delete_mark(text_buffer, selection_end);
}

void display_search_and_replace_dialog()
{
	/*GtkTextBuffer *text_buffer;
	if ((text_buffer = get_visible_text_buffer(GTK_NOTEBOOK(notebook))) == NULL) {
		return; // No tabs, nothing to do..
	}*/

	GtkTextBuffer *text_buffer;
	text_buffer = GTK_TEXT_BUFFER(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER));
	if (text_buffer == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return;
	}

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Search & Replace");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	GtkWidget *container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *entry1 = gtk_entry_new();
	GtkWidget *entry2 = gtk_entry_new();
	GtkWidget *button = gtk_button_new_with_label("Search & Replace");
	gtk_container_add(GTK_CONTAINER(window), container);
	gtk_container_add(GTK_CONTAINER(container), entry1);
	gtk_container_add(GTK_CONTAINER(container), entry2);
	gtk_container_add(GTK_CONTAINER(container), button);
	void **pointers = malloc(3 * sizeof(void *));
	pointers[0] = text_buffer;
	pointers[1] = entry1;
	pointers[2] = entry2;
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_button_clicked), pointers);
	gtk_widget_show_all(GTK_WIDGET(window));
}

gboolean search_and_replace(GdkEventKey *key_event)
{
	display_search_and_replace_dialog();
}

void on_menu_item_activate(GtkMenuItem *item, gpointer data)
{
	display_search_and_replace_dialog();
}

void on_text_view_populate_popup(GtkTextView *text_view, GtkWidget *popup, gpointer data)
{
	printf("populate popup!\n");
	if (!GTK_IS_MENU(popup)) {
		return;
	}

	//GtkWidget *sep = gtk_separator_menu_item_new();
	//gtk_menu_attach(GTK_MENU(popup), sep, 0, 1, 6, 7);
	GtkWidget *item = gtk_menu_item_new_with_label("Search & Replace");
	gtk_menu_attach(GTK_MENU(popup), item, 0, 1, 6, 7);
	g_signal_connect(item, "activate", G_CALLBACK(on_menu_item_activate), text_view);
	//gtk_widget_show(sep);
	gtk_widget_show(item);
}
/*
gdouble page_size;
void adjustment_changed(GtkAdjustment *adjustment, gpointer data) {
	page_size = gtk_adjustment_get_page_size(adjustment);
	gtk_widget_set_size_request(bottom_margin, -1, page_size); //@ that is not working...

	g_print("adjustment: page-size: %f\n", page_size);
}

void on_scrolled_window_height_changed(GObject *scrolled_window)
{
	printf("scrolled window height changed!\n");
}
*/
void on_scrolled_window_size_allocate(GtkWidget *scrolled_window, GdkRectangle *allocation, gpointer bottom_margin)
{
	printf("scrolled window size allocate!\n");
	printf("width: %d, height: %d\n", allocation->width, allocation->height);
	gtk_widget_set_size_request(GTK_WIDGET(bottom_margin), allocation->width, allocation->height);
}

void on_highlighting_selected(GtkMenuItem *item, gpointer data)
{
	printf("menuitem selected!\n");
	const char *selected_item = gtk_menu_item_get_label(item);
	//printf("label: %s\n", selected_item);
	GtkLabel *button_label = GTK_LABEL(data);

	const char *button_label_text = gtk_label_get_text(button_label);
	if (strcmp(button_label_text, selected_item) == 0) {
		printf("highlighting option already selected. no need to do anything\n");
		return;
	}

	gtk_label_set_text(button_label, selected_item);

	/* instead of visible_tab_retrieve_widget() maybe we should have something that gives us a tab to which the widget we already have belongs to. because we have a button label and we want the tab it belongs to. or really we want the buffer */
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);
	if (strcmp(selected_item, "None") == 0) {
		printf("removing highlighting...\n");
		remove_highlighting(text_buffer);
	} else {
		printf("initializing highlighting...\n");
		init_highlighting(text_buffer);
	}
}

GtkWidget *create_tab(const char *file_name)
{
	static int count = 1;
	GtkWidget *tab, *scrolled_window;
	char *tab_title, *tab_name;
	gchar *contents, *base_name;
	//GFile *file;

	g_print("create_tab()\n");

	tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	//gtk_widget_set_hexpand(tab, TRUE);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	//gtk_style_context_add_class (gtk_widget_get_style_context(scrolled_window), "scrolled-window");
	gtk_widget_set_vexpand(scrolled_window, TRUE);

	GtkTextView *text_view = GTK_TEXT_VIEW(gtk_text_view_new());
	gtk_text_view_set_pixels_above_lines(text_view, 3);
	gtk_text_view_set_left_margin(text_view, 1);
	gtk_text_view_set_wrap_mode(text_view, GTK_WRAP_WORD);
	gint position = 30; // set_tab_stops_internal() in gtksourceview
	PangoTabArray *tab_array = pango_tab_array_new(1, TRUE); //@ free?
	pango_tab_array_set_tab(tab_array, 0, PANGO_TAB_LEFT, position);
	gtk_text_view_set_tabs(text_view, tab_array);

	GtkWidget *search_revealer = gtk_revealer_new();
	gtk_widget_set_name(search_revealer, "search-revealer");
	GtkWidget *search_entry = gtk_search_entry_new();
	gtk_style_context_add_class(gtk_widget_get_style_context(search_entry), "search-entry");
	//gtk_revealer_set_reveal_child(GTK_REVEALER(search_revealer), TRUE);

	GtkWidget *command_revealer = gtk_revealer_new();
	//gtk_revealer_set_transition_type(GTK_REVEALER(sidebar_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
	gtk_widget_set_name(command_revealer, "command-revealer");
	GtkWidget *command_entry = gtk_entry_new();
	gtk_style_context_add_class(gtk_widget_get_style_context(command_entry), "command-entry");


	GtkWidget *line_nr_label = gtk_label_new("Line");
	GtkWidget *line_nr_value = gtk_label_new(NULL);
	GtkWidget *line_nr_label_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_container_add(GTK_CONTAINER(line_nr_label_container), line_nr_label);
	gtk_container_add(GTK_CONTAINER(line_nr_label_container), line_nr_value);

	GtkWidget *highlighting_label = gtk_label_new(NULL);
	//GtkWidget *image = gtk_image_new_from_icon_name("pan-down-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
	GtkWidget *image = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_NONE); // Its deprecated, but we are using a very old version of gtk currently.
	GtkWidget *highlighting_menu_button = gtk_menu_button_new();
	GtkWidget *b = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add(GTK_CONTAINER(b), highlighting_label);
	gtk_container_add(GTK_CONTAINER(b), image);
	gtk_container_add(GTK_CONTAINER(highlighting_menu_button), b);

	/*const char *status_message = "Hello world!";
	GtkWidget *status_message_label = gtk_label_new(status_message);
	gtk_label_set_text(GTK_LABEL(status_message_label), status_message);*/

	GtkWidget *status_bar = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(status_bar), 30);
	gtk_grid_attach(GTK_GRID(status_bar), line_nr_label_container, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(status_bar), highlighting_menu_button, 1, 0, 1, 1);
	//gtk_grid_attach(GTK_GRID(status_bar), status_message_label, 2, 0, 1, 1);


	gtk_style_context_add_class (gtk_widget_get_style_context(line_nr_value), "line-number-value-label");
	gtk_style_context_add_class (gtk_widget_get_style_context(highlighting_menu_button), "menu-button");
	//gtk_style_context_add_class (gtk_widget_get_style_context(status_message_label), "status-message-label");
	gtk_style_context_add_class (gtk_widget_get_style_context(status_bar), "status-bar");


	/*GtkWidget *text_view_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget *bottom_margin = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(bottom_margin), GTK_SHADOW_NONE);
	//gtk_widget_set_vexpand(bottom_margin, TRUE);
	gtk_style_context_add_class (gtk_widget_get_style_context(bottom_margin), "bottom-margin");*/

	gtk_container_add(GTK_CONTAINER(tab), search_revealer);
	gtk_container_add(GTK_CONTAINER(search_revealer), search_entry);
	//gtk_container_add(GTK_CONTAINER(text_view_container), GTK_WIDGET(text_view));
	//gtk_container_add(GTK_CONTAINER(text_view_container), bottom_margin);
	//gtk_container_add(GTK_CONTAINER(scrolled_window), text_view_container);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(text_view));
	gtk_container_add(GTK_CONTAINER(tab), scrolled_window);
	gtk_container_add(GTK_CONTAINER(tab), command_revealer);
	gtk_container_add(GTK_CONTAINER(command_revealer), command_entry);
	//gtk_container_add(GTK_CONTAINER(tab), statusbar);
	gtk_container_add(GTK_CONTAINER(tab), status_bar);

	gtk_widget_show_all(GTK_WIDGET(tab));


	struct TabInfo *tab_info;
	tab_info = malloc(sizeof(struct TabInfo)); //@ free?
	if(file_name == NULL) {
		tab_info->file_name = NULL;

		tab_title = malloc(100); //@ free? buffer bounds
		sprintf(tab_title, "%s %d", "Untitled", count);
		tab_info->title = tab_title;
	} else {
		tab_info->file_name = file_name;
		tab_info->title = get_base_name(file_name);
	}
	tab_info->id = count;
	count += 1;
	g_object_set_data(G_OBJECT(tab), "tab-info", tab_info);

	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);

	tab_add_widget_4_retrieval(tab, COMMAND_REVEALER, command_revealer);
	tab_add_widget_4_retrieval(tab, COMMAND_ENTRY, command_entry);
	tab_add_widget_4_retrieval(tab, SEARCH_REVEALER, search_revealer);
	tab_add_widget_4_retrieval(tab, SEARCH_ENTRY, search_entry);
	tab_add_widget_4_retrieval(tab, TEXT_VIEW, text_view);
	tab_add_widget_4_retrieval(tab, TEXT_BUFFER, text_buffer); //@ haa text-buffer is not a widget! void *?
	//tab_add_widget_4_retrieval(tab, STATUS_MESSAGE_LABEL, status_message_label);


	int page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab, NULL);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);

	tab_set_unsaved_changes_to(tab, FALSE);


	if(file_name != NULL) {
		char *contents = read_file(file_name); //@ error handling. if we cant open a file, we shouldnt create a tab?
		gtk_text_buffer_set_text(text_buffer, contents, -1);
		free(contents);
	}

	/* we create the menu here (after we set buffer contents) because we want to do the initial highlighting while creating the menu */
	GtkWidget *menu = gtk_menu_new();

	//const char *default_highlighting = "None";
	const char *default_highlighting = "C-ish Highlighting";
	const char *highlightings[] = {
		"C-ish Highlighting",
		"None",
		NULL
	};

	int i;
	for (i = 0; highlightings[i] != NULL; ++i) {
		//printf("%s\n", languages[i]);
		GtkWidget *item = gtk_menu_item_new_with_label(highlightings[i]);
		gtk_menu_attach(GTK_MENU(menu), item, 0, 1, i, i + 1);
		g_signal_connect(item, "activate", G_CALLBACK(on_highlighting_selected), highlighting_label);

		if (strcmp(highlightings[i], default_highlighting) == 0) {
			on_highlighting_selected(GTK_MENU_ITEM(item), highlighting_label); // set the highlighting
		}
	}

	gtk_widget_show_all(menu);

	gtk_menu_button_set_popup(GTK_MENU_BUTTON(highlighting_menu_button), menu);
	gtk_menu_button_set_direction(GTK_MENU_BUTTON(highlighting_menu_button), GTK_ARROW_UP);

	init_search(tab);
	init_undo(tab);
	init_autocomplete(tab);

	g_signal_connect(G_OBJECT(text_view), "copy-clipboard", G_CALLBACK(text_view_copy_clipboard), NULL);
	g_signal_connect(G_OBJECT(text_view), "cut-clipboard", G_CALLBACK(text_view_cut_clipboard), NULL);
	g_signal_connect(G_OBJECT(text_view), "paste-clipboard", G_CALLBACK(text_view_paste_clipboard), NULL);

	g_signal_connect(G_OBJECT(text_view), "populate-popup", G_CALLBACK(on_text_view_populate_popup), NULL);

	/*gtk_text_buffer_create_tag(text_buffer, "line-highlight", "paragraph-background", "lightgray", NULL);*/
	text_buffer_cursor_position_changed(G_OBJECT(text_buffer), NULL, line_nr_value);
	g_signal_connect(G_OBJECT(text_buffer), "notify::cursor-position", G_CALLBACK(text_buffer_cursor_position_changed), line_nr_value);

	g_signal_connect(G_OBJECT(text_buffer), "changed", G_CALLBACK(text_buffer_changed), NULL);


	/*GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
	gdouble page_size = gtk_adjustment_get_page_size(adjustment);
	printf("adjustment: page-size: %f\n", page_size); // 0.0
	g_signal_connect(G_OBJECT(adjustment), "changed", G_CALLBACK(adjustment_changed), NULL);*/
	//g_object_set(G_OBJECT(text_view), "margin-bottom", 200, NULL);
	//gtk_widget_set_vexpand(GTK_WIDGET(text_view), TRUE);
	/*int height, height_request;
	gtk_widget_get_size_request(scrolled_window, NULL, &height);
	g_object_get(G_OBJECT(scrolled_window), "height-request", &height_request, NULL);
	printf("gtk_widget_get_size_request: height: %d\n", height);
	printf("height-request: %d\n", height_request);*/

	//g_signal_connect(G_OBJECT(scrolled_window), "notify::height-request", G_CALLBACK(on_scrolled_window_height_changed), NULL);
	//g_signal_connect(G_OBJECT(scrolled_window), "size-allocate", G_CALLBACK(on_scrolled_window_size_allocate), bottom_margin);

	//gtk_widget_set_size_request(bottom_margin, -1, 1000);

	return tab;
}

char *get_file_name_from_user(GtkFileChooserAction dialog_type)
{
	const char *open_label = "Open";
	const char *save_label = "Save";
	const char *open_title = "Open File";
	const char *save_title = "Save File";
	const char *button_label, *dialog_title;

	GtkWidget *dialog;
	gint response;
	char *file_name = NULL;

	assert(dialog_type == GTK_FILE_CHOOSER_ACTION_OPEN || dialog_type == GTK_FILE_CHOOSER_ACTION_SAVE);

	if (dialog_type == GTK_FILE_CHOOSER_ACTION_OPEN) {
		dialog_title = open_title;
		button_label = open_label;
	} else { // GTK_FILE_CHOOSER_ACTION_SAVE
		dialog_title = save_title;
		button_label = save_label;
	}

	dialog = gtk_file_chooser_dialog_new(
		dialog_title,
		GTK_WINDOW(window),
		dialog_type,
		"Cancel",
		GTK_RESPONSE_CANCEL,
		button_label,
		GTK_RESPONSE_ACCEPT,
		NULL
	);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_ACCEPT) {
		file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}
	gtk_widget_destroy(dialog);

	return file_name;
}

char *get_base_name(const char *file_name)
{
	char file_name_copy[100];
	char *base_name;
	char *curr_token, *prev_token;

	sprintf(file_name_copy, "%s", file_name); // ...since strtok() modifies the string given.
	curr_token = strtok(file_name_copy, "/"); //@ What if this returns NULL?
	while(curr_token != NULL) {
		prev_token = curr_token;
		curr_token = strtok(NULL, "/");
	}
	base_name = malloc(100); //@ What if base name is longer than 99 characters?
	sprintf(base_name, "%s", prev_token);

	return base_name;
}

#define CTRL	1 // 0001
#define ALT	 	2 // 0010
#define SHIFT	4 // 0100
#define ALTGR	8 // 1000

#define SIZE_KEYCODES 	65536 	// number of different values (2**16 (key_event->hardware_keycode is a 16-bit variable))
#define SIZE_MODIFIERS 	16			// CTRL + ALT + SHIFT + ALTGR + 1
gboolean (*key_combinations[SIZE_MODIFIERS][SIZE_KEYCODES])(GdkEventKey *key_event); // global arrays should be initialized to defaults (NULL) (?)


gboolean on_window_key_press_event(GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	#define NO_MODIFIERS 0x2000000 // Value of key_event->state if no (known) modifiers are set.

	GdkEventKey *key_event = (GdkEventKey *) event;
	g_print("on_window_key_press_event(): hardware keycode: %d\n", key_event->hardware_keycode);

	unsigned short int modifiers = 0;
	if(key_event->state & GDK_CONTROL_MASK) {
		modifiers |= CTRL;
	}
	if(key_event->state & GDK_MOD1_MASK) {
		modifiers |= ALT;
	}
	if(key_event->state & GDK_SHIFT_MASK) {
		modifiers |= SHIFT;
	}
	if(key_event->state & GDK_MOD5_MASK) {
		modifiers |= ALTGR;
	}

	if(key_combinations[modifiers][key_event->hardware_keycode] != NULL) {
		if((*key_combinations[modifiers][key_event->hardware_keycode])(key_event)) {
			return TRUE; // We have dealt with the key-press and nothing else should happen!
		}
	}

	switch(key_event->hardware_keycode) {
		/*case 10: // "1" -> for testing
			g_print("key_event->state: %x\n", (unsigned int) key_event->state);

			g_print("sizeof(unsigned int): %d\n", sizeof(unsigned int));
			g_print("sizeof(key_event->state): %d\n", sizeof(key_event->state));

			g_print("GDK_SHIFT_MASK: %x\n", GDK_SHIFT_MASK); // 0x1
			g_print("GDK_LOCK_MASK: %x\n", GDK_LOCK_MASK); // 0x2
			g_print("GDK_CONTROL_MASK: %x\n", GDK_CONTROL_MASK); // 0x4

			if(key_event->state & GDK_LOCK_MASK) g_print("LOCK\n"); // CapsLock
			if(key_event->state & GDK_CONTROL_MASK) g_print("CONTROL\n"); // Ctrl
			if(key_event->state & GDK_SHIFT_MASK) g_print("SHIFT\n"); // Shift
			if(key_event->state & GDK_SUPER_MASK) g_print("SUPER\n");
			if(key_event->state & GDK_HYPER_MASK) g_print("HYPER\n");
			if(key_event->state & GDK_META_MASK) g_print("META\n");
			if(key_event->state & GDK_MOD1_MASK) g_print("MOD1\n"); // Alt
			if(key_event->state & GDK_MOD2_MASK) g_print("MOD2\n"); // NumLock
			if(key_event->state & GDK_MOD3_MASK) g_print("MOD3\n");
			if(key_event->state & GDK_MOD4_MASK) g_print("MOD4\n");
			if(key_event->state & GDK_MOD5_MASK) g_print("MOD5\n"); // AltGr

			#define NO_MODIFIERS 0x2000000

			if(key_event->state & GDK_CONTROL_MASK)
				g_print("& -> CTRL + \"1\"\n"); // including other combinations
			if(key_event->state == (NO_MODIFIERS | GDK_CONTROL_MASK))
				g_print("== -> CTRL + \"1\"\n"); // excluding other combinations
			break;*/
	}

	return FALSE;
}

/* STYLING: */

/*void apply_style(GtkWidget *widget) {
	g_print("apply_style()\n");
	assert(provider != NULL);
	GtkStyleContext *context = gtk_widget_get_style_context(widget);
	gtk_style_context_add_provider(
		context,
		GTK_STYLE_PROVIDER(provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	if(GTK_IS_TEXT_VIEW(widget)) {
		GtkTextView *text_view = GTK_TEXT_VIEW(widget);

		gtk_text_view_set_pixels_above_lines(text_view, 1);
		gtk_text_view_set_left_margin(text_view, 1);
		gtk_text_view_set_wrap_mode(text_view, GTK_WRAP_WORD);

		gint position = 30; // set_tab_stops_internal() in gtksourceview
		PangoTabArray *tab_array = pango_tab_array_new(1, TRUE); //@ free?
		pango_tab_array_set_tab(tab_array, 0, PANGO_TAB_LEFT, position);
		gtk_text_view_set_tabs(text_view, tab_array);
	}
}*/

/* 
GTK 3.10 doesnt seem to support gtk_text_view_set_bottom_margin()
Setting margins and paddings in css has no effect whatsoever, not even error messages complaining about unknown properties.
*/
/*void parse_settings() {
	char *css, declarations[1000], *contents, *line, *property, *value;
	declarations[0] = 0;
	
	css = malloc(1000); // @free? @buffer bounds
	// ... css shouldnt be a local buffer right?

	g_print("parse_settings()\n");

	contents = read_file(settings_file); //@ free?
	if(contents == NULL) { // No settings file
		sprintf(declarations, "font: Ubuntu Mono 13; color: black; background: white;");
	} else {
		//g_print("[%s]: \"%s\"\n", settings_file, contents);
	
		char *contents_end;
		line = strtok_r(contents, "\n", &contents_end);
		while(line != NULL) {
			char *line_end;
			//g_print("line: %s\n", line);
			property = strtok_r(line, ":", &line_end);
			value = strtok_r(NULL, ":", &line_end);
			if(value == NULL) {
				//g_print("Skip line: \"%s\"\n", line);
			} else {
				//g_print("property: %s\n", property);
				//g_print("value: %s\n", value);
				sprintf(declarations, "%s %s: %s;", declarations, property, value);
			}
			line = strtok_r(NULL, "\n", &contents_end);
		}
	}

	sprintf(css, "GtkTextView, GtkSearchEntry {%s}", declarations);
	sprintf(css, "%s GtkTextView:selected, GtkSearchEntry:selected {background: blue; color: white;}",
		css
	);
	g_print("parse_settings(): css: %s\n", css);

	provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(provider, css, -1, NULL);
}*/


void refresh_application_title(GtkWidget *tab)
{
	gpointer data = g_object_get_data(G_OBJECT(tab), "tab-info");
	assert(data != NULL);
	struct TabInfo *tab_info = (struct TabInfo *) data;
	if(tab_info->file_name != NULL) {
		gtk_window_set_title(GTK_WINDOW(window), tab_info->file_name); //@
	} else {
		gtk_window_set_title(GTK_WINDOW(window), tab_info->title);
	}
}

void on_notebook_switch_page(GtkNotebook *notebook, GtkWidget *tab, guint page_num, gpointer data)
{
	printf("on_notebook_switch_page() called!\n");

	refresh_application_title(tab);
}

/* Another way to implement autocomplete would be to intercept later on when handling the buffers insert-text event. */
gboolean autocomplete_character(GdkEventKey *key_event)
{
	GtkWidget *text_view = GTK_WIDGET(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW));
	if (GTK_IS_TEXT_VIEW(text_view) == FALSE || gtk_widget_is_focus(text_view) == FALSE) {
		printf("autocomplete_character(): early-out...\n");
		return FALSE;
	}

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER));
	actually_autocomplete_character(text_buffer, (char) key_event->keyval);

	return TRUE;
}
/*
gboolean move_lines_up(GdkEventKey *key_event)
{
	GtkTextBuffer *text_buffer;
	if ((text_buffer = get_visible_text_buffer(GTK_NOTEBOOK(notebook))) == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	actually_open_line_before(text_buffer);
}

gboolean move_lines_down(GdkEventKey *key_event)
{
	GtkTextBuffer *text_buffer;
	if ((text_buffer = get_visible_text_buffer(GTK_NOTEBOOK(notebook))) == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	actually_open_line_before(text_buffer);
}

gboolean duplicate_line(GdkEventKey *key_event)
{
	GtkTextBuffer *text_buffer;
	if ((text_buffer = get_visible_text_buffer(GTK_NOTEBOOK(notebook))) == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	actually_open_line_before(text_buffer);
}*/

gboolean open_line_before(GdkEventKey *key_event)
{
	GtkTextBuffer *text_buffer;
	if ((text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER)) == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	actually_open_line_before(text_buffer);
}

gboolean open_line_after(GdkEventKey *key_event)
{
	GtkTextBuffer *text_buffer;
	if ((text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER)) == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	actually_open_line_after(text_buffer);
}

gboolean create_empty_tab(GdkEventKey *key_event)
{
	create_tab(NULL);
	return TRUE;
}

gboolean close_tab(GdkEventKey *key_event)
{
	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	if (page == -1) {
		// no tabs open
		return FALSE;
	}
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);
	gtk_widget_destroy(tab);
	return TRUE;
}

gboolean switch_tab(GdkEventKey *key_event)
{
	int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
	//printf("switch_tab(): n_pages: %d\n", n_pages);
	int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)); // returns -1 if no pages, starts counting from 0
	//printf("switch_tab(): page_n: %d\n", current_page);

	if (n_pages < 2) {
		printf("switch_tab(): less than 2 tabs open -> nowhere to go...\n");
		return TRUE;
	}

	int last_page = n_pages - 1;
	int target_page = current_page + 1;
	if (target_page > last_page) target_page = 0;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), target_page);

	return TRUE;
}

gboolean less_fancy_toggle_sidebar(GdkEventKey *key_event)
{
	if (gtk_widget_is_visible(sidebar_container) == TRUE)
		gtk_widget_hide(sidebar_container);
	else
		gtk_widget_show(sidebar_container);

	return TRUE;
}

gboolean less_fancy_toggle_notebook(GdkEventKey *key_event)
{
	if (gtk_widget_is_visible(notebook) == TRUE)
		gtk_widget_hide(notebook);
	else
		gtk_widget_show(notebook);

	return TRUE;
}

gboolean toggle_search_entry(GdkEventKey *key_event)
{
	GtkWidget *search_entry = visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), SEARCH_ENTRY);
	GtkRevealer *search_revealer = (GtkRevealer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), SEARCH_REVEALER);
	//GtkRevealer *search_revealer = GTK_REVEALER(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), SEARCH_REVEALER));
	GtkWidget *text_view = visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW);

	if (search_entry == NULL && search_revealer == NULL && text_view == NULL) {
		printf("no tabs open -> doing nothing\n");
		return FALSE;
	}

	assert(search_entry != NULL);
	assert(search_revealer != NULL);
	assert(text_view != NULL);

	if(gtk_revealer_get_reveal_child(search_revealer) == FALSE) { // not revealed
		gtk_revealer_set_reveal_child(search_revealer, TRUE);
		gtk_widget_grab_focus(search_entry);
	} else if(gtk_widget_is_focus(search_entry)) { // revealed
		gtk_revealer_set_reveal_child(search_revealer, FALSE);
		gtk_widget_grab_focus(text_view);
	} else {
		gtk_widget_grab_focus(search_entry);
	}

	return TRUE;
}

gboolean toggle_command_entry(GdkEventKey *key_event)
{
	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	if (page == -1) {
		// no tabs open
		return FALSE;
	}
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);

	GtkRevealer *command_revealer = GTK_REVEALER(tab_retrieve_widget(tab, COMMAND_REVEALER));
	assert(GTK_IS_REVEALER(command_revealer));
	GtkWidget *command_entry = tab_retrieve_widget(tab, COMMAND_ENTRY);
	assert(GTK_IS_ENTRY(command_entry));
	GtkWidget *text_view = tab_retrieve_widget(tab, TEXT_VIEW);
	assert(GTK_IS_TEXT_VIEW(text_view));

	if(gtk_revealer_get_reveal_child(command_revealer) == FALSE) { // not revealed
		gtk_revealer_set_reveal_child(command_revealer, TRUE);
		gtk_widget_grab_focus(command_entry);
	} else if(gtk_widget_is_focus(command_entry)) { // revealed & focus
		gtk_revealer_set_reveal_child(command_revealer, FALSE);
		gtk_widget_grab_focus(text_view);
	} else {
		gtk_widget_grab_focus(command_entry);
	}

	return TRUE;
}

/* tab-handling should probably be refactored in a more sensible way */
gboolean handle_tab(GdkEventKey *key_event)
{
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);
	if (text_buffer == NULL) {
		printf("no tabs open\n");
		return FALSE;
	}

	if(handle_tab_key(text_buffer, key_event) == TRUE) return TRUE; // We handled the tab...
	return FALSE; // Otherwise, we dont know...
}

gboolean handle_enter(GdkEventKey *key_event)
{
	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	if(tab == NULL) {
		printf("no tabs open -> doing nothing\n");
		return FALSE;
	}

	//GtkWidget *search_entry = visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), SEARCH_ENTRY);
	GtkWidget *search_entry = tab_retrieve_widget(tab, SEARCH_ENTRY);
	if(gtk_widget_is_focus(search_entry) == TRUE) {
		do_search(tab);
		return TRUE;
	}

	//GtkWidget *command_entry = visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), COMMAND_ENTRY);
	GtkWidget *command_entry = tab_retrieve_widget(tab, COMMAND_ENTRY);
	if(gtk_widget_is_focus(command_entry) == TRUE) {
		const char *text = gtk_entry_get_text(GTK_ENTRY(command_entry)); //@ free?
		if(strlen(text) == 0) {
			return TRUE;
		}
		GtkTextView *text_view = (GtkTextView *) tab_retrieve_widget(tab, TEXT_VIEW);
		GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);
		int line_number = atoi(text);
		//@ Should check if valid line number maybe...
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_line(text_buffer, &iter, line_number - 1); // ...counting from 0 or 1
		gtk_widget_grab_focus(GTK_WIDGET(text_view));
		gtk_text_buffer_place_cursor(text_buffer, &iter);
		gtk_text_view_scroll_to_iter(text_view, &iter, 0.0, FALSE, 0.0, 0.0);
		return TRUE;
	}

	return FALSE;
}

gboolean undo_last_action(GdkEventKey *key_event)
{
	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	if (page == -1) {
		// no tabs open
		printf("Ctrl + Z: No tabs open.\n");
		return FALSE;
	}
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);

	actually_undo_last_action(tab);

	return TRUE;
}

gboolean do_save(GdkEventKey *key_event)
{
	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	if (tab == NULL) {
		printf("no tabs open -> doing nothing\n");
		return FALSE;
	}

	GtkTextView *text_view = (GtkTextView *) tab_retrieve_widget(tab, TEXT_VIEW);
	assert(text_view != NULL);
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);

	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(text_buffer, &start, &end);
	char *contents = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);

	struct TabInfo *tab_info = g_object_get_data(G_OBJECT(tab), "tab-info");
	assert(tab_info != NULL);

	if (tab_info->file_name == NULL) { // The tab doesnt have a file associated with it yet.
		const char *file_name = get_file_name_from_user(GTK_FILE_CHOOSER_ACTION_SAVE);
		//printf("SAVE: %s\n", file_name);
		if(file_name == NULL) return TRUE; // User didnt give us a file name.
		tab_info->file_name = file_name;
		tab_info->title = get_base_name(file_name);
		refresh_application_title(tab);
	}

	write_file(tab_info->file_name, contents); //@ error handling

	tab_set_unsaved_changes_to(tab, FALSE);

	return TRUE;
}

gboolean do_open(GdkEventKey *key_event)
{
	const char *file_name; // We get NULL if user closed the dialog without choosing a file.
	if ((file_name = get_file_name_from_user(GTK_FILE_CHOOSER_ACTION_OPEN)) != NULL) {
		create_tab(file_name);
	}

	return TRUE;
}

void apply_css_from_file(const char *file_name)
{
	printf("applying css from \"%s\"..\n", file_name);
	// Apply css from file:
	static GtkCssProvider *provider = NULL;
	GdkScreen *screen = gdk_screen_get_default();
	if (provider != NULL) {
		printf("apply_css_from_file(): removing an old provider...\n");
		gtk_style_context_remove_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider));
	}
	printf("apply_css_from_file(): creating a new provider...\n");
	provider = gtk_css_provider_new();
	GFile *css_file = g_file_new_for_path(file_name);
	gtk_css_provider_load_from_file(provider, css_file, NULL);
	assert(screen != NULL); assert(provider != NULL);
	//printf("before gtk_style_context_add_provider_for_screen\n");
	gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	//gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_FALLBACK);
	//printf("after gtk_style_context_add_provider_for_screen\n");
	//gtk_widget_show_all(window);
}

void *watch_for_changes(void *data)
{
	const char *file_name = (const char *) data;
	printf("Should watch for changes in: %s\n", file_name);

	int inotify_descriptor = inotify_init();
	if (inotify_descriptor == -1) {
		printf("init_inotify() error!\n");
		return NULL;
	}
	
	int watch_descriptor = inotify_add_watch(inotify_descriptor, file_name, IN_MODIFY);
	if (watch_descriptor == -1) {
		printf("inotify_add_watch() error!\n");
		return NULL;
	}

	char buffer[(sizeof(struct inotify_event) + NAME_MAX + 1)];

	while (1) {
		ssize_t bytes_read = read(inotify_descriptor, buffer, sizeof(buffer)); //@ can fail
		//printf("bytes read by read(): %ld\n", bytes_read);

		char *ptr = buffer;
		while (ptr < buffer + bytes_read) {
			struct inotify_event *event = (struct inotify_event *) ptr;

			if (event->mask & IN_MODIFY) {
				printf("file modified!\n");
				apply_css_from_file(file_name);
			}
			ptr += sizeof(struct inotify_event) + event->len;
		}
	}
}

void activate_handler(GtkApplication *app, gpointer data) {

	LOG_MSG("activate_handler() called\n");

	guint major = gtk_get_major_version();
	guint minor = gtk_get_minor_version();
	guint micro = gtk_get_micro_version();
	printf("GTK version: %u.%u.%u\n", major, minor, micro);

	//test_get_parent_path();

	//gtk_menu_popup_at_rect(); // -> undefined reference to `gtk_menu_popup_at_rect'
	//gtk_menu_popdown (); // -> too few arguments to function ‘gtk_menu_popdown’

	/*gboolean open_line_before(GtkTextBuffer *text_buffer);
	gboolean open_line_after(GtkTextBuffer *text_buffer);
	gboolean autocomplete_single_quote(GtkTextBuffer *text_buffer);

	void autocomplete_character_a(GtkTextBuffer *text_buffer);
	void autocomplete_character_b(GtkTextBuffer *text_buffer);
	void autocomplete_character_c(GtkTextBuffer *text_buffer);
	key_combinations[0][38] = autocomplete_character_a;
	key_combinations[0][56] = autocomplete_character_b;
	key_combinations[0][54] = autocomplete_character_c;

	unsigned short modifiers = CTRL | ALT;
	key_combinations[modifiers][10] = test_handler; // ctrl + alt + 1 (10)
	modifiers = ALT;
	key_combinations[modifiers][35] = open_line_before; // alt + õ (35)
	key_combinations[modifiers][51] = open_line_after; // alt + ' (51)*/
	/*modifiers = 0;
	key_combinations[modifiers][51] = autocomplete_single_quote; // ' (51)*/

	key_combinations[0][23] = handle_tab; // <tab>
	key_combinations[SHIFT][23] = handle_tab; // <tab> + shift
	key_combinations[0][36] = handle_enter; // <enter>

	key_combinations[ALT][111] = move_lines_up; // alt + <up arrow>
	key_combinations[ALT][116] = move_lines_down; // alt + <down arrow>
	key_combinations[ALT][40] = duplicate_line; // alt + d
	key_combinations[ALT][119] = delete_line; // alt + <delete>
	key_combinations[ALT][35] = open_line_before; // alt + õ (35)
	key_combinations[ALT][51] = open_line_after; // alt + ' (51)

	// Auto-close(/-complete):
	key_combinations[0][51] = autocomplete_character; // 51 -> '
	key_combinations[SHIFT][11] = autocomplete_character; // shift + 11 -> "
	key_combinations[SHIFT][17] = autocomplete_character; // shift + 17 -> (
	key_combinations[ALTGR][16] = autocomplete_character; // altgr + 16 -> {
	key_combinations[ALTGR][17] = autocomplete_character; // altgr + 17 -> [

	key_combinations[CTRL][27] = search_and_replace; // ctrl + r

	key_combinations[CTRL][52] = undo_last_action; // ctrl + z

	key_combinations[CTRL][57] = create_empty_tab; // ctrl + n
	key_combinations[CTRL][58] = close_tab; // ctrl + m
	key_combinations[CTRL][45] = switch_tab; // ctrl + k

	key_combinations[CTRL][39] = do_save; // ctrl + s
	key_combinations[CTRL][32] = do_open; // ctrl + o

	key_combinations[CTRL][41] = toggle_search_entry; // ctrl + f
	key_combinations[CTRL][43] = toggle_command_entry; // ctrl + h

	key_combinations[CTRL][42] = less_fancy_toggle_sidebar; // ctrl + g
	key_combinations[CTRL][44] = less_fancy_toggle_notebook; // ctrl + j


	//char *css_file = "themes/css";
	char *css_file = "/home/eero/everything/git/text-editor/themes/css";
	apply_css_from_file(css_file);

	pthread_t id;
	int r = pthread_create(&id, NULL, watch_for_changes, css_file);
	if (r != 0) {
		printf("pthread_create() error!\n");
	}

	/*uid_t real_uid = getuid();
	uid_t effective_uid = geteuid();
	printf("real uid: %d, effective uid: %d\n", real_uid, effective_uid);*/

/*
Cant call set_root_dir() here because it expects file-browser and root-navigation to be already created.
If we used some kind of event/signal-thing, which allows abstractions to register callbacks to be executed in response to events like "root-directory-change" we wouldnt have to worry about that. Because then the code that individual-abstractions need to run would be provided by them in the form of callbacks and wouldnt be hardcoded into set_root_dir function.
*/
	strcpy(root_dir, "/home/eero"); 

	GtkWidget *sidebar_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(sidebar_notebook), GTK_POS_BOTTOM);

	file_browser = create_file_browser_widget();
	GtkWidget *file_browser_scrollbars = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(file_browser_scrollbars), file_browser);
	GtkWidget *title_image = gtk_image_new_from_file("icons/colors/file-browser.png");
	gtk_notebook_append_page(GTK_NOTEBOOK(sidebar_notebook), file_browser_scrollbars, title_image);
	gtk_widget_set_tooltip_text(title_image, "File Browser");

	GtkWidget *search_in_files = create_search_in_files_widget();
	/*GtkWidget *search_in_files_scrollbars = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(search_in_files_scrollbars), search_in_files);*/
	GtkWidget *title_image2 = gtk_image_new_from_file("icons/colors/search-in-files.png");
	//gtk_notebook_append_page(GTK_NOTEBOOK(sidebar_notebook), search_in_files_scrollbars, title_image2);
	gtk_notebook_append_page(GTK_NOTEBOOK(sidebar_notebook), search_in_files, title_image2);
	gtk_widget_set_tooltip_text(title_image2, "Search in Files");

	GtkWidget *root_nav = create_root_nav_widget();

	sidebar_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(sidebar_container), root_nav);
	gtk_container_add(GTK_CONTAINER(sidebar_container), sidebar_notebook);
	gtk_style_context_add_class (
		gtk_widget_get_style_context(sidebar_container),
		"sidebar-container");

	//gtk_widget_set_vexpand(sidebar_container, TRUE);
	//gtk_widget_set_hexpand(sidebar_container, TRUE);
	gtk_widget_set_vexpand(sidebar_notebook, TRUE);
	gtk_widget_set_hexpand(sidebar_notebook, TRUE);
	//gtk_widget_set_hexpand(sidebar_revealer, TRUE);


	notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	// Generally we would like to keep the focus on the text-view widget.
	// I tested this and the only conclusion I arrived at is that its best not to touch the focus at all.
	g_signal_connect(notebook, "switch-page", G_CALLBACK(on_notebook_switch_page), NULL);
	//g_signal_connect(notebook, "focus-in-event", G_CALLBACK(on_notebook_focus_in_event), NULL);
	//g_signal_connect(notebook, "page-added", G_CALLBACK(on_notebook_page_added), NULL);
	gtk_style_context_add_class (
		gtk_widget_get_style_context(notebook),
		"main-notebook");
	
	gtk_widget_set_hexpand(notebook, TRUE);


	window = gtk_application_window_new(app);
	//gtk_window_set_title(GTK_WINDOW(window), "Hello world!");
	gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);

	g_signal_connect(window, "key-press-event", G_CALLBACK(on_window_key_press_event), NULL);

	GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_add1(GTK_PANED(paned), sidebar_container);
	gtk_paned_add2(GTK_PANED(paned), notebook);
	gtk_container_add(GTK_CONTAINER(window), paned);

	gtk_widget_show_all(window);

	create_tab(NULL); // re-factor: create_tab() could just create the tab widget, it doesnt have to depend on the notebook at all (?)

	/*GtkWidget *popup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_decorated(GTK_WINDOW(popup_window), FALSE);
	gtk_window_set_position(GTK_WINDOW(popup_window), GTK_WIN_POS_CENTER);
	gtk_window_set_transient_for(GTK_WINDOW(popup_window), GTK_WINDOW(window));
	gtk_widget_show_all(popup_window);*/
}

int main() {

	int status;
	GtkApplication *app;

	//parse_settings();

	app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate_handler), NULL);

	status = g_application_run(G_APPLICATION(app), 0, NULL);

	g_object_unref(app);
	return status;
}





