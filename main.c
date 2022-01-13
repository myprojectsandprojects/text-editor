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

GtkWidget *app_window;
GtkWidget *nb_container;
GtkWidget *notebook;
GtkWidget *sidebar_container;
GtkWidget *file_browser; // GtkTreeView

//GtkWidget *root_selection;
//gulong root_selection_changed_id;
//int root_selection_index; // file-browser needs access to it // index of the last item in root-selection. we need to know that because each time user selects a new root by double-clicking on a directory well set the last item to that directory.

/*
This is like a root directory of our project (or workspace or)
*/
#define ROOT_DIR_SIZE 100
char root_dir[ROOT_DIR_SIZE];

extern GtkWidget *root_dir_label;

const char *filebrowser_icon_path = "/home/eero/all/text-editor/icons/for-light-theme/files-20.png";
const char *searchinfiles_icon_path = "/home/eero/all/text-editor/icons/for-light-theme/search-20.png";
//const char *unsaved_changes_icon_path = "/home/eero/all/text-editor/icons/my-supercool-icons/is-exclamation-mark-20.png";
const char *unsaved_changes_icon_path = "/home/eero/all/text-editor/icons/my-supercool-icons/is-exclamation-mark-16.png";
const char *file_icon_path = "/home/eero/all/text-editor/icons/for-light-theme/file-24.png";
const char *folder_icon_path = "/home/eero/all/text-editor/icons/for-light-theme/folder-24.png";
//const char *home_icon_path = "/home/eero/all/text-editor/icons/for-light-theme/house-20-tr.png";
const char *home_icon_path = "/home/eero/all/text-editor/icons/for-light-theme/house-20.png";
//const char *parent_dir_icon_path = "/home/eero/all/text-editor/icons/for-light-theme/arrow-20-tr.png";
const char *parent_dir_icon_path = "/home/eero/all/text-editor/icons/for-light-theme/arrow-20.png";
const char *search_icon_path = "/home/eero/all/text-editor/icons/for-light-theme/search-20.png";

const char *settings_file_path = "/home/eero/all/text-editor/themes/settings";
//char *css_file = "/home/eero/all/text-editor/themes/css";
char *css_file_path = "/home/eero/all/text-editor/themes/css"; // cant be const because we pass it to pthread_create()


struct Settings settings = {.left_margin = 1, .unknown_color = "cornflowerblue"};
// ... other members are initialized to 0 (?)


void add_class(GtkWidget *widget, const char *class_name)
{
	GtkStyleContext *style_context = gtk_widget_get_style_context(widget);
	gtk_style_context_add_class(style_context, class_name);
}


void remove_class(GtkWidget *widget, const char *class_name)
{
	GtkStyleContext *style_context = gtk_widget_get_style_context(widget);
	gtk_style_context_remove_class(style_context, class_name);
}


void set_root_dir(const char *path)
{
	assert(path != NULL);
	assert(strlen(path) < ROOT_DIR_SIZE);
	assert(strlen(path) > 0);

	snprintf(root_dir, ROOT_DIR_SIZE, "%s", path);
	
	gtk_label_set_text(GTK_LABEL(root_dir_label), root_dir);
	refresh_application_title();
	gtk_tree_view_set_model(GTK_TREE_VIEW(file_browser),
		GTK_TREE_MODEL(create_store()));
}


void tab_set_unsaved_changes_to(GtkWidget *tab, gboolean unsaved_changes)
{
	LOG_MSG("tab_set_unsaved_changes_to()\n");

	gpointer data = g_object_get_data(G_OBJECT(tab), "tab-info");
	assert(data != NULL);
	struct TabInfo *tab_info = (struct TabInfo *) data;

	tab_info->unsaved_changes = unsaved_changes;

	GtkWidget *title_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	GtkWidget *title_label = gtk_label_new(tab_info->title);
	gtk_container_add(GTK_CONTAINER(title_widget), title_label);

	if (unsaved_changes == TRUE) {
		GtkWidget *unsaved_changes_icon = gtk_image_new_from_file(unsaved_changes_icon_path);
		gtk_container_add(GTK_CONTAINER(title_widget), unsaved_changes_icon);
	}

	gtk_widget_show_all(title_widget);

	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), tab, title_widget);
}


void text_buffer_changed(GtkTextBuffer *text_buffer, gpointer user_data)
{
	LOG_MSG("text_buffer_changed()\n");

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
	LOG_MSG("text_buffer_cursor_position_changed()\n");

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(object);
	int position, line_number, char_offset;
	g_object_get(G_OBJECT(text_buffer), "cursor-position", &position, NULL);
	GtkTextIter i;
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(text_buffer), &i, position);
	line_number = gtk_text_iter_get_line(&i);
	char_offset = gtk_text_iter_get_line_offset(&i);

	char buffer[100];
	sprintf(buffer, "%d, %d", line_number + 1, char_offset + 1);
	GtkLabel *label = (GtkLabel *) user_data;
	gtk_label_set_text(label, buffer);

	/* Line highlighting -- it messes up code highlighting. */
	GtkTextIter start, end, start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
	gtk_text_buffer_remove_tag_by_name(text_buffer, "line-highlight", &start_buffer, &end_buffer);
	gtk_text_iter_set_line_offset(&i, 0);
	start = i;
	gtk_text_iter_forward_line(&i);
	//gtk_text_iter_forward_char(&i);
	//gtk_text_iter_backward_char(&i);
	end = i;
	//printf("applying the tag: %d, %d\n", gtk_text_iter_get_offset(&start), gtk_text_iter_get_offset(&end));
	gtk_text_buffer_apply_tag_by_name(text_buffer, "line-highlight", &start, &end);
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
	printf("paste-clipboard!!!\n");

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


