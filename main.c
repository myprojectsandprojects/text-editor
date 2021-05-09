
//@ bug: cant type single-quotes inside the search-entry

#include <gtk/gtk.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <limits.h> // for NAME_MAX

#include "tab.h"

//void apply_settings(GtkWidget *widget);
char *read_file(const char *filename);
void write_file(const char *filename, const char *contents); // @ can fail
//void apply_style(GtkWidget *widget);
char *get_base_name(const char *file_name);
void refresh_application_title(GtkWidget *tab);
//void on_text_buffer_delete_range(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end, gpointer data);
//void on_text_buffer_insert_text(GtkTextBuffer *text_buffer, GtkTextIter *location, char *text, int len, gpointer user_data);

void init_search(GtkWidget *tab);
void init_undo(GtkWidget *tab);
void actually_undo_last_action(GtkWidget *tab);

//const char *settings_file = "settings";

GtkWidget *window;
//GtkWidget *stack;
GtkWidget *notebook;
GtkWidget *sidebar_revealer;

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
	g_print("buffer changed!\n");

	//$ GtkWidget *tab = gtk_stack_get_visible_child(GTK_STACK(stack)); // Should really use text_buffer I think!
	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	if (page == -1) {
		// buffers contents changed, but we have no "current page"... is this possible?
		return;
	}
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);

	struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	if (tab_info->unsaved_changes == FALSE)
		tab_set_unsaved_changes_to(tab, TRUE);

	// Make sure the title reflects the fact that the tab has unsaved changes.
	//$ refresh_tab_title(tab, TRUE); // @performance?

	/*GtkTextIter start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
	highlight(text_buffer, &start_buffer, &end_buffer);*/
}

void text_buffer_cursor_position_changed(GObject *text_buffer, GParamSpec *pspec, gpointer user_data)
{
	int position, line_number;
	g_object_get(G_OBJECT(text_buffer), "cursor-position", &position, NULL);
	GtkTextIter i;
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(text_buffer), &i, position);
	line_number = gtk_text_iter_get_line(&i);

	char buffer[100];
	sprintf(buffer, "%d", line_number + 1);
	GtkLabel *label = (GtkLabel *) user_data;
	gtk_label_set_text(label, buffer);

	/*GtkTextIter start, end, start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
	gtk_text_buffer_remove_tag_by_name(text_buffer, "line-highlight", &start_buffer, &end_buffer);
	gtk_text_iter_set_line_offset(&i, 0);
	start = i;
	gtk_text_iter_forward_line(&i);
	gtk_text_iter_forward_char(&i);
	end = i;
	printf("applying the tag: %d, %d\n", gtk_text_iter_get_offset(&start), gtk_text_iter_get_offset(&end));
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
	GtkTextBuffer *text_buffer;
	if ((text_buffer = get_visible_text_buffer(GTK_NOTEBOOK(notebook))) == NULL) {
		return; // No tabs, nothing to do..
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

void create_tab(const char *file_name)
{
	static int count = 1;
	GtkWidget *tab, *scrolled_window;
	char *tab_title, *tab_name;
	gchar *contents, *base_name;
	GFile *file;

	g_print("create_tab()\n");

	tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_hexpand(tab, TRUE);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_vexpand(scrolled_window, TRUE);

	GtkTextView *text_view = GTK_TEXT_VIEW(gtk_text_view_new());
	gtk_text_view_set_pixels_above_lines(text_view, 1);
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
	gtk_widget_set_name(command_revealer, "command-revealer");
	GtkWidget *command_entry = gtk_entry_new();
	gtk_style_context_add_class(gtk_widget_get_style_context(command_entry), "command-entry");

	GtkWidget *info_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *line_number_label = gtk_label_new("Line ");
	GtkWidget *line_number_value = gtk_label_new(NULL);
	GtkWidget *fill = gtk_label_new(NULL);
	gtk_widget_set_hexpand(fill, TRUE);

	/*gtk_widget_set_name(line_number_label, "line_number_label");
	gtk_widget_set_name(line_number_value, "line_number_value");
	gtk_widget_set_name(line_number_value, "fill");*/
	gtk_style_context_add_class (gtk_widget_get_style_context(line_number_label), "info-bar");
	gtk_style_context_add_class (gtk_widget_get_style_context(line_number_value), "info-bar-emphasis");
	gtk_style_context_add_class (gtk_widget_get_style_context(fill), "info-bar");

	gtk_container_add(GTK_CONTAINER(tab), search_revealer);
	gtk_container_add(GTK_CONTAINER(search_revealer), search_entry);
	gtk_container_add(GTK_CONTAINER(tab), scrolled_window);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(text_view));
	gtk_container_add(GTK_CONTAINER(tab), command_revealer);
	gtk_container_add(GTK_CONTAINER(command_revealer), command_entry);
	gtk_container_add(GTK_CONTAINER(tab), info_bar);
	gtk_container_add(GTK_CONTAINER(info_bar), line_number_label);
	gtk_container_add(GTK_CONTAINER(info_bar), line_number_value);
	gtk_container_add(GTK_CONTAINER(info_bar), fill);

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


	//$ gtk_stack_add_titled(GTK_STACK(stack), tab, tab_name, tab_title);
	int page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab, NULL);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);

	tab_set_unsaved_changes_to(tab, FALSE);

	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);

	if(file_name != NULL) {
		char *contents = read_file(file_name); //@ error handling. if we cant open a file, we shouldnt create a tab?
		gtk_text_buffer_set_text(text_buffer, contents, -1);
		free(contents);
	}

	//add_highlighting(text_buffer);
	init_highlighting(text_buffer);

	init_search(tab);
	init_undo(tab);

	//$ gtk_stack_set_visible_child(GTK_STACK(stack), tab);

	g_signal_connect(G_OBJECT(text_view), "copy-clipboard", G_CALLBACK(text_view_copy_clipboard), NULL);
	g_signal_connect(G_OBJECT(text_view), "cut-clipboard", G_CALLBACK(text_view_cut_clipboard), NULL);
	g_signal_connect(G_OBJECT(text_view), "paste-clipboard", G_CALLBACK(text_view_paste_clipboard), NULL);

	g_signal_connect(G_OBJECT(text_view), "populate-popup", G_CALLBACK(on_text_view_populate_popup), NULL);

	//gtk_text_buffer_create_tag(text_buffer, "line-highlight", "background", "red", "background-set", TRUE, NULL);
	g_signal_connect(G_OBJECT(text_buffer), "notify::cursor-position", G_CALLBACK(text_buffer_cursor_position_changed), line_number_value);

	g_signal_connect(G_OBJECT(text_buffer), "changed", G_CALLBACK(text_buffer_changed), NULL);
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
	refresh_application_title(tab);
}

