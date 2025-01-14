#include <gtk/gtk.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <limits.h> // for NAME_MAX
#include <ctype.h> // just for testing isalnum()

#include "declarations.h"

#define LIB_INCLUDE_IMPLEMENTATION
#include "lib/lib.hpp"
#include "lib/linux_lib.hpp"

guint gtk_version_major;
guint gtk_version_minor;
guint gtk_version_micro;

// It seems to me, and I might be mistaken about this,
// that GTK *never* deallocates already allocated memory,
// but it does get reused when possible.
// So its not neccessarily a sign of a memory leak
// when memory usage climbs 100MiB+ if you open a large # of tabs for example
// and does not drop after you close them.

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

const char *filebrowser_icon_path = "themes/icons/files.png";
const char *searchinfiles_icon_path = "themes/icons/search.png";
const char *unsaved_changes_icon_path = "themes/icons/exclamation-mark.png";
const char *file_icon_path = "themes/icons/file.png";
const char *folder_icon_path = "themes/icons/folder.png";
const char *home_icon_path = "themes/icons/house.png";
const char *parent_dir_icon_path = "themes/icons/arrow.png";
const char *search_icon_path = "themes/icons/search.png";

//const char *settings_file_path = "/home/eero/all/text-editor/themes/settings";
const char *settings_file_path = "themes/settings";
const char *settings_file_path_ifnotheme = "themes/settings-no-theme"; // if we dont style widgets because we are unsure if the GTK version we are currently using supports our style, we take settings from this file.
//const char *css_file_path = "/home/eero/all/text-editor/themes/style.css";
//const char *css_file_path = "themes/style-3.18.9.css";
//bool use_theme = false;

struct Node *settings;

/*
To refer to a specific page
- we cant use a memory address if pages are stored in a dynamic array
- we cant use an index if we expect pages to be reordered (we dont currently)
so referring to a page by using a unique name (id) seems like a only solid solution
*/
int currentPageIndex = -1; // 0
array<NotebookPage> notebookPages;
static int firstUnusedNotebookPageId; // 0

/*@@ settings could change at any time!!!*/

// currently we have (at least) 4 different ways of bookkeeping per tab data
// - TabInfo
// - tab_add_widget_4_retrieval()
// - undo has it's own bookkeeping
// - then we have some CList of tabs for whatever reason
// we should get to a point where we only use something simple like this

/*
- access currently active (visible) page's data
- iterate over pages (when applying changed settings)
- go from a pointer to a text view to an id of a page to which the text view belongs to?

need to keep track of what's closed
we dont necessarily want to reuse page id's because I can imagine it causing confusion in some cirmustances
*/

// Its messy to mix values from variables into these messages. Can we improve?
//void display_error(const char *primary_message, const char *secondary_message = NULL){
//	char message[1000]; //@ overflow
//
//	if(secondary_message){
//		snprintf(message, 1000, "\n\033[1;31mError: %s\033[0m\n%s\n\n", primary_message, secondary_message);
//	}else{
//		snprintf(message, 1000, "\n\033[1;31m%s\033[0m\n\n", primary_message);
//	}
//
//	fprintf(stderr, "%s", message);
//}

//const int MAX_TEST_MARKS = 10000;
//GtkTextMark *test_marks[MAX_TEST_MARKS];

const int NUM_ASCII_CHARS = 128; // only ' '(32) ... '~'(126) are printable characters
//bool ascii_chars_plain[NUM_ASCII_CHARS];
bool ascii_chars[NUM_ASCII_CHARS];// use character's ascii value as an index to determine if character belongs to a class of characters that form words
// For a shorter jump (as I imagine it) it's not enough to use a different lookup table. camelCase and PascalCase the words begin with an uppercase, it's a different logic.

struct Node *get_node(struct Node *root, const char *apath) {
	struct Node *result = NULL;
	char *path = strdup(apath);
	char *copy = path;
	//printf("path: %s\n", path);

	struct Node *node = root;
	char *segment;
	while ((segment = get_slice_by(&path, '/')) != NULL) {
		//printf("segment: %s\n", segment);
		bool found = false;
		struct Node *n;
		for (int i = 0; i < node->nodes.Count; ++i) {
			n = (struct Node *) node->nodes.Data[i];
			//printf("looking at: %s\n", n->name);
			if (strcmp(n->name, segment) == 0) {
				found = true;
				break;
			}
		}
		if (found) {
			//printf("found node: %s\n", segment);
			node = n;
			result = n; //@ khm
			continue;
		} else {
			result = NULL;
			break;
		}
	}

	free(copy);

	return result;
}

// return value belongs to the node!
const char *settings_get_value(struct Node *settings, const char *path) {
	const char *result = NULL;
	struct Node *node = get_node(settings, path);
	if (node) {
		result = ((struct Node *) node->nodes.Data[0])->name; // first child-node stores the value as its name
	}
	return result;
}

void print_node(struct Node *node, int depth) {
	char indent[100];
	int i;

	for (i = 0; i < depth; ++i) {
		//indent[i] = '\t';
		indent[i] = ' ';
	}
	indent[i] = '\0';

	printf("%s%s\n", indent, node->name);

	for (int i = 0; i < node->nodes.Count; ++i) {
		print_node((struct Node *) node->nodes.Data[i], depth + 1);
	}
}

void print_nodes(struct Node *node) {
	print_node(node, 0);
}

struct Node *parse_settings_file(const char *file_path)
{
	LOG_MSG("parse_settings_file()\n");

//	char *contents = read_file(file_path);
//	if(!contents){
//		ERROR("Error: Cant read settings file: \"%s\" (We cant continue. Exiting.)", file_path);
//		exit(1);
//	}
////	assert(contents);
////	printf("contents:\n%s\n", contents);
//
//	INFO("Using settings from \"%s\"", file_path);

	char *contents;
	if(!Lib::ReadTextFile(file_path, &contents)){
		ERROR("Error: Cant read settings file: \"%s\" (We cant continue. Exiting.)", file_path);
		exit(1);
	}

	INFO("Using settings from \"%s\"", file_path);

	int start_comment, end_comment;
	start_comment = -1;
	end_comment = -1;
	#define SIZE 100
	char buffer[SIZE];
	int index = 0;

	struct Node *node_path[100];
	int path_index = 0;

	struct Node *root = (struct Node *) malloc(sizeof(struct Node));
	root->name = "Root";
//	root->nodes = new_list();
	ArrayInit(&(root->nodes));

	node_path[path_index++] = root;

	for (int i = 0; contents[i] != 0; ++i) {

		// ignore comments
		if (contents[i] == '/') {
			i += 1;
			if (contents[i] == '*') {
				// we have begin comment
				start_comment = i - 1;
				while (true) { // until we hit end-of-string or find '*/'
					i += 1;
					if (contents[i] == 0) {
						i -= 1; // we dont want the outer-loop to miss the 0-character
						break;
					}
					if (contents[i] == '*') {
						i += 1;
						if (contents[i] == '/') {
							// we have end comment
							end_comment = i;
							/*
							int n_chars = end_comment - start_comment + 1; // include '/'
							printf("n_chars: %d\n", n_chars);
							assert(n_chars < SIZE - 1);
							strncpy(buffer, &contents[start_comment], n_chars);
							buffer[n_chars] = 0; // we assume we actually copied this many chars (?)
							printf("%s\n", buffer);
							*/
							break;
						}
						i -= 1;
					}
				} // 'while'
				continue;
			}
			i -= 1;
		} // '/'

		if (contents[i] == '{') {
			buffer[index] = 0;
			index = 0;
			//printf("node: %s\n", buffer);
			
			const char *node_name = trim_whitespace(buffer);
			
			//printf("node name: %s\n", node_name);
			
			struct Node *n = (struct Node *) malloc(sizeof(struct Node));
			n->name = strdup(node_name);
//			n->nodes = new_list();
			ArrayInit(&(n->nodes));

//			list_append(node_path[path_index - 1]->nodes, n);
			ArrayAdd(&(node_path[path_index-1]->nodes), n);
			
			node_path[path_index++] = n;
			//list_add()
			/*
			for (int i = 0; i < path_index; ++i) {
				printf("%s\n", node_path[i]->name);
			}
			printf("\n");
			*/

			continue;
		}

		if (contents[i] == '}') {
			buffer[index] = 0;
			index = 0;
			//printf("node: %s\n", buffer);

			const char *node_name = trim_whitespace(buffer);
			if (strlen(node_name) == 0) {
				path_index -= 1;
				continue;
			}

			//printf("node name: %s\n", node_name);

			struct Node *n = (struct Node *) malloc(sizeof(struct Node));
			n->name = strdup(node_name);
//			n->nodes = new_list();
			ArrayInit(&(n->nodes));

//			list_append(node_path[path_index - 1]->nodes, n);
			ArrayAdd(&(node_path[path_index-1]->nodes), n);
			
			path_index -= 1;
			/*
			for (int i = 0; i < path_index; ++i) {
				printf("%s\n", node_path[i]->name);
			}
			printf("\n");
			*/

			continue;
		}
		
		// store character
		//printf("%c\n", contents[i]);
		buffer[index++] = contents[i];
	}

	// traverse the tree
	/*{
		struct CList *nodes = new_list(); // stack
	
		// push item
		list_append(nodes, root);
	
		while (nodes->i_end > 0) {
	
			// pop item
			struct Node *node = (struct Node *) nodes->data[nodes->i_end - 1];
			nodes->i_end -= 1;
	
			for (int i = 0; i < node->nodes->i_end; ++i) {
				// push item
				list_append(nodes, node->nodes->data[i]);
			}
	
			printf("node name: %s\n", node->name);
		}
	}*/

	//print_nodes(root);
/*
	printf("current-line-background: %s\n", settings_get_value(root, "current-line-background"));
	printf("node/subnode1/subsubnode1: %s\n", settings_get_value(root, "node/subnode1/subsubnode1"));
	printf("doesnotexist: %s\n", settings_get_value(root, "doesnotexist"));
	printf("highlighting/languages/C/tags/comment/foreground-color: %s\n",
		settings_get_value(root, "highlighting/languages/C/tags/comment/foreground-color"));
	printf("highlighting/languages/C/tags/comment/style: %s\n",
		settings_get_value(root, "highlighting/languages/C/tags/comment/style"));
*/
	// iterate over languages
	/*
	struct Node *n_languages = get_node(root, "highlighting/languages");
	struct CList *children = n_languages->nodes;
	for (int i = 0; i < children->i_end; ++i) {
		struct Node *child = (struct Node *) children->data[i];
		const char *language = child->name;
		printf("* %s\n", language);
	}
*/

