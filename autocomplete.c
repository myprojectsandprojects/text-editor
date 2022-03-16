/*
autocomplete.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <gtk/gtk.h>

//#include "tab.h"
#include "declarations.h"
/*
int reading = 0;
int i = 0;
char word[100];
int j = 0;
*/
/*
char *words[100] = {
	"one",
	"two",
	"three",
};
*/

GtkApplicationWindow *_app_window;
GtkWidget *_suggestions_window;
GtkListBox *_suggestions_list;
GtkWidget *_scrolled_window;
GtkTextBuffer *_text_buffer;
GtkTextIter _start_ident, _end_ident; //@ feels kind of hacky

// we'll initialize these when creating the completions-window:
int _num_list_items;
int _index;

struct SortedStrs *sorted_strs_new(void)
{
	struct SortedStrs *p = (struct SortedStrs *) malloc(sizeof(struct SortedStrs));
	p->i = 0;
	return p;
}

void sorted_strs_free(struct SortedStrs *strs)
{
	// free strings
	for (int i = 0; i < strs->i; ++i) {
		free((void *) strs->strs[i]);
	}

	// free the struct itself
	free((void *) strs);
}

void sorted_strs_print(struct SortedStrs *strs)
{
	for (int i = 0; i < strs->i; ++i) {
		printf("(%d) %s\n", strs->counts[i], strs->strs[i]);
	}
}

// we make a copy of str and store a pointer to that
bool sorted_strs_add(struct SortedStrs *strs, const char *str)
{
	assert(strs->i <= MAX_STRS);

	// do we have space for new strings?
	if (strs->i == MAX_STRS) {
		return false; // we didnt actually store the string so we let the caller know by returning 'false'
	}
	//printf("adding: %s\n", str);
/*
	print_strs->strs();
	printf("\n");
*/
	for (int i = 0; i < strs->i; ++i) {
		if (strcmp(str, strs->strs[i]) == 0) {
			strs->counts[i] += 1;
			int j = i - 1;
			for (; j >= 0; --j) {
				if (strs->counts[i] <= strs->counts[j]) {
					break;
				}
			}
			j += 1;
			if (j < i) {
				// swap strings
				//printf("swapping %d with %d\n", i, j);
				const char *t_str = strs->strs[i];
				unsigned int t_count = strs->counts[i];
				strs->strs[i] = strs->strs[j];
				strs->counts[i] = strs->counts[j];
				strs->strs[j] = t_str;
				strs->counts[j] = t_count;
			}
			return true;
		}
	}

	//strs->strs[strs->i] = str;
	strs->strs[strs->i] = strdup(str);
	strs->counts[strs->i] = 1;
	strs->i += 1;
	return true;
}

struct SortedStrs *sorted_strs_get_starts_with(struct SortedStrs *strs, const char *starts_with)
{
	struct SortedStrs *result = sorted_strs_new();
	int j = 0;
	for (int i = 0; i < strs->i; ++i) {
		//printf("(%d) %s\n", strs->counts[i], strs->strs[i]);
		if (strstr(strs->strs[i], starts_with) == strs->strs[i] && strlen(strs->strs[i]) > strlen(starts_with)) {
			//add_str(result, strs->strs[i]);
			result->strs[j] = strs->strs[i];
			result->counts[j] = strs->counts[i];
			++j;
		}
	}
	result->i = j;
	return result;
}


void close_suggestions_window(void)
{
	LOG_MSG("close_suggestions_window()\n");

	/*
	if (_suggestions_window) {
		GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(_suggestions_list), 0);
		GtkAllocation alloc;
		gtk_widget_get_allocation(GTK_WIDGET(row), &alloc);
		printf("width: %d, height: %d, x: %d, y: %d\n",
			alloc.width, alloc.height, alloc.x, alloc.y);
	}
	*/

	if (_suggestions_window != NULL) {
		gtk_widget_destroy(_suggestions_window);
		_suggestions_window = NULL;
		_suggestions_list = NULL;
		_scrolled_window = NULL;
	}
}