/*
void display_search_and_replace_dialog()
{
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
*/
/*
gboolean search_and_replace(GdkEventKey *key_event)
{
	display_search_and_replace_dialog();
}

void on_menu_item_activate(GtkMenuItem *item, gpointer data)
{
	display_search_and_replace_dialog();
}
*/
/*
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
*/

void on_scrolled_window_size_allocate(GtkWidget *scrolled_window, GdkRectangle *allocation, gpointer bottom_margin)
{
	printf("scrolled window size allocate!\n");
	printf("width: %d, height: %d\n", allocation->width, allocation->height);
	gtk_widget_set_size_request(GTK_WIDGET(bottom_margin), allocation->width, allocation->height);
}


void on_highlighting_selected(GtkMenuItem *item, gpointer data)
{
	LOG_MSG("on_highlighting_selected()\n");

	const char *selected_item = gtk_menu_item_get_label(item);
	//printf("label: %s\n", selected_item);
	GtkLabel *button_label = GTK_LABEL(data);

	const char *button_label_text = gtk_label_get_text(button_label);
	if (strcmp(button_label_text, selected_item) == 0) {
		LOG_MSG("on_highlighting_selected(): highlighting option already selected. no need to do anything\n");
		return;
	}

	gtk_label_set_text(button_label, selected_item);

	/* instead of visible_tab_retrieve_widget() maybe we should have something that gives us a tab to which the widget we already have belongs to. because we have a button label and we want the tab it belongs to. or really we want the buffer */
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);
	if (strcmp(selected_item, "None") == 0) {
		LOG_MSG("on_highlighting_selected(): removing highlighting...\n");
		remove_highlighting(text_buffer);
	} else {
		LOG_MSG("on_highlighting_selected(): initializing highlighting...\n");
		init_highlighting(text_buffer);
	}
}


void on_text_view_size_allocate(GtkWidget *textview, GdkRectangle *alloc, gpointer data) {
	LOG_MSG("on_text_view_size_allocate()\n");
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(textview), alloc->height);
}


void on_adjustment_value_changed(GtkAdjustment *adj, gpointer user_data)
{
	//printf("*** on_adjustment_value_changed()\n");

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER));
	GtkTextView *text_view = GTK_TEXT_VIEW(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW));
	gdouble value = gtk_adjustment_get_value(adj);
	// need to translate that value into a line number somehow
	// y must be in buffer-coordinates. gtk_text_view_window_to_buffer_coords()
	GtkTextIter where;
	gtk_text_view_get_line_at_y(text_view, &where, value, NULL);
	gint line = gtk_text_iter_get_line(&where);
	//printf("*** on_adjustment_value_changed: line: %d\n", line);
	gtk_text_buffer_move_mark_by_name(text_buffer, "scroll-mark", &where);
}


