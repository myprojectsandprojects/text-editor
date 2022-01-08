/*
autocomplete.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <gtk/gtk.h>

#include "tab.h"
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

/*@ We dont close the suggestions-window when the user switches tabs, which means 1 is clearly not enough */
//extern GtkWidget *window;
GtkWidget					*_suggestions_window;
GtkApplicationWindow		*_app_window;

GtkTextIter _start_ident, _end_ident;
GtkTextBuffer *_text_buffer;
GtkListBox *_suggestions_list;
int deleting_while_autocompleting = 0;

// we'll initialize these when creating the completions-window:
int _num_list_items;
int _index;

struct StrList* create_strlist(void)
{
	struct StrList *p = malloc(sizeof(struct StrList));
	p->strs_i = 0;
	return p;
}

void print_strs(struct StrList *str_list)
{
	for (int i = 0; i < str_list->strs_i; ++i) {
		printf("(%d) %s\n", str_list->counts[i], str_list->strs[i]);
	}
}

/* 0 -- false, 1 -- true */
int add_str(struct StrList *str_list, const char *str)
{
	assert(str_list->strs_i <= MAX_STRS);

	// do we have space for new strings?
	if (str_list->strs_i == MAX_STRS) {
		return 0; // we didnt actually store the string so we let the caller know by returning 'false'
	}
	//printf("adding: %s\n", str);
/*
	print_str_list->strs();
	printf("\n");
*/
	for (int i = 0; i < str_list->strs_i; ++i) {
		if (strcmp(str, str_list->strs[i]) == 0) {
			str_list->counts[i] += 1;
			int j = i - 1;
			for (; j >= 0; --j) {
				if (str_list->counts[i] <= str_list->counts[j]) {
					break;
				}
			}
			j += 1;
			if (j < i) {
				// swap strings
				//printf("swapping %d with %d\n", i, j);
				const char *t_str = str_list->strs[i];
				unsigned int t_count = str_list->counts[i];
				str_list->strs[i] = str_list->strs[j];
				str_list->counts[i] = str_list->counts[j];
				str_list->strs[j] = t_str;
				str_list->counts[j] = t_count;
			}
			return 1;
		}
	}

	str_list->strs[str_list->strs_i] = str;
	str_list->counts[str_list->strs_i] = 1;
	str_list->strs_i += 1;
	return 1;
}

struct StrList *get_strs(struct StrList *str_list, const char *begins)
{
	struct StrList *result = create_strlist();
	int j = 0;
	for (int i = 0; i < str_list->strs_i; ++i) {
		//printf("(%d) %s\n", str_list->counts[i], str_list->strs[i]);
		if (strstr(str_list->strs[i], begins) == str_list->strs[i] && strlen(str_list->strs[i]) > strlen(begins)) {
			//add_str(result, str_list->strs[i]);
			result->strs[j] = str_list->strs[i];
			result->counts[j] = str_list->counts[i];
			++j;
		}
	}
	result->strs_i = j;
	return result;
}