static void display_suggestions_window(
	GtkTextView *text_view, GtkTextIter *location, struct SortedStrs *completions)
{
	LOG_MSG("display_suggestions_window()\n");

	close_suggestions_window();

	_suggestions_window = gtk_window_new(GTK_WINDOW_POPUP);
	//_suggestions_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	//gtk_window_set_decorated(GTK_WINDOW(_suggestions_window), FALSE);
	//GtkWidget *popup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
/*	
	g_signal_connect(_suggestions_window,
		"key-press-event", G_CALLBACK(autocomplete_on_window_key_press), NULL);
*/
	//gtk_style_context_add_class (gtk_widget_get_style_context(_suggestions_window), "suggestions-popup");

	{
		int width = 1;
		int height = 1;
		gtk_window_set_default_size(GTK_WINDOW(_suggestions_window), width, height);
	}

	_num_list_items = 0;
	_index = 0;

	GtkWidget *suggestions = gtk_list_box_new();
	add_class(suggestions, "suggestions-popup-list");
	for (int i = 0; i < completions->i; ++i) {
		char n[100];
		snprintf(n, 100, "(%d)", completions->counts[i]);
		GtkWidget *l1 = gtk_label_new(n);
		GtkWidget *l2 = gtk_label_new(completions->strs[i]);
		GtkWidget *c = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
		gtk_container_add(GTK_CONTAINER(c), l2);
		gtk_container_add(GTK_CONTAINER(c), l1);
		gtk_list_box_insert(GTK_LIST_BOX(suggestions), c, -1);
		_num_list_items += 1;
	}
	_suggestions_list = GTK_LIST_BOX(suggestions);

	int max_items = 6;
	if (_num_list_items > max_items) {
		_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(_scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		{
			int item_height = 20; // this we pick randomly for now
			int width = 0; // as small as possible
			int height = max_items * item_height;
			gtk_widget_set_size_request(_scrolled_window, width, height);
		}
		gtk_container_add(GTK_CONTAINER(_scrolled_window), suggestions);
		gtk_container_add(GTK_CONTAINER(_suggestions_window), _scrolled_window);
	} else {
		gtk_container_add(GTK_CONTAINER(_suggestions_window), suggestions);
	}


	GdkRectangle rect;
	gint x, y, o_x, o_y;
	GdkWindow *w = gtk_widget_get_window(GTK_WIDGET(text_view));


	gtk_text_view_get_iter_location(text_view, location, &rect);
	//printf("display_suggestions_window: buffer coordinates: x: %d, y: %d\n", rect.x, rect.y);

	gtk_text_view_buffer_to_window_coords(text_view, GTK_TEXT_WINDOW_WIDGET, rect.x, rect.y, &x, &y);
	//printf("display_suggestions_window: window coordinates: x: %d, y: %d\n", x, y);

	gdk_window_get_origin(w, &o_x, &o_y);
	//printf("display_suggestions_window: origin: x: %d, y: %d\n", x, y);
	gtk_window_set_attached_to(GTK_WINDOW(_suggestions_window), GTK_WIDGET(_app_window));
	gtk_window_move(GTK_WINDOW(_suggestions_window), x + o_x, y + o_y + 20);
	gtk_widget_show_all(_suggestions_window);
}


static void autocomplete_on_text_buffer_insert_text_after(
	GtkTextBuffer *text_buffer, GtkTextIter *location, char *text, int len, gpointer user_data)
{
	LOG_MSG("autocomplete_on_text_buffer_insert_text_after()\n");

	close_suggestions_window();

	//assert(strlen(text) == 1);
	// dont do autocomplete if the user pasted larger amount of text
	// or if it was autocompleted by autocomplete_character feature.
	if (strlen(text) != 1) {
		return;
	}

	GtkWidget *tab = (GtkWidget *) user_data;
	GtkTextView *text_view = (GtkTextView *) tab_retrieve_widget(tab, TEXT_VIEW);

	// figure out if we are at the end of an identifier
	{
		// * autocomplete?
		// * unicode vs ascii?

		gunichar c = gtk_text_iter_get_char(location);
		//printf("character at cursor: %c\n", c);
		if (!(isalnum(c) || c == '_')) {
			// possibly at the end of an identifier..
			GtkTextIter i;
			i = *location;
			gtk_text_iter_backward_char(&i);
			c = gtk_text_iter_get_char(&i);
			//printf("character before cursor: %c\n", c);
			if (isalnum(c) || c == '_') {
				// possibly at the end of an identifier..
				while (gtk_text_iter_backward_char(&i)) {
					c = gtk_text_iter_get_char(&i);
					if (!(isalnum(c) || c == '_')) {
						gtk_text_iter_forward_char(&i);
						break;
					}
				}
				
				// check the 1st character (first character of an identifier cant be a digit)
				c = gtk_text_iter_get_char(&i);
				if (isalpha(c) || c == '_') {
					char *text = gtk_text_buffer_get_text(text_buffer, &i, location, FALSE);
					//printf("autocomplete_on_text_buffer_insert_text_after: ");
					//printf("we are at the end of an identifier: %s\n", text);

					//char **words = (char **) tab_retrieve_widget(tab, AUTOCOMPLETE_WORDS);
					/*
					const char *possible_completions[100];
					// search for possible completions for the identifier
					{
						int j = 0;
						for (int i = 0; words[i] != NULL; ++i) {
							if (strstr(words[i], text) == words[i] && strlen(words[i]) > strlen(text)) {
								possible_completions[j] = words[i];
								++j;
							}
						}
						possible_completions[j] = NULL;
					}

					//char *completions[] = {"one", "two", "three", NULL};

					if (possible_completions[0] != NULL) {
						display_suggestions_window(text_view, location, possible_completions);
					}
					*/
					struct SortedStrs *words = (struct SortedStrs *) tab_retrieve_widget(tab, AUTOCOMPLETE_WORDS);
					struct SortedStrs *possible_completions = sorted_strs_get_starts_with(words, text);
					if (possible_completions->i != 0) {
						display_suggestions_window(text_view, location, possible_completions);
						_text_buffer = text_buffer;
						_start_ident = i;
						_end_ident = *location;
					}
					// possible_completions contains a list of pointers to the same strings that words do
					// so we dont want to free them
				} else {
					// not at the end of an identifier
				}
			} else {
				// not at the end of an identifier
			}
		} else {
			// not at the end of an identifier
		}
	}

/*
	int c = text[0];

	if (c == ' ' || c == '\t' || c == '\n') {
		if (reading) {
			reading = 0;
			word[i] = 0;
			words[j] = malloc(strlen(word) + 1);
			strcpy(words[j], word);
			++j;
			int k;
			for (k = 0; k < j; ++k) {
				printf("************************************** word: %s\n", words[k]);
			}
			i = 0;
		}
	} else {
		reading = 1;
		word[i] = c;
		++i;
	}
*/
}


gboolean autocomplete_upkey(GdkEventKey *key_event)
{
	LOG_MSG("autocomplete_upkey()\n");
/*
	if (_suggestions_window) {
		if (_index > 0) {
			_index -= 1;
			GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(_suggestions_list), _index);
			gtk_list_box_select_row(GTK_LIST_BOX(_suggestions_list), row);
		}
		return TRUE;
	}
	return FALSE;
*/
	if (!_suggestions_window) {
		return FALSE;
	}

	_index -= 1;

	if (_index < 0) {
		_index = _num_list_items - 1;
	}

	GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(_suggestions_list), _index);
	gtk_list_box_select_row(GTK_LIST_BOX(_suggestions_list), row);

	if (!_scrolled_window) {
		return TRUE;
	}

	GtkAdjustment *a = gtk_list_box_get_adjustment(GTK_LIST_BOX(_suggestions_list));
	gdouble value = gtk_adjustment_get_value(a);
	gdouble upper = gtk_adjustment_get_upper(a);
	gdouble lower = gtk_adjustment_get_lower(a);
	gdouble page_size = gtk_adjustment_get_page_size(a);
	//printf("value: %f, upper: %f, lower: %f, page-size: %f\n",
		//value, upper, lower, page_size);

	GtkAllocation alloc;
	gtk_widget_get_allocation(GTK_WIDGET(row), &alloc);
	//printf("width: %d, height: %d, x: %d, y: %d\n",
		//alloc.width, alloc.height, alloc.x, alloc.y);

	if (alloc.y >= value && alloc.y + alloc.height <= value + page_size) {
		//printf("selected item is visible..\n");
	} else {
		//printf("selected item is NOT visible..\n");

	if (alloc.y >= value) {
			// scrolling downwards
			gtk_adjustment_set_value(a, alloc.y - (page_size - alloc.height));
		} else {
			// scrolling upwards
			gtk_adjustment_set_value(a, alloc.y);
		}
		gtk_list_box_set_adjustment(GTK_LIST_BOX(_suggestions_list), a);
	}

	return TRUE; // dont call the default handler
}