GtkWidget *create_tab(const char *file_name)
{
	static int count = 1;
	GtkWidget *tab, *scrolled_window;
	char *tab_title, *tab_name;
	gchar *contents, *base_name;
	//GFile *file;

	LOG_MSG("create_tab()\n");

	tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	//gtk_widget_set_hexpand(tab, TRUE);

	struct TabInfo *tab_info;
	tab_info = malloc(sizeof(struct TabInfo)); //@ free?
	if (file_name == NULL) {
		tab_info->file_name = NULL;

		tab_title = malloc(100); //@ free?
		snprintf(tab_title, 100, "%s %d", "Untitled", count);
		tab_info->title = tab_title;
	} else {
		tab_info->file_name = file_name; //@ shouldnt we malloc a new buffer?
		tab_info->title = get_base_name(file_name);
	}
	tab_info->id = count;
	count += 1;
	g_object_set_data(G_OBJECT(tab), "tab-info", tab_info);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	//gtk_style_context_add_class (gtk_widget_get_style_context(scrolled_window), "scrolled-window");
	gtk_widget_set_vexpand(scrolled_window, TRUE);

	GtkTextView *text_view = GTK_TEXT_VIEW(gtk_text_view_new());
	gtk_text_view_set_pixels_above_lines(text_view, settings.pixels_above_lines);
	//gtk_text_view_set_pixels_below_lines(text_view, 30);
	gtk_text_view_set_pixels_below_lines(text_view, settings.pixels_below_lines);
	gtk_text_view_set_left_margin(text_view, settings.left_margin);
	gtk_text_view_set_wrap_mode(text_view, GTK_WRAP_WORD);
	gint position = 30; // set_tab_stops_internal() in gtksourceview
	PangoTabArray *tab_array = pango_tab_array_new(1, TRUE); //@ free?
	pango_tab_array_set_tab(tab_array, 0, PANGO_TAB_LEFT, position);
	gtk_text_view_set_tabs(text_view, tab_array);

	add_class(GTK_WIDGET(text_view), "text-view");
	g_signal_connect(G_OBJECT(text_view), "size-allocate", G_CALLBACK(on_text_view_size_allocate), NULL);


	//printf("*** file-name: %s, title: %s\n", tab_info->file_name, tab_info->title);
	GtkWidget *file_path_label = gtk_label_new(NULL);
	GtkWidget *line_number_label = gtk_label_new(NULL);
	GtkWidget *separator_label = gtk_label_new(":");

	if (tab_info->file_name) {
		gtk_label_set_text(GTK_LABEL(file_path_label), tab_info->file_name);
	} else {
		gtk_label_set_text(GTK_LABEL(file_path_label), tab_info->title);
	}

	GtkWidget *statusbar_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_container_add(GTK_CONTAINER(statusbar_container), file_path_label);
	gtk_container_add(GTK_CONTAINER(statusbar_container), separator_label);
	gtk_container_add(GTK_CONTAINER(statusbar_container), line_number_label);


	GtkWidget *hl_label = gtk_label_new(NULL);
	GtkWidget *hl_menu_button = gtk_menu_button_new();
	gtk_container_add(GTK_CONTAINER(hl_menu_button), hl_label);

	GtkWidget *status_bar = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(status_bar), 0);
	GtkWidget *margin = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(margin, 10, -1);
	GtkWidget *space = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand(space, TRUE);
	gtk_grid_attach(GTK_GRID(status_bar), margin, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(status_bar), statusbar_container, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(status_bar), space, 2, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(status_bar), hl_menu_button, 3, 0, 1, 1);

	add_class(status_bar, "status-bar");
	//add_class(line_nr_value, "line-number-value-label");
	add_class(hl_menu_button, "code-highlighting-menu-button");

	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);

	if(file_name != NULL) {
		char *contents = read_file(file_name); //@ error handling. if we cant open a file, we shouldnt create a tab?
		gtk_text_buffer_set_text(text_buffer, contents, -1);
		free(contents);
	}

	/*
	tab_add_widget_4_retrieval(tab, COMMAND_REVEALER, command_revealer);
	tab_add_widget_4_retrieval(tab, COMMAND_ENTRY, command_entry);
	*/
	tab_add_widget_4_retrieval(tab, TEXT_VIEW, text_view);
	tab_add_widget_4_retrieval(tab, TEXT_BUFFER, text_buffer); //@ haa text-buffer is not a widget! void *?
	tab_add_widget_4_retrieval(tab, FILEPATH_LABEL, file_path_label);
	tab_add_widget_4_retrieval(tab, SCROLLED_WINDOW, scrolled_window);
	
	GtkWidget *wgt_search = create_search_widget(tab);

	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(text_view));
	gtk_container_add(GTK_CONTAINER(tab), wgt_search);
	gtk_container_add(GTK_CONTAINER(tab), scrolled_window);
	gtk_container_add(GTK_CONTAINER(tab), status_bar);

	gtk_widget_show_all(GTK_WIDGET(tab));

	int page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab, NULL);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);

	/* We want autocomplete-character's handler for "insert-text"-signal to be the first handler called.  
	(code-highlighting and undo also register callbacks for this signal.) */
	init_autocomplete_character(text_buffer);

	tab_set_unsaved_changes_to(tab, FALSE);

	/* we create the menu here (after we set buffer contents) because we want to do the initial highlighting while creating the menu */
	GtkWidget *menu = gtk_menu_new();

	//const char *default_highlighting = "None";
	const char *default_highlighting = "C";
	const char *highlightings[] = {
		"C",
		"None",
		NULL
	};

	int i;
	for (i = 0; highlightings[i] != NULL; ++i) {
		//printf("%s\n", languages[i]);
		GtkWidget *item = gtk_menu_item_new_with_label(highlightings[i]);
		gtk_menu_attach(GTK_MENU(menu), item, 0, 1, i, i + 1);
		g_signal_connect(item, "activate", G_CALLBACK(on_highlighting_selected), hl_label);

		if (strcmp(highlightings[i], default_highlighting) == 0) {
			on_highlighting_selected(GTK_MENU_ITEM(item), hl_label); // set the highlighting
		}
	}

	gtk_widget_show_all(menu);

	gtk_menu_button_set_popup(GTK_MENU_BUTTON(hl_menu_button), menu);
	//gtk_menu_button_set_direction(GTK_MENU_BUTTON(hl_menu_button), GTK_ARROW_UP);

	init_undo(tab);

	g_signal_connect(G_OBJECT(text_view), "copy-clipboard", G_CALLBACK(text_view_copy_clipboard), NULL);
	g_signal_connect(G_OBJECT(text_view), "cut-clipboard", G_CALLBACK(text_view_cut_clipboard), NULL);
	g_signal_connect(G_OBJECT(text_view), "paste-clipboard", G_CALLBACK(text_view_paste_clipboard), NULL);

	//g_signal_connect(G_OBJECT(text_view), "populate-popup", G_CALLBACK(on_text_view_populate_popup), NULL);

	gtk_text_buffer_create_tag(text_buffer,
		"line-highlight", "paragraph-background", settings.line_highlight_color, NULL);

	text_buffer_cursor_position_changed(G_OBJECT(text_buffer), NULL, line_number_label);
	g_signal_connect(G_OBJECT(text_buffer),
		"notify::cursor-position", G_CALLBACK(text_buffer_cursor_position_changed), line_number_label);

	g_signal_connect(G_OBJECT(text_buffer), "changed", G_CALLBACK(text_buffer_changed), NULL);
	//g_signal_connect_after(G_OBJECT(text_buffer), "changed", G_CALLBACK(text_buffer_changed_after), NULL);

	{
		GtkTextIter i;
		gtk_text_buffer_get_start_iter(text_buffer, &i);
		gtk_text_buffer_create_mark(text_buffer, "scroll-mark", &i, TRUE);
		GtkAdjustment *adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(text_view));
		g_signal_connect(adj, "value-changed", G_CALLBACK(on_adjustment_value_changed), NULL);
	}

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
		GTK_WINDOW(app_window),
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
	char file_name_copy[100]; //@ buffer bounds
	char *base_name;
	char *curr_token, *prev_token;

	sprintf(file_name_copy, "%s", file_name); // ...since strtok() modifies the string given.
	curr_token = strtok(file_name_copy, "/"); //@ What if this returns NULL?
	while(curr_token != NULL) {
		prev_token = curr_token;
		curr_token = strtok(NULL, "/");
	}
	base_name = malloc(100); //@  buffer bounds
	sprintf(base_name, "%s", prev_token);

	return base_name;
}