static void display_suggestions_window(
	GtkTextView *text_view, GtkTextIter *location, struct StrList *completions)
{
	if (_suggestions_window != NULL) {
		gtk_widget_destroy(_suggestions_window);
		_suggestions_window = NULL;
	}

	GdkRectangle rect;
	gint x, y, o_x, o_y;
	GdkWindow *w = gtk_widget_get_window(GTK_WIDGET(text_view));


	gtk_text_view_get_iter_location(text_view, location, &rect);
	printf("create_popup(): buffer coordinates: x: %d, y: %d\n", rect.x, rect.y);

	gtk_text_view_buffer_to_window_coords(text_view, GTK_TEXT_WINDOW_WIDGET, rect.x, rect.y, &x, &y);
	printf("create_popup(): window coordinates: x: %d, y: %d\n", x, y);

	gdk_window_get_origin(w, &o_x, &o_y);
	printf("create_popup(): origin: x: %d, y: %d\n", x, y);


	_suggestions_window = gtk_window_new(GTK_WINDOW_POPUP);
	//_suggestions_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	//gtk_window_set_decorated(GTK_WINDOW(_suggestions_window), FALSE);
	//GtkWidget *popup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
/*	
	g_signal_connect(_suggestions_window,
		"key-press-event", G_CALLBACK(autocomplete_on_window_key_press), NULL);
*/
	//gtk_style_context_add_class (gtk_widget_get_style_context(_suggestions_window), "suggestions-popup");
	add_class(_suggestions_window, "suggestions-popup");

	gtk_window_set_default_size(GTK_WINDOW(_suggestions_window), 1, 200);

	_num_list_items = 0;
	_index = 0;

	GtkWidget *suggestions = gtk_list_box_new();
	for (int i = 0; i < completions->strs_i; ++i) {
		char n[100];
		snprintf(n, 100, "(%d)", completions->counts[i]);
		GtkWidget *l = gtk_label_new(completions->strs[i]);
		GtkWidget *l2 = gtk_label_new(n);
		GtkWidget *c = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
		gtk_container_add(GTK_CONTAINER(c), l);
		gtk_container_add(GTK_CONTAINER(c), l2);
		gtk_list_box_insert(GTK_LIST_BOX(suggestions), c, -1);
		_num_list_items += 1;
	}
	GtkWidget *scrollbars = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollbars), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	_suggestions_list = GTK_LIST_BOX(suggestions);

	gtk_container_add(GTK_CONTAINER(scrollbars), suggestions);
	gtk_container_add(GTK_CONTAINER(_suggestions_window), scrollbars);
	//gtk_window_set_decorated(GTK_WINDOW(popup_window), FALSE);
	//gtk_window_set_position(GTK_WINDOW(popup_window), GTK_WIN_POS_CENTER);
	//gtk_window_set_position(GTK_WINDOW(popup_window), GTK_WIN_POS_MOUSE);
	//gtk_window_set_transient_for(GTK_WINDOW(popup_window), GTK_WINDOW(window));
	gtk_window_set_attached_to(GTK_WINDOW(_suggestions_window), GTK_WIDGET(_app_window));
	gtk_window_move(GTK_WINDOW(_suggestions_window), x + o_x, y + o_y + 20);
	gtk_widget_show_all(_suggestions_window);
}


static void autocomplete_on_text_buffer_insert_text_after(
	GtkTextBuffer *text_buffer,
	GtkTextIter *location,
	char *text,
	int len,
	gpointer user_data)
{
	printf("autocomplete_on_text_buffer_insert_text()\n");
	//printf("text_buffer_insert_text_4_autocomplete(): %s\n", text);

	if (_suggestions_window != NULL) {
		gtk_widget_destroy(_suggestions_window);
		_suggestions_window = NULL;
	}

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
		printf("character @ cursor: %c\n", c);
		if (!(isalnum(c) || c == '_')) {
			printf("possibly at the end of an identifier..\n");
			GtkTextIter i;
			i = *location;
			gtk_text_iter_backward_char(&i);
			c = gtk_text_iter_get_char(&i);
			printf("character before cursor: %c\n", c);
			if (isalnum(c) || c == '_') {
				printf("still possibly at the end of an identifier..\n");
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
					printf("We are at the end of an identifier: %s\n", text);

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
					struct StrList *words = (struct StrList *) tab_retrieve_widget(tab, AUTOCOMPLETE_WORDS);
					struct StrList *possible_completions = get_strs(words, text);
					if (possible_completions->strs_i != 0) {
						display_suggestions_window(text_view, location, possible_completions);
						_text_buffer = text_buffer;
						_start_ident = i;
						_end_ident = *location;
					}
					free(possible_completions); //@ ?
				} else {
					printf("not at the end of an identifier..\n");
				}

			} else {
				printf("not at the end of an identifier..\n");
			}
		} else {
			printf("we are definately not at the end of an identifier..\n");
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
	printf("autocomplete_upkey()\n");

	if (_suggestions_window) {
		if (_index > 0) {
			_index -= 1;
			GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(_suggestions_list), _index);
			gtk_list_box_select_row(GTK_LIST_BOX(_suggestions_list), row);
		}
		return TRUE;
	}
	return FALSE;
}


gboolean autocomplete_downkey(GdkEventKey *key_event)
{
	printf("autocomplete_downkey()\n");

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
}