	free(contents);

	return root;
}

struct CList *new_list(void) {
	struct CList *l = (struct CList *) malloc(sizeof(struct CList));
	l->i_end = 0;
	l->size = CLIST_INITIAL_SIZE;
	l->data = (void **) malloc(l->size * sizeof(void *));
	return l;
}

void list_append(struct CList *l, void *item) {
	if (!(l->i_end < l->size)) {
		// realloc()?
		l->size *= 2;
		void **new_memory = (void **) malloc(l->size * sizeof(void *));
		for (int i = 0; i < l->i_end; ++i) {
			new_memory[i] = l->data[i];
		}
		free(l->data); // freeing the list of pointers, not what is being pointed to
		l->data = new_memory;
	}
	l->data[l->i_end] = item;
	l->i_end += 1;
}

void *list_pop_last(struct CList *l) {
	void *r = NULL;
	if (l->i_end > 0) {
		l->i_end -= 1;
		r = l->data[l->i_end];
	}
	return r;
}

bool list_delete_item(struct CList *l, void *item)
{
	bool r = false;

	int i = 0;
	while (l->data[i] != item && i < l->i_end) {
		i += 1;
	}
	if (i < l->i_end) {
		// we found the item
		r = true;

		// we dont do this because we dont know what it is.
		// we expect our caller to free the thing themselves.
		//free(l->data[i]);

		for (int j = i + 1; j < l->i_end; ++j) {
			l->data[j - 1] = l->data[j];
		}

		l->i_end -= 1;
	}

	return r;
}

//struct Table *table_create(void)
//{
//	struct Table *t = (struct Table *) malloc(sizeof(struct Table));
//	t->names = list_create<const char *>();
//	t->values = list_create<void *>();
//	return t;
//}
//
//void table_store(struct Table *t, const char *name, void *value)
//{
//	list_add(t->names, name);
//	list_add(t->values, value);
//}
//
//void *table_get(struct Table *t, const char *name)
//{
//	for (int i = 0; i < t->names->index; ++i) {
//		if (strcmp(name, t->names->data[i]) == 0) {
//			return t->values->data[i];
//		}
//	}
//	return NULL;
//}


/* Well thats an entirely pointless function probably.. */
void add_menu_item(GtkMenu *menu, const char *label, GCallback callback, gpointer data)
{
	LOG_MSG("add_menu_item(): \"%s\"\n", label);

	assert(menu != NULL);

	static GtkMenu *_menu = NULL;
	static int top_pos, bottom_pos;

	if (_menu != menu) {
		_menu = menu;
		top_pos = 0;
		bottom_pos = 1;
	}

	GtkWidget *item = gtk_menu_item_new_with_label(label);
	gtk_menu_attach(GTK_MENU(menu), item, 0, 1, top_pos, bottom_pos);
	g_signal_connect(item, "activate", callback, data);

	top_pos += 1;
	bottom_pos += 1;
}


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

	/*
	// "If the tree_view already has a model set, it will remove it before setting the new model"
	// ... so i take it to mean that we dont need to explicitly free the model
	gtk_tree_view_set_model(GTK_TREE_VIEW(file_browser),
		GTK_TREE_MODEL(create_store()));
	*/
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(file_browser));
	gtk_tree_store_clear(GTK_TREE_STORE(model));
	create_nodes_for_dir(GTK_TREE_STORE(model), NULL, root_dir, 2);
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

/*
	printf("text_buffer_cursor_position_changed: line-highlighting stuff\n");
	// Line highlighting -- it messes up code highlighting.
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
*/
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

void on_text_view_size_allocate(GtkWidget *textview, GdkRectangle *alloc, gpointer data) {
	LOG_MSG("on_text_view_size_allocate()\n");
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(textview), alloc->height);
}

/*
void on_adjustment_value_changed(GtkAdjustment *adj, gpointer user_data)
{
	printf("*** on_adjustment_value_changed()\n");

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
*/

//@ should inform the user and/or use default values
void textview_apply_settings(GtkTextView *text_view, struct Node *settings)
{
	LOG_MSG("%s()\n", __FUNCTION__);

//	gtk_text_view_set_cursor_visible(text_view, FALSE);

	{
		const char *value_str = settings_get_value(settings, "pixels-above-lines");
		assert(value_str);
		gtk_text_view_set_pixels_above_lines(text_view, atoi(value_str));
	}

	{
		const char *value_str = settings_get_value(settings, "pixels-below-lines");
		assert(value_str);
		gtk_text_view_set_pixels_below_lines(text_view, atoi(value_str));
	}

	{
		const char *value_str = settings_get_value(settings, "pixels-inside-wrap");
		assert(value_str);
		gtk_text_view_set_pixels_inside_wrap(text_view, atoi(value_str));
	}

	{
		const char *value_str = settings_get_value(settings, "left-margin");
		assert(value_str);
		gtk_text_view_set_left_margin(text_view, atoi(value_str));
	}

	const char *value = settings_get_value(settings, "wrap-mode");
	assert(value);
	if (strcmp(value, "GTK_WRAP_NONE") == 0) {
		gtk_text_view_set_wrap_mode(text_view, GTK_WRAP_NONE);
	} else if (strcmp(value, "GTK_WRAP_CHAR") == 0) {
		gtk_text_view_set_wrap_mode(text_view, GTK_WRAP_CHAR);
	} else if (strcmp(value, "GTK_WRAP_WORD") == 0) {
		gtk_text_view_set_wrap_mode(text_view, GTK_WRAP_WORD);
	} else if (strcmp(value, "GTK_WRAP_WORD_CHAR") == 0) {
		gtk_text_view_set_wrap_mode(text_view, GTK_WRAP_WORD_CHAR);
	} else {
		assert(false);
	}
}

static void set_highlighting_based_on_file_extension(GtkWidget *tab, Node *settings, const char *file_name){
	LOG_MSG("set_highlighting_based_on_file_extension()\n");

	if(!file_name || *file_name == '\0'){ // NULL or ""
		highlighting_set(tab, "None");
		return;
	}

	const char *extension = Lib::basename_get_extension(Lib::filepath_get_basename(file_name));

	const char *language_found = NULL;
	Node *languages = get_node(settings, "languages");
//	assert(languages);
	if(!languages){
		highlighting_set(tab, "None");
		return;
	}

	for (int i = 0; !language_found && i < languages->nodes.Count; ++i) {
		Node *language = (Node *) languages->nodes.Data[i];
		
		Node *extensions = get_node(language, "file-extensions");
		if (!extensions) continue;

		for (int i = 0; i < extensions->nodes.Count; ++i) {
			Node *n = (Node *) extensions->nodes.Data[i];

			if(strcmp(n->name, extension) == 0){
				language_found = language->name;
				break;
			}
		}
	}

	if (language_found) {
		highlighting_set(tab, language_found);
	} else {
		highlighting_set(tab, "None");
	}
}

void line_highlighting_on_text_buffer_cursor_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data){
	long t1 = Lib::get_time_us();

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(object);

	GtkTextIter cursor_pos;
	get_cursor_position(text_buffer, NULL, &cursor_pos, NULL);

	GtkTextIter start_line, end_line;
	start_line = end_line = cursor_pos;
//	gtk_text_iter_backward_line(&start_line);
//	gtk_text_iter_forward_char(&start_line);
	gtk_text_iter_set_line_offset(&start_line, 0);
	gtk_text_iter_forward_line(&end_line);

	GtkTextIter start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
	gtk_text_buffer_remove_tag_by_name(text_buffer, "line-highlighting", &start_buffer, &end_buffer);

	gtk_text_buffer_apply_tag_by_name(text_buffer, "line-highlighting", &start_line, &end_line);

	long t2 = Lib::get_time_us();
	LOG_MSG("line higlighting took: %ldus\n", t2 - t1);
}

void line_highlighting_init(GtkTextBuffer *text_buffer, const char *color){
	LOG_MSG("line_highlighting_init()\n");

	assert(color);

	gtk_text_buffer_create_tag(text_buffer, "line-highlighting", "paragraph-background", color, NULL);

	g_signal_connect(text_buffer, "notify::cursor-position",	G_CALLBACK(line_highlighting_on_text_buffer_cursor_position_changed),	NULL);
}

//@ also comments probably
bool is_inside_literal_or_comment(GtkTextIter *iter)
{
	bool result = false;

	GSList *text_tags = gtk_text_iter_get_tags(iter);
	for(GSList *p = text_tags; p != NULL; p = p->next)
	{
		GtkTextTag *text_tag = (GtkTextTag *) p->data;
		const char *name;
		g_object_get(text_tag, "name", &name, NULL);
//		printf("tag name: %s\n", name);
		if ((strcmp(name, "string") == 0) || (strcmp(name, "char") == 0) || (strcmp(name, "comment") == 0))
		{
			result = true;
			break;
		}
	}
//	free(text_tags);
	
	return result;
}