gboolean on_notebook_focus_in_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	g_print("on_notebook_focus_in_event!\n");

	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	if (page == -1) {
		// no tabs open
		return FALSE;
	}
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);

	GtkTextView *text_view = tab_get_text_view(tab);
	gtk_widget_grab_focus(GTK_WIDGET(text_view)); // @doesnt work!
}


// Why not just call editing functions directly?
gboolean autocomplete_character(GdkEventKey *key_event)
{
	GtkTextBuffer *text_buffer;
	if ((text_buffer = get_visible_text_buffer(GTK_NOTEBOOK(notebook))) == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

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
	if ((text_buffer = get_visible_text_buffer(GTK_NOTEBOOK(notebook))) == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	actually_open_line_before(text_buffer);
}

gboolean open_line_after(GdkEventKey *key_event)
{
	GtkTextBuffer *text_buffer;
	if ((text_buffer = get_visible_text_buffer(GTK_NOTEBOOK(notebook))) == NULL) {
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

gboolean toggle_sidebar(GdkEventKey *key_event)
{
	if(gtk_revealer_get_reveal_child(GTK_REVEALER(sidebar_revealer)) == TRUE) { // If revealed:
		gtk_revealer_set_reveal_child(GTK_REVEALER(sidebar_revealer), FALSE);
	} else { // If not revealed:
		gtk_revealer_set_reveal_child(GTK_REVEALER(sidebar_revealer), TRUE);
	}
	return TRUE;
}

gboolean toggle_search_entry(GdkEventKey *key_event)
{
	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	if (page == -1) {
		// no tabs open
		return FALSE;
	}
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);

	GtkRevealer *search_revealer = tab_get_search_revealer(tab);
	GtkWidget *search_entry = gtk_bin_get_child(GTK_BIN(search_revealer));
	GtkTextView *text_view = tab_get_text_view(tab);

	if(gtk_revealer_get_reveal_child(search_revealer) == FALSE) { // not revealed
		gtk_revealer_set_reveal_child(search_revealer, TRUE);
		gtk_widget_grab_focus(search_entry);
	} else if(gtk_widget_is_focus(GTK_WIDGET(search_entry))) { // revealed
		gtk_revealer_set_reveal_child(search_revealer, FALSE);
		gtk_widget_grab_focus(GTK_WIDGET(text_view));
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

	GtkRevealer *command_revealer = tab_get_command_revealer(tab);
	GtkWidget *command_entry = gtk_bin_get_child(GTK_BIN(command_revealer));
	GtkTextView *text_view = tab_get_text_view(tab);

	if(gtk_revealer_get_reveal_child(command_revealer) == FALSE) { // not revealed
		gtk_revealer_set_reveal_child(command_revealer, TRUE);
		gtk_widget_grab_focus(command_entry);
	} else if(gtk_widget_is_focus(GTK_WIDGET(command_entry))) { // revealed
		gtk_revealer_set_reveal_child(command_revealer, FALSE);
		gtk_widget_grab_focus(GTK_WIDGET(text_view));
	} else {
		gtk_widget_grab_focus(command_entry);
	}

	return TRUE;
}

gboolean handle_tab(GdkEventKey *key_event)
{
	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	if (page == -1) {
		// no tabs open
		return FALSE;
	}
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);

	if(handle_tab_key(tab, key_event) == TRUE) return TRUE; // We handled the tab...
	return FALSE; // Otherwise, we dont know...
}

gboolean handle_enter(GdkEventKey *key_event)
{
	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	if (page == -1) {
		// no tabs open
		return FALSE;
	}
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);

	//$$
	GtkRevealer *search_revealer = tab_get_search_revealer(tab);
	GtkWidget *search_entry = gtk_bin_get_child(GTK_BIN(search_revealer));
	if(gtk_widget_is_focus(search_entry) == TRUE) {
		do_search(tab);
		return TRUE;
	}

	GtkRevealer *command_revealer = tab_get_command_revealer(tab);
	GtkWidget *command_entry = gtk_bin_get_child(GTK_BIN(command_revealer));
	if(gtk_widget_is_focus(command_entry) == TRUE) {
		const char *text = gtk_entry_get_text(GTK_ENTRY(command_entry)); //@ free?
		if(strlen(text) == 0) {
			return TRUE;
		}
		GtkTextView *text_view = tab_get_text_view(tab);
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
		return FALSE;
	}
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);

	g_print("SHOULD UNDO LAST ACTION\n");
	actually_undo_last_action(tab);

	return TRUE;
}

gboolean do_save(GdkEventKey *key_event)
{
	int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	if (page == -1) {
		// no tabs open
		return FALSE;
	}

	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);
	GtkTextView *text_view = tab_get_text_view(tab);
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
	gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
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

	g_print("activate_handler() called\n");


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

	gboolean move_lines_up(GdkEventKey *key_event);
	gboolean move_lines_down(GdkEventKey *key_event);
	gboolean duplicate_line(GdkEventKey *key_event);
	key_combinations[ALT][111] = move_lines_up; // alt + <up arrow>
	key_combinations[ALT][116] = move_lines_down; // alt + <down arrow>
	key_combinations[ALT][40] = duplicate_line; // alt + d
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

	key_combinations[CTRL][39] = do_save; // ctrl + s
	key_combinations[CTRL][32] = do_open; // ctrl + o

	key_combinations[CTRL][41] = toggle_search_entry; // ctrl + f
	key_combinations[CTRL][42] = toggle_sidebar; // ctrl + g
	key_combinations[CTRL][43] = toggle_command_entry; // ctrl + h


	char *css_file = "css";
	apply_css_from_file(css_file);

	pthread_t id;
	int r = pthread_create(&id, NULL, watch_for_changes, css_file);
	if (r != 0) {
		printf("pthread_create() error!\n");
	}

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Hello world!");
	gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);

	GtkWidget *container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	//$ GtkWidget *header_bar = gtk_header_bar_new();

	GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	sidebar_revealer = gtk_revealer_new();
	//gtk_revealer_set_reveal_child(GTK_REVEALER(sidebar_revealer), FALSE);
	gtk_revealer_set_transition_type(GTK_REVEALER(sidebar_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);

	init_file_browser(sidebar_revealer);
	//GtkWidget *sidebar_placeholder = gtk_label_new("sidebar");


	/*$ stack = gtk_stack_new();
	g_signal_connect(stack, "notify::visible-child", G_CALLBACK(stack_visible_child_changed), NULL);

	GtkWidget *stack_switcher = gtk_stack_switcher_new();
	gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(stack_switcher), GTK_STACK(stack));*/

	notebook = gtk_notebook_new();
	g_signal_connect(notebook, "switch-page", G_CALLBACK(on_notebook_switch_page), NULL);
	g_signal_connect(notebook, "focus-in-event", G_CALLBACK(on_notebook_focus_in_event), NULL);


	g_signal_connect(window, "key-press-event", G_CALLBACK(on_window_key_press_event), NULL);

	gtk_container_add(GTK_CONTAINER(window), container);
	gtk_container_add(GTK_CONTAINER(container), content);
	gtk_container_add(GTK_CONTAINER(content), sidebar_revealer);
	//gtk_container_add(GTK_CONTAINER(sidebar_revealer), sidebar_placeholder);
	gtk_container_add(GTK_CONTAINER(content), notebook);

	create_tab(NULL);

	gtk_widget_show_all(window);
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