#define CTRL	1 // 0001
#define ALT	2 // 0010
#define SHIFT	4 // 0100
#define ALTGR	8 // 1000

#define SIZE_KEYCODES 	65536 	// number of different values (2**16 (key_event->hardware_keycode is a 16-bit variable))
#define SIZE_MODIFIERS 	16			// CTRL + ALT + SHIFT + ALTGR + 1
//gboolean (*key_combinations[SIZE_MODIFIERS][SIZE_KEYCODES])(GdkEventKey *key_event); // global arrays should be initialized to defaults (NULL) (?)
#define MAX_HANDLERS 3 // maximally, how many handlers for a keycombination
/* these should be initialized to 0's by default: */
gboolean (*key_combinations[SIZE_MODIFIERS][SIZE_KEYCODES][MAX_HANDLERS])(GdkEventKey *key_event);

void add_keycombination_handler(
	int modifiers, int keycode, gboolean (*handler)(GdkEventKey *key_event))
{
	LOG_MSG("add_keycombination_handler(): modifiers: %d, keycode: %d\n",
		modifiers, keycode);
	int i = 0;
	while (key_combinations[modifiers][keycode][i] != NULL) {
		i += 1;
	}
	assert(i < MAX_HANDLERS);
	key_combinations[modifiers][keycode][i] = handler;
}


gboolean on_app_window_key_press(GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	#define NO_MODIFIERS 0x2000000 // Value of key_event->state if no (known) modifiers are set.

	GdkEventKey *key_event = (GdkEventKey *) event;
	printf("on_app_window_key_press(): hardware keycode: %d\n", key_event->hardware_keycode);

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
/*
	if(key_combinations[modifiers][key_event->hardware_keycode] != NULL) {
		if((*key_combinations[modifiers][key_event->hardware_keycode])(key_event)) {
			return TRUE; // We have dealt with the key-press and nothing else should happen!
		}
	}
*/
	for (int i = 0; key_combinations[modifiers][key_event->hardware_keycode][i] != NULL; ++i) {
		if((*key_combinations[modifiers][key_event->hardware_keycode][i])(key_event)) {
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


void refresh_application_title(void)
{
	printf("refresh_application_title()\n");

	#define BUFFER_SIZE 100
	char app_title[BUFFER_SIZE];
	snprintf(app_title, BUFFER_SIZE, "Root Directory: %s", root_dir);
	gtk_window_set_title(GTK_WINDOW(app_window), app_title);

/*
	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	if (!tab) {
		printf("\t-> no visible tab\n");
		return;
	}
	gpointer data = g_object_get_data(G_OBJECT(tab), "tab-info");
	assert(data != NULL);
	struct TabInfo *tab_info = (struct TabInfo *) data;
	if(tab_info->file_name != NULL) {
		//gtk_window_set_title(GTK_WINDOW(window), tab_info->file_name);
		snprintf(app_title, 100,
			"Root Directory: %s\tCurrent File: %s", root_dir, tab_info->file_name);
	} else {
		//gtk_window_set_title(GTK_WINDOW(window), tab_info->title);
		snprintf(app_title, 100,
			"Root Directory: %s\tCurrent File: %s", root_dir, tab_info->title);
	}
*/
}


void on_notebook_switch_page(GtkNotebook *notebook, GtkWidget *tab, guint page_num, gpointer data)
{
	LOG_MSG("on_notebook_switch_page()\n");

	GtkWidget *text_view = (GtkWidget *) tab_retrieve_widget(tab, TEXT_VIEW);
	assert(text_view);
	gtk_widget_grab_focus(text_view);
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
	if (gtk_widget_is_visible(notebook) == TRUE) {
		//gtk_widget_hide(notebook);
		gtk_widget_hide(nb_container);
	}
	else {
		//gtk_widget_show(notebook);
		gtk_widget_show(nb_container);
	}

	return TRUE;
}

/*
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
*/

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

		GtkLabel *filepath_label = tab_retrieve_widget(tab, FILEPATH_LABEL);
		gtk_label_set_text(filepath_label, file_name);
	}

	write_file(tab_info->file_name, contents); //@ error handling

	tab_set_unsaved_changes_to(tab, FALSE);

	/* autocomplete: we'll update the list of words during each save-op to be more up-to-date */
	struct StrList *words = autocomplete_create_and_store_words(text_buffer);
	void *old_words = tab_retrieve_widget(tab, AUTOCOMPLETE_WORDS);
	free(old_words);
	tab_add_widget_4_retrieval(tab, AUTOCOMPLETE_WORDS, (void *) words);

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

/* signature is such because we need register it as a callback */
gboolean apply_css_from_file(void *data)
{
	LOG_MSG("apply_css_from_file()\n");
	//const char *file_name = (const char *) data;
	const char *file_name = css_file_path;

	//printf("applying css from \"%s\"..\n", file_name);
	LOG_MSG("\tapplying css from \"%s\"..\n", file_name);
	// Apply css from file:
	static GtkCssProvider *provider = NULL;
	GdkScreen *screen = gdk_screen_get_default();
	if (provider != NULL) {
		LOG_MSG("\tremoving an old provider...\n");
		gtk_style_context_remove_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider));
	}
	LOG_MSG("\tcreating a new provider...\n");
	provider = gtk_css_provider_new();
	GFile *css_file = g_file_new_for_path(file_name);
	gtk_css_provider_load_from_file(provider, css_file, NULL);
	assert(screen != NULL); assert(provider != NULL);
	//printf("before gtk_style_context_add_provider_for_screen\n");
	gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	//gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_FALLBACK);
	//printf("after gtk_style_context_add_provider_for_screen\n");
	//gtk_widget_show_all(window);

	return FALSE; // Dont call again
}