//@ Is noticeably slow if matching parenthesis are far away from each other.
// I dont really understand why "paragraph-background" doesnt highlight the first line.
//void matching_parenthesis_highlighting_on_text_buffer_cursor_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data)
//{
//	printf("matching_parenthesis_highlighting_on_text_buffer_cursor_position_changed()\n");
//
//	// If no highlighting, dont do any of this. @Later on, we might want to think how to organize/factor things in a more reasonable way.
//	GtkWidget *tab = (GtkWidget *) user_data;
//	assert(tab);
//	if (tab_retrieve_widget(tab, HIGHLIGHTER) == NULL) return;
//	
//	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(object);
//	
//	GtkTextIter start_buffer, end_buffer;
//	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
//	gtk_text_buffer_remove_tag_by_name(text_buffer, "matching-parenthesis-highlighting", &start_buffer, &end_buffer);
//	gtk_text_buffer_remove_tag_by_name(text_buffer, "matching-brace-highlighting", &start_buffer, &end_buffer);
//	
//	GtkTextIter cursor_pos;
//	get_cursor_position(GTK_TEXT_BUFFER(text_buffer), NULL, &cursor_pos, NULL);
//	if (is_inside_literal(&cursor_pos)) return;
//	gunichar c = gtk_text_iter_get_char(&cursor_pos);
//	if (c == '(')
//	{
//		GtkTextIter iter = cursor_pos;
//		int level = 0;
//		while (gtk_text_iter_forward_char(&iter))
//		{
//			if (is_inside_literal(&iter)) continue;
//			c = gtk_text_iter_get_char(&iter);
//			if (c == '(')
//			{
//				level += 1;
//			}
//			if (c == ')')
//			{
//				if (level == 0)
//				{
//					GtkTextIter end1 = cursor_pos;
//					GtkTextIter end2 = iter;
//					gtk_text_iter_forward_char(&end1);
//					gtk_text_iter_forward_char(&end2);
//					gtk_text_buffer_apply_tag_by_name(text_buffer, "matching-parenthesis-highlighting", &cursor_pos, &end1);
//					gtk_text_buffer_apply_tag_by_name(text_buffer, "matching-parenthesis-highlighting", &iter, &end2);
//					break;
//				}
//				else
//				{
//					level -= 1;
//				}
//			}
//		}
//	}
//	else if (c == ')')
//	{
//		GtkTextIter iter = cursor_pos;
//		int level = 0;
//		while (gtk_text_iter_backward_char(&iter))
//		{
//			if (is_inside_literal(&iter)) continue;
//			c = gtk_text_iter_get_char(&iter);
//			if (c == ')')
//			{
//				level += 1;
//			}
//			
//			if (c == '(')
//			{
//				if (level == 0)
//				{
//					GtkTextIter end1 = cursor_pos;
//					GtkTextIter end2 = iter;
//					gtk_text_iter_forward_char(&end1);
//					gtk_text_iter_forward_char(&end2);
//					gtk_text_buffer_apply_tag_by_name(text_buffer, "matching-parenthesis-highlighting", &cursor_pos, &end1);
//					gtk_text_buffer_apply_tag_by_name(text_buffer, "matching-parenthesis-highlighting", &iter, &end2);
//					break;
//				}
//				else
//				{
//					level -= 1;
//				}
//			}
//		}
//	}
//	if (c == '{')
//	{
//		GtkTextIter iter = cursor_pos;
//		int level = 0;
//		while (gtk_text_iter_forward_char(&iter))
//		{
//			if (is_inside_literal(&iter)) continue;
//			c = gtk_text_iter_get_char(&iter);
//			if (c == '{')
//			{
//				// if inside string or char-literal, then ignore.
//				level += 1;
//			}
//			if (c == '}')
//			{
//				// if inside string or char-literal, then ignore.
//				if (level == 0)
//				{
//					gtk_text_iter_forward_char(&iter);
//					gtk_text_buffer_apply_tag_by_name(text_buffer, "matching-brace-highlighting", &iter, &cursor_pos);
//					break;
//				}
//				else
//				{
//					level -= 1;
//				}
//			}
//		}
//	}
//	else if (c == '}')
//	{
//		GtkTextIter iter = cursor_pos;
//		int level = 0;
//		while (gtk_text_iter_backward_char(&iter))
//		{
//			if (is_inside_literal(&iter)) continue;
//			c = gtk_text_iter_get_char(&iter);
//			if (c == '}')
//			{
//				level += 1;
//			}
//			
//			if (c == '{')
//			{
//				if (level == 0)
//				{
//					GtkTextIter range_end = cursor_pos;
//					gtk_text_iter_forward_char(&range_end);
//					gtk_text_buffer_apply_tag_by_name(text_buffer, "matching-brace-highlighting", &iter, &range_end);
//					break;
//				}
//				else
//				{
//					level -= 1;
//				}
//			}
//		}
//	}
//}

//void matching_parenthesis_highlighting_init(GtkTextBuffer *text_buffer, GtkWidget *tab){
//	LOG_MSG("matching_parenthesis_highlighting_init()\n");
//
//	gtk_text_buffer_create_tag(
//		text_buffer,
//		"matching-parenthesis-highlighting",
//		"underline", PANGO_UNDERLINE_SINGLE,
////		"background", "red",
//		NULL);
//
//	gtk_text_buffer_create_tag(
//		text_buffer,
//		"matching-brace-highlighting",
//		"paragraph-background", "rgb(240, 255, 240)",
//		NULL);
//
//	assert(tab);
//	g_signal_connect(
//		text_buffer,
//		"notify::cursor-position",
//		G_CALLBACK(matching_parenthesis_highlighting_on_text_buffer_cursor_position_changed),
//		tab);
//}

void matching_char_highlighting_on_cursor_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data)
{
	long t1 = Lib::get_time_us();

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(object);
	
	GtkTextIter start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
	gtk_text_buffer_remove_tag_by_name(text_buffer, "matching-char-highlighting", &start_buffer, &end_buffer);
	
	GtkTextIter cursor_pos; get_cursor_position(GTK_TEXT_BUFFER(text_buffer), NULL, &cursor_pos, NULL);
	GtkTextIter iter = cursor_pos;
	Scope chars[] = {
		{'(', ')'},
	};

	gunichar c = gtk_text_iter_get_char(&cursor_pos);
	if (c == '(')
	{
		if (is_inside_literal_or_comment(&cursor_pos)) return;

		if (move_end_scope(&iter, chars, sizeof(chars) / sizeof(chars[0]))) {
			GtkTextIter end1 = cursor_pos;
			GtkTextIter end2 = iter;
			gtk_text_iter_forward_char(&end1);
			gtk_text_iter_forward_char(&end2);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "matching-char-highlighting", &cursor_pos, &end1);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "matching-char-highlighting", &iter, &end2);
		}
	}
	else if (c == ')')
	{
		if (is_inside_literal_or_comment(&cursor_pos)) return;

		if (move_start_scope(&iter, chars, sizeof(chars) / sizeof(chars[0]))) {
			GtkTextIter end1 = cursor_pos;
			GtkTextIter end2 = iter;
			gtk_text_iter_forward_char(&end1);
			gtk_text_iter_forward_char(&end2);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "matching-char-highlighting", &cursor_pos, &end1);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "matching-char-highlighting", &iter, &end2);
		}
	}

	long t2 = Lib::get_time_us();
	LOG_MSG("matching char higlighting took: %ldus\n", t2 - t1);
}

void scope_highlighting_on_cursor_position_changed(GObject *_text_buffer, GParamSpec *pspec, gpointer user_data)
{
	long t1 = Lib::get_time_us();

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(_text_buffer);

	GtkTextIter start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
	gtk_text_buffer_remove_tag_by_name(text_buffer, "scope-highlighting", &start_buffer, &end_buffer);

	GtkTextIter cursor_pos; get_cursor_position(GTK_TEXT_BUFFER(text_buffer), NULL, &cursor_pos, NULL);
	GtkTextIter iter = cursor_pos;
	Scope chars[] = {
		{'{', '}'},
	};

	gunichar c = gtk_text_iter_get_char(&cursor_pos);

	if (c == '{')
	{
		if (is_inside_literal_or_comment(&cursor_pos)) return;
//		if (has_tag(&cursor_pos, "comment") || has_tag(&cursor_pos, "string") || has_tag(&cursor_pos, "char")) {return;}
//		if (has_tag(&cursor_pos, {"comment", "string", "char", NULL})) {return;}

		if (move_end_scope(&iter, chars, sizeof(chars) / sizeof(chars[0]))) {
			gtk_text_iter_forward_char(&iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "scope-highlighting", &cursor_pos, &iter);
		}
	}
	else if (c == '}')
	{
		if (is_inside_literal_or_comment(&cursor_pos)) return;

		if (move_start_scope(&iter, chars, sizeof(chars) / sizeof(chars[0]))) {
			gtk_text_iter_set_line_offset(&iter, 0);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "scope-highlighting", &iter, &cursor_pos);
		}
	}

	long t2 = Lib::get_time_us();
	LOG_MSG("scope higlighting took: %ldus\n", t2 - t1);
}

void matching_char_highlighting_init(GtkWidget *tab, const char *color_str)
{
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);
	gtk_text_buffer_create_tag(text_buffer, "matching-char-highlighting", "background", color_str, NULL);
	g_signal_connect(text_buffer, "notify::cursor-position", G_CALLBACK(matching_char_highlighting_on_cursor_position_changed), tab);

/*
gtk_text_buffer_remove_tag_by_name() call is much slower if we create the tag this way:
*/
//	GdkRGBA color;
//	if(gdk_rgba_parse(&color, color_str) == TRUE) {
//		gtk_text_buffer_create_tag(text_buffer, "matching-char-highlighting", "underline", PANGO_UNDERLINE_SINGLE, "underline-rgba", &color, NULL);
//		g_signal_connect(text_buffer, "notify::cursor-position",
//			G_CALLBACK(matching_char_highlighting_on_cursor_position_changed), tab);
//	} else {
//		fprintf(stderr, "error: failed to initialize matching-char-highlighting feature for a tab: failed to parse color: \"%s\"\n", color_str);
//	}
}

void scope_highlighting_init(GtkWidget *tab, const char *color)
{
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);
	gtk_text_buffer_create_tag(text_buffer, "scope-highlighting", "paragraph-background", color, NULL);
	g_signal_connect(text_buffer, "notify::cursor-position", G_CALLBACK(scope_highlighting_on_cursor_position_changed), tab);
}

bool is_word(unsigned int ch) {
	return (ch < NUM_ASCII_CHARS) ? ascii_chars[ch] : true;
}