gboolean autocomplete_downkey(GdkEventKey *key_event)
{
	LOG_MSG("autocomplete_downkey()\n");
/*
	if (_suggestions_window) {
		printf("_index: %d\n", _index);
		if (_index < _num_list_items - 1) {
			_index += 1;
			GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(_suggestions_list), _index);
			gtk_list_box_select_row(GTK_LIST_BOX(_suggestions_list), row);
		}
		return TRUE;
	}
	return FALSE;
*/

	if (!_suggestions_window) {
		return FALSE;
	}

	_index += 1;

	if (_index > _num_list_items - 1) {
		_index = 0;
	}

	GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(_suggestions_list), _index);
	gtk_list_box_select_row(GTK_LIST_BOX(_suggestions_list), row);

	if (!_scrolled_window) {
		return TRUE;
	}

	GtkAdjustment *a = gtk_list_box_get_adjustment(GTK_LIST_BOX(_suggestions_list));
	gdouble value = gtk_adjustment_get_value(a);
	gdouble upper = gtk_adjustment_get_upper(a);
	gdouble lower = gtk_adjustment_get_lower(a);
	gdouble page_size = gtk_adjustment_get_page_size(a);
	//printf("value: %f, upper: %f, lower: %f, page-size: %f\n",
		//value, upper, lower, page_size);

	GtkAllocation alloc;
	gtk_widget_get_allocation(GTK_WIDGET(row), &alloc);
	//printf("width: %d, height: %d, x: %d, y: %d\n",
		//alloc.width, alloc.height, alloc.x, alloc.y);

	if (alloc.y >= value && alloc.y + alloc.height <= value + page_size) {
		//printf("selected item is visible..\n");
	} else {
		//printf("selected item is NOT visible..\n");

	if (alloc.y >= value) {
			// scrolling downwards
			gtk_adjustment_set_value(a, alloc.y - (page_size - alloc.height));
		} else {
			// scrolling upwards
			gtk_adjustment_set_value(a, alloc.y);
		}
		gtk_list_box_set_adjustment(GTK_LIST_BOX(_suggestions_list), a);
	}

	return TRUE; // dont call the default handler
}