void *watch_for_changes(void *data)
{
	LOG_MSG("watch_for_changes()\n");

	const char *file_name = (const char *) data;
	LOG_MSG("watch_for_changes(): watching for changes in: %s\n", file_name);

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

	//@ Probably should allocate more memory...
	char buffer[(sizeof(struct inotify_event) + NAME_MAX + 1)];

	while (1) {
		ssize_t bytes_read = read(inotify_descriptor, buffer, sizeof(buffer)); //@ can fail
		//printf("bytes read by read(): %ld\n", bytes_read);

		char *ptr = buffer;
		while (ptr < buffer + bytes_read) {
			struct inotify_event *event = (struct inotify_event *) ptr;

			if (event->mask & IN_MODIFY) {
				printf("file modified!\n");
/*
Calling apply_css_from_file() directly crashes the app when modifying the css-file using another editor process. It doesnt happen always though, so it might be necessary to modify the css-file multiple times.
*/
				//apply_css_from_file(file_name); 
				g_timeout_add_seconds(1, apply_css_from_file, data);
			}
			ptr += sizeof(struct inotify_event) + event->len;
		}
	}
}


//void parse_settings_file(const char *file_path)
gboolean parse_settings_file(void *data)
{
	LOG_MSG("parse_settings_file()\n");

	//char *contents = read_file(file_path);
	char *contents = read_file(settings_file_path);
	//printf("parse_settings_file(): contents:\n%s\n", contents);

	#define SIZE 100
	char *line, *name, *value, copy[SIZE];
	while (line = get_slice_by(&contents, '\n')) {
		//printf("line: %s\n", line);

		snprintf(copy, SIZE, "%s", line);
		name = get_slice_by(&line, ':');
		value = get_slice_by(&line, ':');

		if (!name || !value) {
			printf("parse_settings_file(): unrecognized line format: \"%s\"\n", copy);
			continue;
		}
		//printf("name: %s, value: %s\n", name, value);

		if (strcmp(name, "line-highlight") == 0) {
			snprintf(settings.line_highlight_color, SETTING_VALUE_MAX, "%s", value);
		} else if (strcmp(name, "comment") == 0) {
			snprintf(settings.comment_color, SETTING_VALUE_MAX, "%s", value);
		} else if (strcmp(name, "string") == 0) {
			snprintf(settings.string_color, SETTING_VALUE_MAX, "%s", value);
		} else if (strcmp(name, "identifier") == 0) {
			snprintf(settings.identifier_color, SETTING_VALUE_MAX, "%s", value);
		} else if (strcmp(name, "number") == 0) {
			snprintf(settings.number_color, SETTING_VALUE_MAX, "%s", value);
		} else if (strcmp(name, "operator") == 0) {
			snprintf(settings.operator_color, SETTING_VALUE_MAX, "%s", value);
		} else if (strcmp(name, "keyword") == 0) {
			snprintf(settings.keyword_color, SETTING_VALUE_MAX, "%s", value);
		} else if (strcmp(name, "type") == 0) {
			snprintf(settings.type_color, SETTING_VALUE_MAX, "%s", value);
		} else if (strcmp(name, "preproccessor") == 0) {
			snprintf(settings.preproccessor_color, SETTING_VALUE_MAX, "%s", value);
		} else if (strcmp(name, "unknown") == 0) {
			snprintf(settings.unknown_color, SETTING_VALUE_MAX, "%s", value);
		} else if (strcmp(name, "pixels-above-lines") == 0) {
			settings.pixels_above_lines = atoi(value);
		} else if (strcmp(name, "pixels-below-lines") == 0) {
			settings.pixels_below_lines = atoi(value);
		} else if (strcmp(name, "left-margin") == 0) {
			settings.left_margin = atoi(value);
		} else {
			printf("parse_settings_file(): unknown name: %s\n", name);
		}
	}

	return FALSE;
}