void next_word_boundary(GtkTextIter *iter) {
	gunichar ch = gtk_text_iter_get_char(iter);
	if(is_word(ch)) {
		do {
			if(!gtk_text_iter_forward_char(iter)) {
				break;
			}
			ch = gtk_text_iter_get_char(iter);
//		} while(ch == '_' || g_unichar_isalnum(ch));
		} while(is_word(ch));
	} else {
		do {
			if(!gtk_text_iter_forward_char(iter)) {
				break;
			}
			ch = gtk_text_iter_get_char(iter);
//		} while(!(ch == '_' || g_unichar_isalnum(ch)));
		} while(!is_word(ch));
	}

/*
	do {
		ch = get_char(iter);
		if(!is_word(ch)) {
			break;
		}
	} while(forward(iter))

	while(forward(iter)) {
		ch = get_char(iter);
		if(!is_word(ch)) {
			break;
		}
	}
*/
}

void prev_word_boundary(GtkTextIter *iter) {
	bool hit_start_buffer = false;

	gunichar ch = gtk_text_iter_get_char(iter);
	if(is_word(ch)) {
		do {
			if(!gtk_text_iter_backward_char(iter)) {
				hit_start_buffer = true;
				break;
			}
			ch = gtk_text_iter_get_char(iter);
		} while(is_word(ch));
	} else {
		do {
			if(!gtk_text_iter_backward_char(iter)) {
				hit_start_buffer = true;
				break;
			}
			ch = gtk_text_iter_get_char(iter);
		} while(!is_word(ch));
	}

	if(!hit_start_buffer) {
		gtk_text_iter_forward_char(iter);
	}
}

void show_cursor(GtkTextView *view, bool should_show_cursor) {
	GdkWindow *window = gtk_text_view_get_window(view, GTK_TEXT_WINDOW_TEXT); // text-view has multiple windows
	GdkCursor *cursor = gdk_window_get_cursor(window);
	GdkCursorType cursor_type = gdk_cursor_get_cursor_type(cursor);
	bool make_cursor = should_show_cursor ? (cursor_type == GDK_BLANK_CURSOR) : (cursor_type != GDK_BLANK_CURSOR);
	if(make_cursor) {
		GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (view));
		GdkCursor *new_cursor = gdk_cursor_new_from_name(display, (should_show_cursor ? "text" : "none"));
		gdk_window_set_cursor(window, new_cursor); //@@ cleans up the old cursor?
		g_object_unref(new_cursor);
	}
}

gboolean textview_button_press(GtkTextView *view, GdkEventButton *event, gpointer page_id) {
	assert(event->type == GDK_BUTTON_PRESS
		|| event->type == GDK_DOUBLE_BUTTON_PRESS
		|| event->type == GDK_TRIPLE_BUTTON_PRESS);

	NotebookPage *page = &notebookPages.Data[(unsigned long)page_id];
	TextViewMouseSelectionState *state = &page->mouse_selection_state;

	if(!gtk_widget_is_focus(GTK_WIDGET(view))) {
		gtk_widget_grab_focus(GTK_WIDGET(view));
	}

	/*
	Normally: 1 -- left, 2 -- middle, 3 -- right
	*/
	if(event->button == 1) {
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);

		gint buf_x, buf_y;
		gtk_text_view_window_to_buffer_coords(view, GTK_TEXT_WINDOW_TEXT, event->x, event->y, &buf_x, &buf_y);
		GtkTextIter button_press_position;
		gtk_text_view_get_iter_at_location(view, &button_press_position, buf_x, buf_y);

		if(event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS) {
			assert(state->press_position != NULL);
			gtk_text_buffer_delete_mark(buffer, state->press_position); state->mark_count -= 1;
		}

		if(event->type == GDK_2BUTTON_PRESS) {
			LOG_MSG("2PRESS (page id: %d)\n", page->id);
//			printf("%p\n", view);

			GtkTextIter start = button_press_position; // cursor
			GtkTextIter end = button_press_position;

			prev_word_boundary(&start);
			next_word_boundary(&end);
			gtk_text_buffer_select_range(buffer, &start, &end);

			state->selection_granularity = SELECTION_GRANULARITY_WORD;
			state->original_selection_start = gtk_text_buffer_create_mark(buffer, NULL, &start, FALSE); state->mark_count += 1;
			state->original_selection_end = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE); state->mark_count += 1;
		} else if(event->type == GDK_3BUTTON_PRESS) {
			LOG_MSG("3PRESS (page id: %d)\n", page->id);
//			printf("%p\n", view);

			GtkTextIter start = button_press_position; // cursor
			GtkTextIter end = button_press_position;
			
			gtk_text_iter_set_line_offset(&start, 0);
			gtk_text_iter_forward_line(&end);
			gtk_text_buffer_select_range(buffer, &start, &end);

			state->selection_granularity = SELECTION_GRANULARITY_LINE;
			state->original_selection_start = gtk_text_buffer_create_mark(buffer, NULL, &start, FALSE); state->mark_count += 1;
			state->original_selection_end = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE); state->mark_count += 1;
		} else {
			LOG_MSG("PRESS (page id: %d)\n", page->id);
//			printf("%p\n", view);

			gtk_text_buffer_place_cursor(buffer, &button_press_position);

			state->selection_granularity = SELECTION_GRANULARITY_CHARACTER;
			state->press_position = gtk_text_buffer_create_mark(buffer, NULL, &button_press_position, FALSE); state->mark_count += 1;
//			printf("Creating bunch of marks\n");
//			for(int i = 0; i < MAX_TEST_MARKS; ++i) {
//				test_marks[i] = gtk_text_buffer_create_mark(buffer, NULL, &button_press_pos, FALSE);
//			}
		}

		return TRUE;
	} else {
		return FALSE;
	}
}

gboolean textview_button_release(GtkTextView *view, GdkEventButton *event, gpointer page_id) {
	assert(event->type == GDK_BUTTON_RELEASE);

	//@@ We are assuming that we are on a same tab/page where the button went down!

	NotebookPage *page = &notebookPages.Data[(unsigned long)page_id];
	TextViewMouseSelectionState *state = &page->mouse_selection_state;

	/*
	Normally: 1 -- left, 2 -- middle, 3 -- right
	*/
	if(event->button == 1) {
		LOG_MSG("BUTTON RELEASE (page id: %d)\n", page->id);
//		printf("%p\n", view);

		GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);

		if(state->selection_granularity == SELECTION_GRANULARITY_WORD || state->selection_granularity == SELECTION_GRANULARITY_LINE) {
			gtk_text_buffer_delete_mark(buffer, state->original_selection_start); state->mark_count -= 1;
			gtk_text_buffer_delete_mark(buffer, state->original_selection_end); state->mark_count -= 1;
		} else {
			assert(state->selection_granularity == SELECTION_GRANULARITY_CHARACTER);
			gtk_text_buffer_delete_mark(buffer, state->press_position); state->mark_count -= 1;
		}

		assert(state->mark_count == 0);

		state->selection_granularity = SELECTION_GRANULARITY_NONE;
//		GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
//		for(int i = 0; i < MAX_TEST_MARKS; ++i) {
//			gtk_text_buffer_delete_mark(buffer, test_marks[i]);
//		}

		return TRUE;
	} else {
		return FALSE;
	}
}