gboolean do_autocomplete(GdkEventKey *key_event)
{
	LOG_MSG("do_autocomplete()\n");
	if (_suggestions_window) {
		assert(_text_buffer);
		//char *to_autocomplete = gtk_text_buffer_get_text(_text_buffer, &_start_ident, &_end_ident, FALSE);
		GtkListBoxRow *row = gtk_list_box_get_selected_row(_suggestions_list);
		GtkWidget *x = gtk_bin_get_child(GTK_BIN(row));
		assert(GTK_IS_CONTAINER(x));
		GList *children = gtk_container_get_children(GTK_CONTAINER(x));
		assert(GTK_IS_LABEL(children->data));
		// assume first label is what we need
		//const gchar *text = gtk_label_get_label(GTK_LABEL(children->data)); // "This string is owned by the widget and must not be modified or freed"
		char *text = strdup((const char *) gtk_label_get_label(GTK_LABEL(children->data)));
		/*
		for (GList *p = children; p != NULL; p = p->next) {
			GtkWidget *widget = p->data;
			assert(GTK_IS_LABEL(widget));
			const char *label_text = gtk_label_get_label(GTK_LABEL(widget));
			printf("label text: %s\n", label_text);
		}
		*/

		GtkTextIter iter;
		GtkTextMark *m1 = gtk_text_buffer_create_mark(_text_buffer, NULL, &_start_ident, FALSE);
		GtkTextMark *m2 = gtk_text_buffer_create_mark(_text_buffer, NULL, &_end_ident, FALSE);
		//gtk_text_buffer_move_mark(text_buffer, m1, &match_start);
		//gtk_text_buffer_move_mark(text_buffer, m2, &match_end);
		gtk_text_buffer_delete(_text_buffer, &_start_ident, &_end_ident);
		assert(_text_buffer);
		gtk_text_buffer_get_iter_at_mark(_text_buffer, &iter, m1);
		gtk_text_buffer_insert(_text_buffer, &iter, text, -1);
		gtk_text_buffer_delete_mark(_text_buffer, m1);
		gtk_text_buffer_delete_mark(_text_buffer, m2);

		// gtk_text_buffer_insert() triggers our insert-text handler which deletes the suggestions-window...
/*
		gtk_widget_destroy(_suggestions_window);
		_suggestions_window = NULL;
*/
		_text_buffer = NULL;
		free(text);
		return TRUE;
	}
	return FALSE;
}


static void autocomplete_on_text_buffer_cursor_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data)
{
/*
	if (_suggestions_window) {
		if (!deleting_while_autocompleting) {
			gtk_widget_destroy(_suggestions_window);
			_suggestions_window = NULL;
		} else {
			deleting_while_autocompleting = 0;
		}
	}
*/
	LOG_MSG("autocomplete_on_text_buffer_cursor_position_changed()\n");
	close_suggestions_window();
}