gboolean handler1(GdkEventKey *key_event)
{
	printf("handler1 called!\n");
	return FALSE;
}

gboolean handler2(GdkEventKey *key_event)
{
	printf("handler2 called!\n");
	return FALSE;
}

gboolean handler3(GdkEventKey *key_event)
{
	printf("handler3 called!\n");
	return FALSE;
}

gboolean handler4(GdkEventKey *key_event)
{
	printf("handler4 called!\n");
	return FALSE;
}


gboolean scroll_to_cursor_middle(GdkEventKey *key_event)
{
	LOG_MSG("scroll_to_cursor()\n");

	GtkTextView *text_view = (GtkTextView *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW);
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);

	GtkTextIter pos;
	get_cursor_position(text_buffer, NULL, &pos, NULL);
	gtk_text_view_scroll_to_iter(text_view, &pos,
		0.0,
		TRUE, // use alignment?
		0.0, // x-alignment
		0.5); // y-alignment (in the middle of the screen)

	return TRUE;
}


gboolean scroll_to_cursor_top(GdkEventKey *key_event)
{
	LOG_MSG("scroll_to_cursor()\n");

	GtkTextView *text_view = (GtkTextView *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW);
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);

	GtkTextIter pos;
	get_cursor_position(text_buffer, NULL, &pos, NULL);
	gtk_text_view_scroll_to_iter(text_view, &pos,
		0.0,
		TRUE, // use alignment?
		0.0, // x-alignment
		0.0); // y-alignment (in the middle of the screen)

	return TRUE;
}


gboolean scroll_to_cursor_bottom(GdkEventKey *key_event)
{
	LOG_MSG("scroll_to_cursor()\n");

	GtkTextView *text_view = (GtkTextView *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW);
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);

	GtkTextIter pos;
	get_cursor_position(text_buffer, NULL, &pos, NULL);
	gtk_text_view_scroll_to_iter(text_view, &pos,
		0.0,
		TRUE, // use alignment?
		0.0, // x-alignment
		1.0); // y-alignment (in the middle of the screen)

	return TRUE;
}


gboolean scroll_to_start(GdkEventKey *key_event)
{
	LOG_MSG("scroll_to_cursor()\n");

	GtkTextView *text_view = (GtkTextView *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW);
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);

	GtkTextIter pos;
	gtk_text_buffer_get_start_iter(text_buffer, &pos);
	gtk_text_view_scroll_to_iter(text_view, &pos,
		0.0,
		TRUE, // use alignment?
		0.0, // x-alignment
		0.0); // y-alignment

	return TRUE;
}


gboolean scroll_to_end(GdkEventKey *key_event)
{
	LOG_MSG("scroll_to_cursor()\n");

	GtkTextView *text_view = (GtkTextView *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW);
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);

	GtkTextIter pos;
	gtk_text_buffer_get_end_iter(text_buffer, &pos);
	gtk_text_view_scroll_to_iter(text_view, &pos,
		0.0,
		TRUE, // use alignment?
		0.0, // x-alignment
		1.0); // y-alignment

	return TRUE;
}


gboolean scroll_up(GdkEventKey *key_event)
{
	LOG_MSG("scroll_up()\n");

	GtkTextView *text_view = (GtkTextView *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW);
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);

	GtkTextIter i;
	GtkTextMark *m = gtk_text_buffer_get_mark(text_buffer, "scroll-mark");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &i, m);
	gtk_text_iter_backward_lines(&i, 10);
	gtk_text_view_scroll_to_iter(text_view, &i, 0.0, TRUE, 0.0, 0.0);

	return TRUE;
}


gboolean scroll_down(GdkEventKey *key_event)
{
	LOG_MSG("scroll_down()\n");

	GtkTextView *text_view = (GtkTextView *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW);
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);

	GtkTextIter i;
	GtkTextMark *m = gtk_text_buffer_get_mark(text_buffer, "scroll-mark");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &i, m);
	gtk_text_iter_forward_lines(&i, 10);
	gtk_text_view_scroll_to_iter(text_view, &i, 0.0, TRUE, 0.0, 0.0);

	return TRUE;
}