/* gtk_text_buffer_select_range() takes "99%" of the time */
gboolean textview_mouse_move(GtkTextView *view, GdkEventMotion *event, gpointer page_id) {
//	printf("MOUSE MOVE (x: %f, y: %f)\n", event->x, event->y);

	//@@ We are assuming that we are on a same tab/page where the button went down!

//	long t1 = Lib::get_time_us();

//	long a1 = Lib::get_time_us();

	NotebookPage *page = &notebookPages.Data[(unsigned long)page_id];
	TextViewMouseSelectionState *state = &page->mouse_selection_state;

	show_cursor(view, true);

//	long a2 = Lib::get_time_us();
//	printf("mouse move: a: %d\n", a2 - a1);

	if(state->selection_granularity != SELECTION_GRANULARITY_NONE) {
//		long b1 = Lib::get_time_us();

		GtkTextIter mouse_position;
		gint buf_x, buf_y;
		gtk_text_view_window_to_buffer_coords(view, GTK_TEXT_WINDOW_TEXT, event->x, event->y, &buf_x, &buf_y);
		gtk_text_view_get_iter_at_location(view, &mouse_position, buf_x, buf_y);

		GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);

//		long b2 = Lib::get_time_us();
//		printf("mouse move: b: %d\n", b2 - b1);

//		long c1 = Lib::get_time_us();

		if(state->selection_granularity == SELECTION_GRANULARITY_CHARACTER) {
//			assert(button_press_selection_start == NULL);
//			assert(button_press_selection_end == NULL);
//			assert(button_press_position != NULL);

//			GtkTextBuffer *state_buffer = gtk_text_mark_get_buffer(state->press_position);
//			assert(buffer == state_buffer);

			GtkTextIter start;
			start = mouse_position;

			GtkTextIter end;
			gtk_text_buffer_get_iter_at_mark(buffer, &end, state->press_position);

//			assert(gtk_text_iter_compare(&start, &end) != 0);
			long e1 = Lib::get_time_us();
			gtk_text_buffer_select_range(buffer, &start, &end);
			long e2 = Lib::get_time_us();
//			printf("mouse move: gtk_text_buffer_select_range(): %d\n", e2 - e1);
		} else if(state->selection_granularity == SELECTION_GRANULARITY_WORD) {
//			assert(button_press_selection_start != NULL);
//			assert(button_press_selection_end != NULL);
//			assert(button_press_position == NULL);

			GtkTextIter original_start, original_end;
			gtk_text_buffer_get_iter_at_mark(buffer, &original_start, state->original_selection_start);
			gtk_text_buffer_get_iter_at_mark(buffer, &original_end, state->original_selection_end);

			assert(gtk_text_iter_compare(&original_start, &original_end) <= 0);
			bool mouse_before_original = (gtk_text_iter_compare(&mouse_position, &original_start) < 0) ? true : false;
			bool mouse_after_original = (gtk_text_iter_compare(&mouse_position, &original_end) > 0) ? true : false;

			if(mouse_before_original) {
				GtkTextIter new_start = mouse_position;
				prev_word_boundary(&new_start);
				long e1 = Lib::get_time_us();
				gtk_text_buffer_select_range(buffer, &new_start, &original_end);
				long e2 = Lib::get_time_us();
//				printf("mouse move: gtk_text_buffer_select_range(): %d\n", e2 - e1);
			} else if(mouse_after_original) {
				GtkTextIter new_start = mouse_position;
				next_word_boundary(&new_start);
				long e1 = Lib::get_time_us();
				gtk_text_buffer_select_range(buffer, &new_start, &original_start);
				long e2 = Lib::get_time_us();
//				printf("mouse move: gtk_text_buffer_select_range(): %d\n", e2 - e1);
			} else {
				long e1 = Lib::get_time_us();
				gtk_text_buffer_select_range(buffer, &original_start, &original_end);
				long e2 = Lib::get_time_us();
//				printf("mouse move: gtk_text_buffer_select_range(): %d\n", e2 - e1);
			}
		} else if(state->selection_granularity == SELECTION_GRANULARITY_LINE) {
//			assert(button_press_selection_start != NULL);
//			assert(button_press_selection_end != NULL);
//			assert(button_press_position == NULL);

			GtkTextIter original_start, original_end;
			gtk_text_buffer_get_iter_at_mark(buffer, &original_start, state->original_selection_start);
			gtk_text_buffer_get_iter_at_mark(buffer, &original_end, state->original_selection_end);

			gint original_line = gtk_text_iter_get_line(&original_start);
			gint mouse_line = gtk_text_iter_get_line(&mouse_position);

			if(mouse_line < original_line) {
				GtkTextIter iter = mouse_position;
				gtk_text_iter_set_line_offset(&iter, 0);
				gtk_text_buffer_select_range(buffer, &iter, &original_end);
			} else if(mouse_line > original_line) {
				GtkTextIter iter = mouse_position;
				gtk_text_iter_forward_line(&iter);
				gtk_text_buffer_select_range(buffer, &iter, &original_start);
			} else {
				assert(mouse_line == original_line);
				gtk_text_buffer_select_range(buffer, &original_start, &original_end);
			}
		} else {
			assert(false);
		}

//		long c2 = Lib::get_time_us();
//		printf("mouse move: c: %d\n", c2 - c1);

//		bool scroll_up = false;
//		bool scroll_down = false;
//		bool scroll_left = false;
//		bool scroll_right = false;
//
//		if(buf_y < visible_rect.y) {scroll_up = true;}
//		if(buf_x < visible_rect.x) {scroll_left = true;}
//		if(buf_y > visible_rect.y + visible_rect.height) {scroll_down = true;}
//		if(buf_x > visible_rect.x + visible_rect.width) {scroll_right = true;}
//
//		if(???) {
//			gdouble sensitivity = 0.01;
//			gdouble vertical_pos = (vertical_scroll?) ? (up) ? 0.0 + sensitivity : 1.0 - sensitivity;
//			gdouble horizontal_pos = (left) ? 0.0 + sensitivity : 1.0 - sensitivity;
//			gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(view), &start, 0.0, TRUE, horizontal_alignment, vertical_alignment);
//		}

//		long d1 = Lib::get_time_us();

		// scroll if the user has moved the mouse pointer outside the visible area of the text buffer
		GdkRectangle visible_rect;
		gtk_text_view_get_visible_rect(view, &visible_rect);
		if(buf_x <= visible_rect.x || buf_x >= visible_rect.x + visible_rect.width || buf_y <= visible_rect.y || buf_y >= visible_rect.y + visible_rect.height) {
			gtk_text_view_scroll_to_iter(view, &mouse_position, 0.0, FALSE, 0.0, 0.0);
		}

//		long d2 = Lib::get_time_us();
//		printf("mouse move: d: %d\n", d2 - d1);
	}

//	long t2 = Lib::get_time_us();
//	printf("mouse_move: %ldus\n", t2 - t1);

	return TRUE;
}

void textview_buffer_changed(GtkTextBuffer *buffer, gpointer _view) {
	show_cursor(GTK_TEXT_VIEW(_view), false);
}

GtkWidget *create_tab(const char *file_name)
{
	int page_id = firstUnusedNotebookPageId;
	firstUnusedNotebookPageId += 1;

	char *tab_title;
	gchar *contents, *base_name;

	LOG_MSG("%s()\n", __FUNCTION__);

	GtkWidget *tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	//gtk_widget_set_hexpand(tab, TRUE);

	struct TabInfo *tab_info;
	tab_info = (TabInfo *) malloc(sizeof(struct TabInfo));
//	tab_info->id = first_unused_page_id;
	tab_info->id = page_id;
	if (file_name == NULL) {
		tab_info->file_name = NULL;

		tab_title = (char *) malloc(100);
//		snprintf(tab_title, 100, "%s %d", "Untitled", count);
		snprintf(tab_title, 100, "Untitled %d", tab_info->id + 1);
		tab_info->title = tab_title;
	} else {
		tab_info->file_name = file_name; //@ shouldnt we malloc a new buffer?
		tab_info->title = get_base_name(file_name);
	}
	g_object_set_data(G_OBJECT(tab), "tab-info", tab_info);

	/*@@
	Shouldnt this be done at the very end when the tab is ready, because apply_settings() might be called at any time?
	This is a thread synchronization issue.
	*/
	/*@@
	GtkNotebook's "switch-page" handler is called before this function returns. Is there anything else like this?
	*/
	NotebookPage *page = ArrayAppend(&notebookPages);
	Lib::ZeroMemory((uint8_t *)page, sizeof(NotebookPage)); // use calloc() in ArrayAppend() etc?
	page->id = page_id;
	page->tab = tab;

	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	//gtk_style_context_add_class (gtk_widget_get_style_context(scrolled_window), "scrolled-window");
	gtk_widget_set_vexpand(scrolled_window, TRUE);


	GtkTextView *text_view = GTK_TEXT_VIEW(gtk_text_view_new());
	page->view = text_view;
	textview_apply_settings(text_view, settings);

	// set_tab_stops_internal() in gtksourceview:
	gint position = 30;
	PangoTabArray *tab_array = pango_tab_array_new(1, TRUE); //@ free?
	pango_tab_array_set_tab(tab_array, 0, PANGO_TAB_LEFT, position);
	gtk_text_view_set_tabs(text_view, tab_array);
	pango_tab_array_free(tab_array);

	add_class(GTK_WIDGET(text_view), "text-view");

	g_signal_connect(G_OBJECT(text_view), "size-allocate", G_CALLBACK(on_text_view_size_allocate), NULL);


	//printf("*** file-name: %s, title: %s\n", tab_info->file_name, tab_info->title);
	GtkWidget *file_path_label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(file_path_label), TRUE);
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

	GtkWidget *status_bar = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(status_bar), 0);
	GtkWidget *margin = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(margin, 10, -1);
	GtkWidget *space = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand(space, TRUE);
	gtk_grid_attach(GTK_GRID(status_bar), margin, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(status_bar), statusbar_container, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(status_bar), space, 2, 0, 1, 1);

	GtkWidget *menu_button = create_highlighting_selection_button(tab, settings);
	gtk_grid_attach(GTK_GRID(status_bar), menu_button, 3, 0, 1, 1);

	add_class(status_bar, "status-bar");

	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);
	page->buffer = text_buffer;

	if (file_name) {
		char *contents;
		if(!Lib::ReadTextFile(file_name, &contents)) {
			assert(false);
		}
		gtk_text_buffer_set_text(text_buffer, contents, -1);
		free(contents);
	}

	tab_add_widget_4_retrieval(tab, TEXT_VIEW, text_view);
	tab_add_widget_4_retrieval(tab, TEXT_BUFFER, text_buffer); //@ haa text-buffer is not a widget! void *?
	tab_add_widget_4_retrieval(tab, FILEPATH_LABEL, file_path_label);

	GtkWidget *wgt_search = create_search_widget(tab);

	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(text_view));
	gtk_container_add(GTK_CONTAINER(tab), wgt_search);
	gtk_container_add(GTK_CONTAINER(tab), scrolled_window);
	gtk_container_add(GTK_CONTAINER(tab), status_bar);

	gtk_widget_show_all(GTK_WIDGET(tab));

	int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab, NULL);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_num);

	init_bookmarks(page);

	/*
	If we combined line highlighting, '{}' highlighting and '()' highlighting into one callback, we would be more efficient, but I tested performance while all of them turned off, and honestly, the win would be insignificant. So I am too lazy to do that.
	*/
	{
		const char *value = settings_get_value(settings, "line-highlighting/color");

		if(value)
			line_highlighting_init(text_buffer, value);
		else{
			ERROR("Setting \"line-highlighting/color\" doesnt seem to be set in the settings file. (Reverting to default value then: black)")
			line_highlighting_init(text_buffer, "black");
		}
	}
	{
		const char *value = settings_get_value(settings, "scope-highlighting/color");
		if (value) {
			scope_highlighting_init(tab, value);
		} else {
			ERROR("Setting \"scope-highlighting/color\" doesnt seem to be set in the settings file!")
		}
	}
	{
		const char *value = settings_get_value(settings, "matching-char-highlighting/color");
		if (value) {
			matching_char_highlighting_init(tab, value);
		} else {
			ERROR("Setting \"matching-char-highlighting/color\" doesnt seem to be set in the settings file!)")
		}
	}

	highlighting_init(tab, page, settings); //@ get rid of this
	set_highlighting_based_on_file_extension(tab, settings, file_name);

	// autocomplete-identifier
	autocomplete_identifier_init(tab, text_buffer);

	/* We want text-expansions's handler for "insert-text"-signal to be the first handler called.  
	(code-highlighting and undo also register callbacks for this signal.) */
//	init_autocomplete_character(text_buffer, settings, tab);
	text_expansion_init(text_buffer, settings, tab);
	gulong text_expansion_insert_id = g_signal_connect(G_OBJECT(text_buffer), "insert-text", G_CALLBACK(text_expansion_text_buffer_insert_text), NULL);
	g_signal_connect(G_OBJECT(text_buffer), "delete-range", G_CALLBACK(text_expansion_text_buffer_delete_range), NULL);
	g_signal_connect(G_OBJECT(text_buffer), "begin-user-action", G_CALLBACK(text_expansion_text_buffer_begin_user_action), NULL);