gboolean autocomplete_close_popup(GdkEventKey *key_event)
{
	if (_suggestions_window) {
		gtk_widget_destroy(_suggestions_window);
		_suggestions_window = NULL;
		
		return TRUE;
	}
	return FALSE;
}


static void autocomplete_on_notebook_page_removed(
	GtkNotebook* self,
	GtkWidget* child,
	guint page_num,
	gpointer user_data)
{
	LOG_MSG("autocomplete_on_notebook_page_removed()\n");

	close_suggestions_window();
}


static void autocomplete_on_notebook_switch_page(
	GtkNotebook* self,
	GtkWidget* child,
	guint page_num,
	gpointer user_data)
{
	LOG_MSG("autocomplete_on_notebook_switch_page()\n");

	close_suggestions_window();
}


// create the list of words
	/*@ right now, for example: "0x123abc", which is a number literal, gives us "x123abc"
	and we store it as a word. if thats not what we want, we could also look for number-literals
	so that we can ignore those characters.. */
struct SortedStrs *autocomplete_create_and_store_words(GtkTextBuffer *text_buffer)
{
	LOG_MSG("autocomplete_create_and_store_words()\n");

	struct SortedStrs *words = sorted_strs_new();
	GtkTextIter i;

	// identifiers
	int num_overall 	= 0;
//	int num_stored 	= 0;

	for (gtk_text_buffer_get_start_iter(text_buffer, &i);
		!gtk_text_iter_is_end(&i); gtk_text_iter_forward_char(&i)) {
		gunichar c = gtk_text_iter_get_char(&i);
		//printf("character: %c (%d)\n", c, c);
		if (g_unichar_isalpha(c) || c == '_') {
			GtkTextIter ident_start = i;
			while (gtk_text_iter_forward_char(&i)) {
				c = gtk_text_iter_get_char(&i);
				if (!g_unichar_isalnum(c) && c != '_') break;
			}
			char *ident = gtk_text_buffer_get_text(text_buffer, &ident_start, &i, FALSE);
			num_overall += 1;
			bool is_space_left = true;
			if (strlen(ident) > 1) {
				 is_space_left = sorted_strs_add(words, ident);
			}
			free(ident);
			if (!is_space_left) break;
		}
	}
/*
	printf("number of identifiers stored: %d\n", words->i);
	printf("number of identifiers overall: %d\n", num_overall);
*/

	return words;
}


static void autocomplete_init_tab(GtkWidget *tab)
{
	LOG_MSG("autocomplete_init_tab()\n");

	GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);
	//GtkTextView *text_view = (GtkTextView *) tab_retrieve_widget(tab, TEXT_VIEW);
	assert(text_buffer);

	g_signal_connect_after(G_OBJECT(text_buffer), "insert-text",
		G_CALLBACK(autocomplete_on_text_buffer_insert_text_after), (gpointer) tab);

	g_signal_connect(G_OBJECT(text_buffer), "notify::cursor-position",
		G_CALLBACK(autocomplete_on_text_buffer_cursor_position_changed), NULL);

	struct SortedStrs *words = autocomplete_create_and_store_words(text_buffer);
	tab_add_widget_4_retrieval(tab, AUTOCOMPLETE_WORDS, (void *) words);
}


static void autocomplete_on_notebook_page_added(
	GtkNotebook* self,
	GtkWidget* child,
	guint page_num,
	gpointer user_data)
{
	LOG_MSG("autocomplete_on_notebook_page_added()\n");

	autocomplete_init_tab(child);
}


void autocomplete_init(GtkNotebook *notebook, GtkApplicationWindow* app_window)
{
	LOG_MSG("autocomplete_init()\n");

	_app_window = app_window;

	/* the only reason we want page-removed is to delete the window when the last page/tab is closed. 
	all other page-remove's always trigger switch-page also. */
	// maybe its possible to keep the suggestions-window when switching from one tab to another?
	g_signal_connect_after(G_OBJECT(notebook), "page-removed",
		G_CALLBACK(autocomplete_on_notebook_page_removed), NULL);

	g_signal_connect_after(G_OBJECT(notebook), "switch-page",
		G_CALLBACK(autocomplete_on_notebook_switch_page), NULL);

	g_signal_connect_after(G_OBJECT(notebook), "page-added",
		G_CALLBACK(autocomplete_on_notebook_page_added), NULL);
}