gboolean do_autocomplete(GdkEventKey *key_event)
{
	if (_suggestions_window) {
		printf("*** should do autocomplete ***\n");

		// todo: do autocomplete
		char *to_autocomplete = gtk_text_buffer_get_text(_text_buffer, &_start_ident, &_end_ident, FALSE);
		printf("should autocomplete: %s\n", to_autocomplete);

		GtkListBoxRow *row = gtk_list_box_get_selected_row(_suggestions_list);
		GtkWidget *x = gtk_bin_get_child(GTK_BIN(row));
		assert(GTK_IS_CONTAINER(x));
		GList *children = gtk_container_get_children(GTK_CONTAINER(x));
		assert(GTK_IS_LABEL(children->data));
		// assume first label is what we need
		const char *text = gtk_label_get_label(GTK_LABEL(children->data));
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
		deleting_while_autocompleting = 1; // deleting text changes the cursor position...
		gtk_text_buffer_delete(_text_buffer, &_start_ident, &_end_ident);
		gtk_text_buffer_get_iter_at_mark(_text_buffer, &iter, m1);
		gtk_text_buffer_insert(_text_buffer, &iter, text, -1);
		gtk_text_buffer_delete_mark(_text_buffer, m1);
		gtk_text_buffer_delete_mark(_text_buffer, m2);

		// gtk_text_buffer_insert() triggers our insert-text handler which deletes the suggestions-window...
/*
		gtk_widget_destroy(_suggestions_window);
		_suggestions_window = NULL;
*/
		return TRUE;
	}
	return FALSE;
}


static void autocomplete_on_text_buffer_cursor_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data)
{
	if (_suggestions_window) {
		if (!deleting_while_autocompleting) {
			printf("deleting tha window\n");
			gtk_widget_destroy(_suggestions_window);
			_suggestions_window = NULL;
		} else {
			deleting_while_autocompleting = 0;
		}
	}
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
	printf("autocomplete_on_notebook_page_removed()\n");

	if (_suggestions_window) {
		gtk_widget_destroy(_suggestions_window);
		_suggestions_window = NULL;
	}
}


static void autocomplete_on_notebook_switch_page(
	GtkNotebook* self,
	GtkWidget* child,
	guint page_num,
	gpointer user_data)
{
	printf("autocomplete_on_notebook_switch_page()\n");

	if (_suggestions_window) {
		gtk_widget_destroy(_suggestions_window);
		_suggestions_window = NULL;
	}
}


// create the list of words
	/*@ right now, for example: "0x123abc", which is a number literal, gives us "x123abc"
	and we store it as a word. if thats not what we want, we could also look for number-literals
	so that we can ignore those characters.. */
struct StrList *autocomplete_create_and_store_words(GtkTextBuffer *text_buffer)
{
	printf("*** autocomplete_create_and_store_words()\n");

	struct StrList *words = create_strlist();
	GtkTextIter i;

	int n = 0;
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
			if (strlen(ident) > 1) {
				//printf("identifier: %s\n", ident);
				/*
				words[j] = ident;
				++j;
				*/
				// if no room for new strings then break
				if (!add_str(words, ident)) {
					break;
				}
				++n;
				/*
				if (j - 1 == NUM_WORDS - 1) {
					break;
				}
				*/
			}
		}
	}
	printf("identifiers read: %d\n", n); // we only actually store unique strings

	return words;
}


static void autocomplete_init_tab(GtkWidget *tab)
{
	printf("autocomplete_init_tab()\n");

	GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);
	//GtkTextView *text_view = (GtkTextView *) tab_retrieve_widget(tab, TEXT_VIEW);
	assert(text_buffer);

	g_signal_connect_after(G_OBJECT(text_buffer), "insert-text",
		G_CALLBACK(autocomplete_on_text_buffer_insert_text_after), (gpointer) tab);

	g_signal_connect(G_OBJECT(text_buffer), "notify::cursor-position",
		G_CALLBACK(autocomplete_on_text_buffer_cursor_position_changed), NULL);

	struct StrList *words = autocomplete_create_and_store_words(text_buffer);
	tab_add_widget_4_retrieval(tab, AUTOCOMPLETE_WORDS, (void *) words);
}


static void autocomplete_on_notebook_page_added(
	GtkNotebook* self,
	GtkWidget* child,
	guint page_num,
	gpointer user_data)
{
	printf("*** autocomplete_on_notebook_page_added()\n");

	autocomplete_init_tab(child);
}


void autocomplete_init(GtkNotebook *notebook, GtkApplicationWindow* app_window)
{
	printf("autocomplete_init()\n");

	_app_window = app_window;

	/* the only reason we want page-removed is to delete the window when the last page/tab is closed. 
	all other page-remove's always trigger switch-page also.*/
	g_signal_connect_after(G_OBJECT(notebook), "page-removed",
		G_CALLBACK(autocomplete_on_notebook_page_removed), NULL);

	g_signal_connect_after(G_OBJECT(notebook), "switch-page",
		G_CALLBACK(autocomplete_on_notebook_switch_page), NULL);

	g_signal_connect_after(G_OBJECT(notebook), "page-added",
		G_CALLBACK(autocomplete_on_notebook_page_added), NULL);
}