//	tab_add_widget_4_retrieval(tab, AUTOCOMPLETE_CHARACTER_HANDLER_ID, (void *) id); //@ hack
	// we block the autocomplete-character's "insert-text" handler to prevent it from autocompleting our undos's.
	// autocomplete-character should only complete user-level insertions, but how do we differentiate between user-level and hmm program-level?

	tab_set_unsaved_changes_to(tab, FALSE);

	/* @ Could just pass in tab-id directly? Performance? */
	gulong undo_insert_id = g_signal_connect(G_OBJECT(text_buffer), "insert-text", G_CALLBACK(undo_text_buffer_insert_text), tab);
	gulong undo_delete_id = g_signal_connect(G_OBJECT(text_buffer), "delete-range", G_CALLBACK(undo_text_buffer_delete_range), tab);

	g_signal_connect(G_OBJECT(text_view), "copy-clipboard", G_CALLBACK(text_view_copy_clipboard), NULL);
	g_signal_connect(G_OBJECT(text_view), "cut-clipboard", G_CALLBACK(text_view_cut_clipboard), NULL);
	g_signal_connect(G_OBJECT(text_view), "paste-clipboard", G_CALLBACK(text_view_paste_clipboard), NULL);

	//g_signal_connect(G_OBJECT(text_view), "populate-popup", G_CALLBACK(on_text_view_populate_popup), NULL);

	text_buffer_cursor_position_changed(G_OBJECT(text_buffer), NULL, line_number_label);
	g_signal_connect(G_OBJECT(text_buffer), "notify::cursor-position", G_CALLBACK(text_buffer_cursor_position_changed), line_number_label);

	g_signal_connect(G_OBJECT(text_buffer), "changed", G_CALLBACK(text_buffer_changed), NULL);
	//g_signal_connect_after(G_OBJECT(text_buffer), "changed", G_CALLBACK(text_buffer_changed_after), NULL);

	MultiCursor_Init(text_buffer, settings);
	g_signal_connect(text_view, "button-press-event", G_CALLBACK(MultiCursor_TextView_ButtonPress), text_buffer);
	gulong multicursor_insert_id = g_signal_connect_after(text_buffer, "insert-text", G_CALLBACK(MultiCursor_TextBuffer_InsertText), NULL);
	gulong multicursor_delete_id = g_signal_connect(text_buffer, "delete-range", G_CALLBACK(MultiCursor_TextBuffer_DeleteRange), NULL);

	gulong insert_handlers[] = {
		undo_insert_id,
		multicursor_insert_id,
		text_expansion_insert_id
	};
	gulong delete_handlers[] = {
		undo_delete_id,
		multicursor_delete_id
	};
	int insert_handlers_count = sizeof(insert_handlers) / sizeof(gulong);
	int delete_handlers_count = sizeof(delete_handlers) / sizeof(gulong);
//	printf("insert_handlers_count: %d, delete_handlers_count: %d\n", insert_handlers_count, delete_handlers_count);
	undo_init(insert_handlers, insert_handlers_count, delete_handlers, delete_handlers_count, tab_info->id);

	/*text view mouse text selection*/
	/*@@NotebookPage *get_page(int id)*/
	gpointer page_index = (gpointer)((unsigned long)(notebookPages.Count - 1));
	g_signal_connect(text_view, "button-press-event", G_CALLBACK(textview_button_press), page_index);
	g_signal_connect(text_view, "button-release-event", G_CALLBACK(textview_button_release), page_index);
	g_signal_connect(text_view, "motion-notify-event", G_CALLBACK(textview_mouse_move), page_index);

	/*
	GTK seems to have some default handler that makes the mouse pointer disappear as we start typing.
	This is undone when the mouse moves, by default.
	But since we dont rely on default mouse-move handler anymore, we ourselves have to make the cursor to reappear.
	That's all fine, but I can't figure out how to tell GTK that we made the cursor visible again.
	As far as GTK is concerned the cursor is still gone.
	Because GTK will not make the cursor disappear when it thinks its already disappeared, GTK will make the cursor disappear only once, and from then on, it refuses to do it again.
	At least something along those lines seems to be happening. I havent really read the GTK code or anything.
	This means that we have to hide/show the cursor ourselves.
	*/
	g_signal_connect_after(text_buffer, "changed", G_CALLBACK(textview_buffer_changed), text_view);

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

#define CTRL	1 // 0001
#define ALT		2 // 0010
#define SHIFT	4 // 0100
#define ALTGR	8 // 1000

#define SIZE_KEYCODES 	65536 	// number of different values (2**16 (key_event->hardware_keycode is a 16-bit variable))
#define SIZE_MODIFIERS 	16			// CTRL + ALT + SHIFT + ALTGR + 1
//gboolean (*key_combinations[SIZE_MODIFIERS][SIZE_KEYCODES])(GdkEventKey *key_event); // global arrays should be initialized to defaults (NULL) (?)
#define MAX_HANDLERS 3 // maximally, how many handlers for a keycombination
/* these should be initialized to 0's by default: */
gboolean (*key_combinations[SIZE_MODIFIERS][SIZE_KEYCODES][MAX_HANDLERS])(GdkEventKey *key_event);

void add_keycombination_handler(int modifiers, int keycode, gboolean (*handler)(GdkEventKey *key_event))
{
	LOG_MSG("add_keycombination_handler(): modifiers: %d, keycode: %d\n", modifiers, keycode);

	//@@ What if key combinations array is full (all key combinations have max number of handlers)? Wouldnt we simply just iterate over arbitrary memory until we find NULL somewhere or crash?
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
	LOG_MSG("on_app_window_key_press(): hardware keycode: %d\n", key_event->hardware_keycode);

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

	for (int i = 0; i < MAX_HANDLERS && key_combinations[modifiers][key_event->hardware_keycode][i] != NULL; ++i) {
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
	LOG_MSG("refresh_application_title()\n");

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
	LOG_MSG("%s()\n", __FUNCTION__);

	GtkWidget *text_view = (GtkWidget *) tab_retrieve_widget(tab, TEXT_VIEW);
	assert(text_view);
	gtk_widget_grab_focus(text_view);

	currentPageIndex = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	LOG_MSG("%s(): currentPageIndex: %d\n", __FUNCTION__, currentPageIndex);
}


gboolean create_empty_tab(GdkEventKey *key_event)
{
	create_tab();
	return TRUE;
}


gboolean exit_app(GdkEventKey *key_event)
{
	exit(EXIT_SUCCESS);
}

gboolean close_tab(GdkEventKey *key_event)
{
	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	if (!tab) {
		return FALSE;
	}

	// need to do this before freeing the tab-info because tab_retrieve_widget() needs tab-info
//	struct SortedStrs *autocomplete_words = (struct SortedStrs *) tab_retrieve_widget(tab, AUTOCOMPLETE_WORDS);
//	sorted_strs_free(autocomplete_words);

	// autocomplete-identifier
	AutocompleteState *state = (AutocompleteState *) tab_retrieve_widget(tab, AUTOCOMPLETE_STATE);
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));
	autocomplete_state_free(state, text_buffer);

	// test
	/*
	GtkTextView *text_view = (GtkTextView *) tab_retrieve_widget(tab, TEXT_VIEW);
	gtk_widget_destroy(GTK_WIDGET(text_view));
	*/
	//GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);
	//g_object_unref(text_buffer);

	struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	assert(tab_info);

	free((void *) tab_info->title);
	if (tab_info->file_name) {
		free((void *) tab_info->file_name);
	}
	free(tab_info);

//	void gtk_notebook_remove_page(GtkNotebook* notebook, gint page_num)
	gtk_widget_destroy(tab);

	assert(notebookPages.Count);
	ArrayRemove(&notebookPages, currentPageIndex);

	/* If the last page got removed, we have to set 'currentPageIndex' here because 'switch-page' signal doesn't fire. (Otherwise we update 'currentPageIndex' in GtkNotebook's 'switch-page' handler.) */
	if(!notebookPages.Count) {
		currentPageIndex = -1;
	}

	return TRUE;
}


gboolean tab_navigate_next(GdkEventKey *key_event) {
	printf("tab_navigate_next()\n");

	int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
	int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)); // returns -1 if no pages, starts counting from 0

	if (n_pages < 2) {
		printf("tab_navigate_next(): less than 2 tabs open -> nowhere to go...\n");
		return TRUE;
	}

	int last_page = n_pages - 1;
	int target_page = current_page + 1;
	if (target_page > last_page) target_page = 0;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), target_page);


//	NotebookPage *after_last = &notebook_pages[first_unused_page_id];
//	NotebookPage *first = &notebook_pages[0];
//
//	assert(visible_page >= first);
//	assert(visible_page < after_last);
//
//	do {
//		visible_page += 1;
//		if (visible_page == after_last) {
//			visible_page = first;
//		}
//	} while (!visible_page->in_use);
//
//	int num_pages_before = 0;
//	for (NotebookPage *nth = first; nth < visible_page; ++nth) {
//		if (nth->in_use) {
//			num_pages_before += 1;
//		}
//	}
//	assert(num_pages_before == target_page);
//	printf("num pages before visible page: %d\n", num_pages_before);

	return TRUE;
}

gboolean tab_navigate_previous(GdkEventKey *key_event) {
	printf("tab_navigate_previous()\n");

	int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
	int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)); // returns -1 if no pages, starts counting from 0

	if (n_pages < 2) {
		printf("tab_navigate_previous(): less than 2 tabs open -> nowhere to go...\n");
		return TRUE;
	}

	int last_page = n_pages - 1;
	int target_page = current_page - 1;
	if (target_page < 0) target_page = last_page;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), target_page);

//	NotebookPage *first = &notebook_pages[0];
//	assert(visible_page >= first);
//	do {
//		visible_page -= 1;
//		if (visible_page < first) {
//			visible_page = &notebook_pages[first_unused_page_id];
//			visible_page -= 1;
//		}
//	} while (!visible_page->in_use);
//
//	int num_pages = 0;
//	for (NotebookPage *nth_page = visible_page; nth_page >= first; nth_page -= 1) {
//		if (nth_page->in_use) {
//			num_pages += 1;
//		}
//	}
//	int num_pages_before = num_pages - 1;
//	assert(num_pages_before == target_page);
//	printf("num pages before visible page: %d\n", num_pages_before);

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
		return FALSE;
	}
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);

	actually_undo_last_action(tab);

	return TRUE;
}