void activate_handler(GtkApplication *app, gpointer data)
{
	LOG_MSG("activate_handler() called\n");

/*
Cant call set_root_dir() here because it expects file-browser and root-navigation to be already created.
If we used some kind of event/signal-thing, which allows abstractions to register callbacks to be executed in response to events like "root-directory-change" we wouldnt have to worry about that. Because then the code that individual-abstractions need to run would be provided by them in the form of callbacks and wouldnt be hardcoded into set_root_dir function.
*/

	char *home_dir = getenv("HOME");
	assert(home_dir);
	//printf("home directory: %s\n", home_dir);
	snprintf(root_dir, ROOT_DIR_SIZE, "%s", home_dir);

/*
	add_keycombination_handler(CTRL | ALT, 42, handler1); // ctrl + alt + g
	add_keycombination_handler(CTRL | ALT, 42, handler2);
	add_keycombination_handler(CTRL | ALT, 42, handler3);
	//add_keycombination_handler(CTRL | ALT, 42, handler4);
*/

	/*
		84 - numpad "5"
		80 - numpad up
		88 - numpad down
		79 - numpad home
		87 - numpad end
		81 - numpad page-up
		89 - numpad page-down
	*/

	add_keycombination_handler(0, 84, scroll_to_cursor_middle);
	add_keycombination_handler(0, 80, scroll_to_cursor_top);
	add_keycombination_handler(0, 88, scroll_to_cursor_bottom);
	add_keycombination_handler(0, 79, scroll_to_start);
	add_keycombination_handler(0, 87, scroll_to_end);
	add_keycombination_handler(0, 81, scroll_up); // to do. scroll without moving the cursor is the idea
	add_keycombination_handler(0, 89, scroll_down); // to do

	//key_combinations[0][23] = handle_tab; // <tab>
	add_keycombination_handler(0, 23, handle_tab);
	//key_combinations[SHIFT][23] = handle_tab; // <tab> + shift
	add_keycombination_handler(SHIFT, 23, handle_tab);
	add_keycombination_handler(0, 36, do_search);// <enter>

	add_keycombination_handler(0, 9, autocomplete_close_popup); // escape
	add_keycombination_handler(0, 111, autocomplete_upkey); // up
	add_keycombination_handler(0, 116, autocomplete_downkey); // down
	add_keycombination_handler(0, 36, do_autocomplete); // enter
	add_keycombination_handler(0, 23, do_autocomplete); // tab

	//@ cursors blink is off for move_cursor_left() & move_cursor_right()
	// also comments & identifiers -- not very convenient
/*
	key_combinations[CTRL][113] = move_cursor_left; // ctrl + <left arrow>
	key_combinations[CTRL][114] = move_cursor_right; // ctrl + <right arrow>
	key_combinations[SHIFT|CTRL][113] = move_cursor_left;
	key_combinations[SHIFT|CTRL][114] = move_cursor_right;
*/

	//key_combinations[CTRL][111] = move_cursor_up; // ctrl + <up>
	add_keycombination_handler(CTRL, 111, move_cursor_up);
	//key_combinations[CTRL][116] = move_cursor_down; // ctrl + <down>
	add_keycombination_handler(CTRL, 116, move_cursor_down);

	//key_combinations[CTRL][47] = move_cursor_start_line; // ctrl + ö
	add_keycombination_handler(CTRL, 47, move_cursor_start_line);
	//key_combinations[CTRL][48] = move_cursor_end_line; // ctrl + ä
	add_keycombination_handler(CTRL, 48, move_cursor_end_line);

	//key_combinations[ALT][111] = move_lines_up; // alt + <up arrow>
	add_keycombination_handler(ALT, 111, move_lines_up);
	//key_combinations[ALT][116] = move_lines_down; // alt + <down arrow>
	add_keycombination_handler(ALT, 116, move_lines_down);
	//key_combinations[ALT][113] = move_token_left; // alt + <left arrow>
	add_keycombination_handler(ALT, 113, move_token_left);
	//key_combinations[ALT][114] = move_token_right; // alt + <right arrow>
	add_keycombination_handler(ALT, 114, move_token_right);
	//key_combinations[ALT][35] = insert_line_before; // alt + õ (35)
	add_keycombination_handler(ALT, 35, insert_line_before);
	//key_combinations[ALT][51] = insert_line_after; // alt + ' (51)
	add_keycombination_handler(ALT, 51, insert_line_after);
	//key_combinations[ALT][40] = duplicate_line; // alt + d
	add_keycombination_handler(ALT, 40, duplicate_line);
	//key_combinations[ALT][119] = delete_line; // alt + <delete>
	add_keycombination_handler(ALT, 119, delete_line);
	//key_combinations[ALT][34] = change_line; // alt + ü
	add_keycombination_handler(ALT, 34, change_line);
	//key_combinations[ALT][33] = delete_end_of_line; // alt + p
	add_keycombination_handler(ALT, 33, delete_end_of_line);

	//key_combinations[CTRL][52] = undo_last_action; // ctrl + z
	add_keycombination_handler(CTRL, 52, undo_last_action);

	//key_combinations[CTRL][57] = create_empty_tab; // ctrl + n
	add_keycombination_handler(CTRL, 57, create_empty_tab);
	//key_combinations[CTRL][58] = close_tab; // ctrl + m
	add_keycombination_handler(CTRL, 58, close_tab);
	//key_combinations[CTRL][45] = switch_tab; // ctrl + k
	add_keycombination_handler(CTRL, 45, switch_tab);

	//key_combinations[CTRL][39] = do_save; // ctrl + s
	add_keycombination_handler(CTRL, 39, do_save);
	//key_combinations[CTRL][32] = do_open; // ctrl + o
	add_keycombination_handler(CTRL, 32, do_open);

	//key_combinations[CTRL][41] = toggle_search_entry; // ctrl + f
	add_keycombination_handler(CTRL, 41, toggle_search_entry);
	//key_combinations[CTRL][27] = toggle_replace_entry; // ctrl + r
	//key_combinations[CTRL][43] = display_openfile_dialog; // ctrl + h
	add_keycombination_handler(CTRL, 43, display_openfile_dialog);

	//key_combinations[CTRL][42] = less_fancy_toggle_sidebar; // ctrl + g
	add_keycombination_handler(CTRL, 42, less_fancy_toggle_sidebar);
	//key_combinations[CTRL][44] = less_fancy_toggle_notebook; // ctrl + j
	add_keycombination_handler(CTRL, 44, less_fancy_toggle_notebook);



	//parse_settings_file(settings_file_path);
	parse_settings_file(NULL);
	apply_css_from_file((void *) css_file_path);
/*
	pthread_t id;
	int r = pthread_create(&id, NULL, watch_for_changes, css_file_path);
	if (r != 0) {
		printf("pthread_create() error!\n");
	}
*/

	hotloader_register_callback(css_file_path, apply_css_from_file);
	hotloader_register_callback(settings_file_path, parse_settings_file);

	/*uid_t real_uid = getuid();
	uid_t effective_uid = geteuid();
	printf("real uid: %d, effective uid: %d\n", real_uid, effective_uid);*/


	GtkWidget *sidebar_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(sidebar_notebook), GTK_POS_BOTTOM);
	add_class(sidebar_notebook, "sidebar-notebook");

	file_browser = create_filebrowser_widget();
	GtkWidget *file_browser_scrollbars = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(file_browser_scrollbars), file_browser);
	GtkWidget *filebrowser_icon = gtk_image_new_from_file(filebrowser_icon_path);
	gtk_widget_set_tooltip_text(filebrowser_icon, "File Browser");
	gtk_notebook_append_page(GTK_NOTEBOOK(sidebar_notebook), file_browser_scrollbars, filebrowser_icon);

	GtkWidget *search_in_files = create_search_in_files_widget();
	GtkWidget *searchinfiles_icon = gtk_image_new_from_file(searchinfiles_icon_path);
	gtk_widget_set_tooltip_text(searchinfiles_icon, "Search in Files");
	gtk_notebook_append_page(GTK_NOTEBOOK(sidebar_notebook), search_in_files, searchinfiles_icon);

	GtkWidget *root_nav = create_root_nav_widget();

	sidebar_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(sidebar_container), root_nav);
	gtk_container_add(GTK_CONTAINER(sidebar_container), sidebar_notebook);

	gtk_widget_set_vexpand(sidebar_notebook, TRUE);
	gtk_widget_set_hexpand(sidebar_notebook, TRUE);
	gtk_widget_set_size_request(sidebar_container, 500, 100);


	notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	g_signal_connect_after(notebook,
		"switch-page", G_CALLBACK(on_notebook_switch_page), NULL);
	add_class(notebook, "main-notebook");
	gtk_widget_set_hexpand(notebook, TRUE);

	nb_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(nb_container), notebook);


	app_window = gtk_application_window_new(app);
	//gtk_window_set_title(GTK_WINDOW(window), "Hello world!");
	gtk_window_set_default_size(GTK_WINDOW(app_window), 1300, 600);

	add_class(app_window, "app-window");
	//gtk_widget_set_name(window, "app-window");

	g_signal_connect(app_window, "key-press-event",
						G_CALLBACK(on_app_window_key_press), NULL);