gboolean do_save(GdkEventKey *key_event)
{
	LOG_MSG("do_save()\n");

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

	struct TabInfo *tab_info = (TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	assert(tab_info != NULL);

	if (tab_info->file_name == NULL) { // The tab doesnt have a file associated with it yet.
		const char *file_name = get_file_name_from_user(GTK_FILE_CHOOSER_ACTION_SAVE);
		//printf("SAVE: %s\n", file_name);
		if(file_name == NULL) return TRUE; // User didnt give us a file name.
		tab_info->file_name = file_name;
		tab_info->title = get_base_name(file_name);

		GtkLabel *filepath_label = (GtkLabel *) tab_retrieve_widget(tab, FILEPATH_LABEL);
		gtk_label_set_text(filepath_label, file_name);
	}

	//@ strlen()?
	if(!Lib::WriteFile(tab_info->file_name, (u8 *)contents, strlen(contents))) {
		assert(false);
	}

	tab_set_unsaved_changes_to(tab, FALSE);

	/* autocomplete: we'll update the list of words during each save-op to be more up-to-date */
//	struct SortedStrs *words = autocomplete_create_and_store_words(text_buffer);
//	struct SortedStrs *old_words = (struct SortedStrs *) tab_retrieve_widget(tab, AUTOCOMPLETE_WORDS);
//	sorted_strs_free(old_words);
//	tab_add_widget_4_retrieval(tab, AUTOCOMPLETE_WORDS, (void *) words);

	// autocomplete-identifier
	AutocompleteState *old_state = (AutocompleteState *) tab_retrieve_widget(tab, AUTOCOMPLETE_STATE);
	autocomplete_state_free(old_state, text_buffer);

	tab_add_widget_4_retrieval(tab, AUTOCOMPLETE_STATE,
		autocomplete_state_new(
				autocomplete_get_identifiers(
					contents)));

//	AutocompleteState *new_state = (AutocompleteState *) tab_retrieve_widget(tab, AUTOCOMPLETE_STATE);
//	autocomplete_print_identifiers(new_state->identifiers);

	free(contents);

	return TRUE;
}


gboolean do_open(GdkEventKey *key_event)
{
	LOG_MSG("do_open()\n");

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
	const char *file_name = (const char *) data;
	//const char *file_name = css_file_path;

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

	g_object_unref(css_file);

	return FALSE; // Dont call again
}

gboolean scroll_to_cursor_middle(GdkEventKey *key_event)
{
	LOG_MSG("scroll_to_cursor()\n");

	//@
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

	GtkTextIter i;
	GtkTextView *text_view = (GtkTextView *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW);
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);
	GtkAdjustment *adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(text_view));
	gdouble value = gtk_adjustment_get_value(adj);
	gtk_text_view_get_line_at_y(text_view, &i, value, NULL);
	gtk_text_iter_backward_lines(&i, 20);
	gtk_text_view_scroll_to_iter(text_view, &i, 0.0, TRUE, 0.0, 0.0);

	return TRUE;
}


gboolean scroll_down(GdkEventKey *key_event)
{
	LOG_MSG("scroll_down()\n");

/*
	GtkTextView *text_view = (GtkTextView *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW);
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);

	GtkTextIter i;
	GtkTextMark *m = gtk_text_buffer_get_mark(text_buffer, "scroll-mark");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &i, m);
	gtk_text_iter_forward_lines(&i, 20);
	gtk_text_view_scroll_to_iter(text_view, &i, 0.0, TRUE, 0.0, 0.0);
*/

	GtkTextIter i;
	GtkTextView *text_view = (GtkTextView *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW);
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);
	GtkAdjustment *adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(text_view));
	gdouble value = gtk_adjustment_get_value(adj);
	gtk_text_view_get_line_at_y(text_view, &i, value, NULL);
	gtk_text_iter_forward_lines(&i, 20);
	gtk_text_view_scroll_to_iter(text_view, &i, 0.0, TRUE, 0.0, 0.0);

	return TRUE;
}


gboolean apply_settings(gpointer user_arg)
{
	LOG_MSG("%s\n", __FUNCTION__);

	struct Node *new_settings = parse_settings_file(settings_file_path);
	//@@ free old settings
	settings = new_settings;

	for (int i = 0; i < notebookPages.Count; ++i) {
		printf("Updating tab: %d\n", notebookPages.Data[i].id);

		// apply SOME of the changed settings

		textview_apply_settings(notebookPages.Data[i].view, settings);

		highlighting_apply_settings(settings, &notebookPages.Data[i]);
		Highlighter highlighter = (Highlighter)tab_retrieve_widget(notebookPages.Data[i].tab, HIGHLIGHTER);
		if(highlighter) highlighter(notebookPages.Data[i].buffer, NULL);

		//...

//		// we are trying to update the tags in a very hacky way
//		char *highlighting = (char *) tab_retrieve_widget(tab, CURRENT_TEXT_HIGHLIGHTING);
//		assert(highlighting);
//
//		// the string that "highlighting" points to is freed by "set_text_highlighting()"
//		highlighting = strdup(highlighting);
//
//		//printf("highlighting before: %s\n", highlighting);
//		set_text_highlighting(tab, "None");
//		//printf("highlighting after: %s\n", highlighting);
//		set_text_highlighting(tab, highlighting);
//
//		free(highlighting);
//
//
//		// update highlighting menu
//		highlighting_update_menu(tab, settings);
//
//		// update line highlighting
//		GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);
//		highlighting_current_line_enable_or_disable(settings, text_buffer);
		
	}

	return FALSE; // dont call us again
}

void activate_handler(GtkApplication *app, gpointer data)
{
	LOG_MSG("%s()\n", __FUNCTION__);

//	unsigned char c = 32; //space
//	while(true) {
//		putchar(c);
////		printf("%d\n", c);
//		if(c == 255) {break;}
//		c += 1;
//	}
//	puts("");

//	for(int i = 0; i <= 0xff; ++i) {
//		is_word[i] = true;
//	}
//
//	const char *whitespace = " \n\t";
//	for(int i = 0; whitespace[i] != '\0'; ++i) {
//		is_word[whitespace[i]] = false;
//	}

	for(int ch = 0; ch < NUM_ASCII_CHARS; ++ch) {
		ascii_chars[ch] = false;
	}

	const char *word_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
	for(int i = 0; word_chars[i] != '\0'; ++i) {
		char ch = word_chars[i];
		ascii_chars[ch] = true;
	}

//	const char *all_ascii = "\t\n !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
//	for(int i = 0; all_ascii[i] != '\0'; ++i) {
//		is_word[all_ascii[i]] = true;
//	}

/*
Cant call set_root_dir() here because it expects file-browser and root-navigation to be already created.
If we used some kind of event/signal-thing, which allows abstractions to register callbacks to be executed in response to events like "root-directory-change" we wouldnt have to worry about that. Because then the code that individual-abstractions need to run would be provided by them in the form of callbacks and wouldnt be hardcoded into set_root_dir function.
*/

	char *home_dir = getenv("HOME"); assert(home_dir);
	//printf("home directory: %s\n", home_dir);
	snprintf(root_dir, ROOT_DIR_SIZE, "%s", home_dir);

	add_keycombination_handler(0, 84, scroll_to_cursor_middle); // 84 - numpad "5"
	add_keycombination_handler(0, 80, scroll_to_cursor_top); // 80 - numpad up
	add_keycombination_handler(0, 88, scroll_to_cursor_bottom); // 88 - numpad down
	add_keycombination_handler(0, 79, scroll_to_start); // 79 - numpad home
	add_keycombination_handler(0, 87, scroll_to_end); // 87 - numpad end
//	add_keycombination_handler(0, 81, scroll_up); // 81 - numpad page-up
//	add_keycombination_handler(0, 89, scroll_down); // 89 - numpad page-down

	add_keycombination_handler(CTRL, 10, add_bookmark); // ctrl + 1
	add_keycombination_handler(CTRL, 11, goto_next_bookmark); // ctrl + 2
	add_keycombination_handler(CTRL, 12, clear_bookmarks); // ctrl + 3

	// autocomplete-identifier
//	add_keycombination_handler(0, 67, autocomplete_emacs_style); // F1
//	add_keycombination_handler(0, 68, autocomplete_clear); // F2
	add_keycombination_handler(SHIFT, 23, autocomplete_clear); // shift + tab
	add_keycombination_handler(0, 23, autocomplete_emacs_style); // tab

	add_keycombination_handler(0, 23, handle_tab); // <tab>
	add_keycombination_handler(SHIFT, 23, handle_tab); // <tab> + shift

	add_keycombination_handler(0, 36, do_search);// <enter>
	add_keycombination_handler(SHIFT, 36, do_search);// <enter> + shift
	add_keycombination_handler(0, 36, handle_enter);// <enter>

//	add_keycombination_handler(0, 9, autocomplete_close_popup); // escape
//	add_keycombination_handler(0, 111, autocomplete_upkey); // up
//	add_keycombination_handler(0, 116, autocomplete_downkey); // down
//	add_keycombination_handler(0, 36, do_autocomplete); // enter
//	add_keycombination_handler(0, 23, do_autocomplete); // tab

	add_keycombination_handler(CTRL, 114, cursor_jump_forward); // ctrl + <right arrow>
	add_keycombination_handler(CTRL, 113, cursor_jump_backward); // ctrl + <left arrow>
	add_keycombination_handler(CTRL | SHIFT, 114, cursor_jump_forward); // ctrl + shift + <right arrow>
	add_keycombination_handler(CTRL | SHIFT, 113, cursor_jump_backward); // ctrl + shift + <left arrow>

	add_keycombination_handler(ALT, 114, cursor_jump_forward); // ctrl + <right arrow>
	add_keycombination_handler(ALT, 113, cursor_jump_backward); // ctrl + <left arrow>
	add_keycombination_handler(ALT | SHIFT, 114, cursor_jump_forward); // ctrl + shift + <right arrow>
	add_keycombination_handler(ALT | SHIFT, 113, cursor_jump_backward); // ctrl + shift + <left arrow>

	add_keycombination_handler(CTRL, 111, move_cursor_up); // ctrl + <up>
	add_keycombination_handler(CTRL, 116, move_cursor_down); // ctrl + <down>
	add_keycombination_handler(CTRL | SHIFT, 111, move_cursor_up);
	add_keycombination_handler(CTRL | SHIFT, 116, move_cursor_down);

	add_keycombination_handler(CTRL, 47, move_cursor_start_line); // ctrl + ö
	add_keycombination_handler(CTRL | SHIFT, 47, move_cursor_start_line_shift); // ctrl + shift + ö
	add_keycombination_handler(CTRL, 48, move_cursor_end_line); // ctrl + ä
	add_keycombination_handler(CTRL | SHIFT, 48, move_cursor_end_line_shift); // ctrl + shift + ä

	// alt + home/end?
	add_keycombination_handler(CTRL, 33, move_cursor_opening); // ctrl + p
	add_keycombination_handler(CTRL, 34, move_cursor_closing); // ctrl + ü
	add_keycombination_handler(CTRL | SHIFT, 33, move_cursor_opening); // shift + ctrl + p
	add_keycombination_handler(CTRL | SHIFT, 34, move_cursor_closing); // shift + ctrl + ü

	add_keycombination_handler(CTRL, 31, select_inside); // ctrl + i
	add_keycombination_handler(ALT, 31, delete_inside); // alt + i

	add_keycombination_handler(0, 86, jump_to_previous_occurrence); // numpad +
	add_keycombination_handler(0, 104, jump_to_next_occurrence); // numpad enter

	add_keycombination_handler(ALT, 111, move_lines_up); // alt + <up arrow>
	add_keycombination_handler(ALT, 116, move_lines_down); // alt + <down arrow>
//	add_keycombination_handler(ALT, 113, move_token_left); // alt + <left arrow>
//	add_keycombination_handler(ALT, 114, move_token_right); // alt + <right arrow>
	add_keycombination_handler(ALT, 35, insert_line_before); // alt + õ (35)
	add_keycombination_handler(ALT, 51, insert_line_after); // alt + ' (51)
	add_keycombination_handler(ALT, 40, duplicate_line); // alt + d
	add_keycombination_handler(ALT, 119, delete_line); // alt + <delete>
	add_keycombination_handler(ALT, 34, change_line); // alt + ü
	add_keycombination_handler(ALT, 33, delete_end_of_line); // alt + p
	add_keycombination_handler(ALT, 32, delete_word); // alt + o

	add_keycombination_handler(ALT, 61, comment_block); // alt + -
	add_keycombination_handler(SHIFT | ALT, 61, uncomment_block); // shift + alt + -

	add_keycombination_handler(CTRL, 52, undo_last_action); // ctrl + z

	add_keycombination_handler(CTRL, 57, create_empty_tab); // ctrl + n
	add_keycombination_handler(CTRL, 25, close_tab); // ctrl + w
	add_keycombination_handler(CTRL, 24, exit_app); // ctrl + q

//	add_keycombination_handler(CTRL, 21, tab_navigate_next); // ctrl + "the key left from backspace"
//	add_keycombination_handler(CTRL, 20, tab_navigate_previous); // ctrl + "the key left from the key left from backspace"
	add_keycombination_handler(CTRL, 23, tab_navigate_next); // ctrl + tab
	add_keycombination_handler(CTRL | SHIFT, 23, tab_navigate_previous); // ctrl + shift + tab

	add_keycombination_handler(CTRL, 39, do_save); // ctrl + s
	add_keycombination_handler(CTRL, 32, do_open); // ctrl + o

	add_keycombination_handler(CTRL, 41, toggle_search_entry); // ctrl + f
//	add_keycombination_handler(0, 37, toggle_search_entry); // ctrl
//	add_keycombination_handler(0, 106, toggle_search_entry); // numpad /
	add_keycombination_handler(CTRL, 43, display_openfile_dialog); // ctrl + h

	add_keycombination_handler(CTRL, 42, less_fancy_toggle_sidebar); // ctrl + g
	add_keycombination_handler(CTRL, 44, less_fancy_toggle_notebook); // ctrl + j


	// Overwrite global settings:
	GtkSettings *gtk_settings = gtk_settings_get_default();
	g_object_set(gtk_settings, "gtk-cursor-blink", FALSE, NULL);
	g_object_set(gtk_settings, "gtk-cursor-aspect-ratio", 0.07, NULL); // default: 0.040000


	//@ Different GTK versions require different CSS. Themes I wrote for 3.18.9 do not work on 3.24.34 and 3.24.43. So what should we do here?

	const char *css_file_used = NULL;
	if(gtk_version_major == 3 && gtk_version_minor == 18 && gtk_version_micro == 9) {
		css_file_used = "themes/style-3.18.9.css";
	} else if(gtk_version_major == 3 && gtk_version_minor == 24 && gtk_version_micro == 34) {
		css_file_used = "themes/style-3.24.34.css";
	} else if(gtk_version_major == 3 && gtk_version_minor == 24 && gtk_version_micro == 43) {
		css_file_used = "themes/style-3.24.43.css";
	} else {
		css_file_used = "themes/style-3.24.43.css";
	}
//	assert(css_file_used != NULL); // We dont have CSS for the version of GTK in use!

	apply_css_from_file((void *)css_file_used);
	hotloader_register_callback(css_file_used, apply_css_from_file, (void *)css_file_used);
	
	settings = parse_settings_file(settings_file_path);
	hotloader_register_callback(settings_file_path, apply_settings, NULL);

	//char *filename1 = strdup("/home/eero/test/file1");
	//char *filename2 = strdup("/home/eero/test/file2");
	//hotloader_register_callback("/home/eero/test/file1", test_func, (void *)filename1);
	//hotloader_register_callback("/home/eero/test/file2", test_func, (void *)filename2);

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


	ArrayInit(&notebookPages);

	notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	g_signal_connect_after(notebook, "switch-page", G_CALLBACK(on_notebook_switch_page), NULL);
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

//	autocomplete_init(GTK_NOTEBOOK(notebook), GTK_APPLICATION_WINDOW(app_window));

	GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_add1(GTK_PANED(paned), sidebar_container);
	//gtk_paned_add2(GTK_PANED(paned), notebook);
	gtk_paned_add2(GTK_PANED(paned), nb_container);
	gtk_container_add(GTK_CONTAINER(app_window), paned);

	gtk_widget_show_all(app_window);

	gtk_widget_hide(sidebar_container); // let's make sidebar hidden at startup

	create_tab(); // re-factor: create_tab() could just create the tab widget, it doesnt have to depend on the notebook at all (?)

/*
	for (int i = 0; i < 90; ++i) {
		create_tab(strdup("/home/eero/all/test-files/testfile-large"));
		close_tab(NULL);
	}
*/
}

//bool get_executable_path(char *buffer, int buffer_size)
//{
//	bool ret;
//	ssize_t num_copied = readlink("/proc/self/exe", buffer, buffer_size);
//	if (num_copied == -1)
//	{
//		// something went wrong (errno should contain further info)
//		ret = false;
//	}
//	else if (num_copied == buffer_size)
//	{
//		// path too long
//		ret = false;
//	}
//	else
//	{
//		buffer[num_copied] = 0;
//		ret = true;
//	}
//	return ret;
//}

const int MYBUFFER_MAX = 3;

struct MyBuffer {
	int data[MYBUFFER_MAX];
	int index;
	bool is_full;
};

void init_mybuffer(MyBuffer *buffer) {
	buffer->index = 0;
	buffer->is_full = false;
}

void add_number(MyBuffer *buffer, int n) {
	assert(MYBUFFER_MAX > 0);

	if(buffer->index == MYBUFFER_MAX) {
		buffer->is_full = true;
		buffer->index = 0;
	}

	if(buffer->is_full) {
		// we are overwriting a value at 'numbers_index'
		// so maybe you want to do something here		
	}

	buffer->data[buffer->index] = n;
	buffer->index += 1;
}

void print_numbers(MyBuffer *buffer) {
	if(buffer->is_full) {
		for(int i = buffer->index; i < MYBUFFER_MAX; ++i) {
			printf("%d\n", buffer->data[i]);
		}
		for(int i = 0; i < buffer->index; ++i) {
			printf("%d\n", buffer->data[i]);
		}
	} else {
		for(int i = 0; i < buffer->index; ++i) {
			printf("%d\n", buffer->data[i]);
		}
	}
}

int main(int argc, char *argv[])
{
//	int add_these[] = {1,2,3,666,42,101,102,103};
//	MyBuffer my_buffer;
//	init_mybuffer(&my_buffer);
//	printf("numbers:\n");
//	print_numbers(&my_buffer);
//	puts("");
//	for(int i = 0; i < sizeof(add_these) / sizeof(add_these[0]); ++i) {
//		add_number(&my_buffer, add_these[i]);
//		printf("after adding '%d', 'numbers' contains:\n", add_these[i]);
//		print_numbers(&my_buffer);
//		puts("");
//	}
//	return 0;

//	array<const char *> MyArray;
//	ArrayInit(&MyArray);
//	ArrayAdd(&MyArray, "Hello world!");
//	printf("%s\n", MyArray.Data[0]);
//	return 0;

	LOG_MSG("main()\n");

	// Make sure current working directory is the directory which contains the executable. (This might not be the case when the executable is executed through a symbolic link or through a Bash-shell from a different directory)
	const int exe_path_size = 1024; // allegedly MAX_PATH is not particularly trustworthy
	char exe_path[exe_path_size];
	if (!get_executable_path(exe_path, exe_path_size)) {
		//@ error handling
		printf("failed\n");
		return 1;
	}
	char *parent_path = get_parent_path_noalloc(exe_path);
	chdir(parent_path);
	LOG_MSG("Changed CWD to \"%s\"\n", parent_path);

	gtk_version_major = gtk_get_major_version();
	gtk_version_minor = gtk_get_minor_version();
	gtk_version_micro = gtk_get_micro_version();
	printf("GTK version: %u.%u.%u\n", gtk_version_major, gtk_version_minor, gtk_version_micro);

	GtkApplication *app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate_handler), NULL);

	int status = g_application_run(G_APPLICATION(app), 0, NULL);

//	g_object_unref(app);

	return status;
}