/*
	// thats a double-click:
	g_signal_connect(app_window, "button-press-event",
		G_CALLBACK(autocomplete_on_appwindow_button_pressed), NULL);
*/

	autocomplete_init(GTK_NOTEBOOK(notebook), GTK_APPLICATION_WINDOW(app_window));

	GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_add1(GTK_PANED(paned), sidebar_container);
	//gtk_paned_add2(GTK_PANED(paned), notebook);
	gtk_paned_add2(GTK_PANED(paned), nb_container);
	gtk_container_add(GTK_CONTAINER(app_window), paned);

	gtk_widget_show_all(app_window);

	gtk_widget_hide(sidebar_container); // let's make sidebar hidden at startup

	create_tab(NULL); // re-factor: create_tab() could just create the tab widget, it doesnt have to depend on the notebook at all (?)

	/*GtkWidget *popup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_decorated(GTK_WINDOW(popup_window), FALSE);
	gtk_window_set_position(GTK_WINDOW(popup_window), GTK_WIN_POS_CENTER);
	gtk_window_set_transient_for(GTK_WINDOW(popup_window), GTK_WINDOW(window));
	gtk_widget_show_all(popup_window);*/
}


int main()
{
	LOG_MSG("main()\n");

	int status;
	GtkApplication *app;

	//test_str_replace();
	test_parse_str();

	guint major = gtk_get_major_version();
	guint minor = gtk_get_minor_version();
	guint micro = gtk_get_micro_version();
	printf("GTK version: %u.%u.%u\n", major, minor, micro);

	app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app,
		"activate", G_CALLBACK(activate_handler), NULL);

	status = g_application_run(G_APPLICATION(app), 0, NULL);

	//g_object_unref(app);
	return status;
}