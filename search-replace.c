#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <assert.h>

//#include "tab.h"
#include "declarations.h"


extern GtkWidget *notebook;
extern struct Node *settings;


const char *print_action2take(int action2take)
{
	const char *action2take_strs[] = {
		"SEARCH", "REPLACE", "GO_TO_LINE", "DO_NOTHING"};
	return action2take_strs[action2take];
}


int parse_str(const char *str2parse,
	int *line_num, char **search_str, char **replace_with_str)
{
	LOG_MSG("parse_str()\n");

	//#define BUFFER_SIZE 100 // how much text can user type into the entry?
	//char buffer[BUFFER_SIZE];
	int i1, i2, j;
	
	*line_num = -1;
	*search_str = *replace_with_str = NULL;
	int action2take = DO_NOTHING;
	i1 = i2 = j = 0;

	/* should check for validity? */
	if (str2parse[0] == ':') {
		*line_num = atoi(&str2parse[1]);
		return GO_TO_LINE;
	}

	//*search_str = malloc(strlen(str2parse));
	// ...causes this error: *** Error in `./a.out': corrupted size vs. prev_size: 0x00000000034eda60 ***
	// when searching for "on_highlighting_selected"
	
	*search_str = (char *) malloc(strlen(str2parse) + 1);
	char *current_buffer = *search_str;
	action2take = SEARCH;

	for (;;) {
		if (str2parse[i2] == '\0') {
			int n = i2 - i1 + 1; // '+ 1' to also copy the '\0'
			strncpy(&current_buffer[j], &str2parse[i1], n);
			break;
		}
		if (str2parse[i2] == '\\') {
			int i = i2 + 1;
			if (str2parse[i] == '\\') {
				int n = i2 - i1;
				strncpy(&current_buffer[j], &str2parse[i1], n);
				j += n;
				current_buffer[j] = '\\';
				j += 1;
				i1 = i2 = i2 + 2;
			} else if (str2parse[i] == 't') {
				int n = i2 - i1;
				strncpy(&current_buffer[j], &str2parse[i1], n);
				j += n;
				current_buffer[j] = '\t';
				j += 1;
				i1 = i2 = i2 + 2;
			} else if (str2parse[i] == 'n') {
				int n = i2 - i1;
				strncpy(&current_buffer[j], &str2parse[i1], n);
				j += n;
				current_buffer[j] = '\n';
				j += 1;
				i1 = i2 = i2 + 2;
			} else if (str2parse[i] == '/') {
				int n = i2 - i1;
				strncpy(&current_buffer[j], &str2parse[i1], n);
				j += n;
				current_buffer[j] = '/';
				j += 1;
				i1 = i2 = i2 + 2;
			} else if (str2parse[i] == ':') { // if user wants ":" as the 1st character, they should say "\:"
				int n = i2 - i1;
				strncpy(&current_buffer[j], &str2parse[i1], n);
				j += n;
				current_buffer[j] = ':';
				j += 1;
				i1 = i2 = i2 + 2;
			} else {
				// unknown escape sequence
				action2take = DO_NOTHING;
				break;
			}
		} else if (str2parse[i2] == '/') {
			if (i2 == 0) {
				action2take = DO_NOTHING;
				break;
			}
			if (action2take == REPLACE) {
				i2 += 1;
				continue;
			}
			/* we should do replace instead of search */
			action2take = REPLACE;
			int n = i2 - i1;
			strncpy(&current_buffer[j], &str2parse[i1], n);
			current_buffer[j + n] = 0;
			i1 = i2 = i2 + 1;
			*replace_with_str = (char *) malloc(strlen(str2parse));
			current_buffer = *replace_with_str;
			j = 0;
		} else {
			i2 += 1;
		}
	}

	return action2take;
}


/*@
cant for example replace "/" with "abc", because how would we say that?
also there is no way to refer to a special characters like "\n"
if we used "\" "to escape special characters", we would say: "\//abc" to replace "/" with "abc"
and: "\t/\n" to replace tabs with newlines
anything followed by \ would be interpreted in an unconventional way
so \/ would be literal forwardslash as / has a special meaning
and \n would be a special newline character as n has no special meaning
\\ would be literal backslash and \\\ would open a gateway to another dimension
*/
int not_used_parse_search_str(
	const char *str_2_parse,
	int *line_num,
	char **search,
	char **replace,
	char **with)
{
	char parsed[100];
	int action_2_take;
	int i1, i2, j;

	action_2_take = SEARCH;
	*replace = *with = *search = NULL;
	*line_num = -1;

	i1 = i2 = j = 0;

	// to see if we are actually 0-terminating our string:
	//memset((void *) parsed, 'z', sizeof(parsed));

	for (;;) {
		if (str_2_parse[i2] == 0) {
			int to_copy = i2 - i1 + 1; // '+ 1' to also copy the '\0'
			strncpy(&parsed[j], &str_2_parse[i1], to_copy);

			if (action_2_take == SEARCH) {
				*search = (char *) malloc(strlen(parsed) + 1);
				strcpy(*search, parsed);
			} else if (action_2_take == REPLACE) {
				*with = (char *) malloc(strlen(parsed) + 1);
				strcpy(*with, parsed);
			}
			break;
		} else if (i2 == 0 && str_2_parse[i2] == ':') {
			int count = 1;
			for (int j = 1; str_2_parse[j] == ':'; ++j) {
				++count;
			}
			if (count > 1) {
				// we have multiple ':'s at the beginning
				// we should subtract one of them and 
				// continue parsing the rest of the string...
				++i1;
				i2 += count;
				continue;
			}
			/* lets just check the validity for the heck of it... */
			if (str_2_parse[1] == 0) {
				action_2_take = DO_NOTHING;
				break;
			}
			int found_not_digit = 0;
			for (int j = 1; str_2_parse[j] != 0; ++j) {
				if (!isdigit(str_2_parse[j])) {
					found_not_digit = 1;
					break;
				}
			}
			if (found_not_digit) {
				action_2_take = DO_NOTHING;
				break;
			} else {
				action_2_take = GO_TO_LINE;
				*line_num = atoi(&str_2_parse[1]);
				break;
			}
		} else if (str_2_parse[i2] == '/') {
			int c = 1;
			for (; str_2_parse[i2+c] == '/'; ++c) ;
			if (c > 1) {
				// assume that actual number of characters copied
				// is i2 - i1 -- destination string doesnt run out.
				int to_copy = i2 - i1;
				strncpy(&parsed[j], &str_2_parse[i1], to_copy);
				j += to_copy;
				i1 = i2 + 1;
				i2 += c;
				continue;
			}
			// we already found a singular '/', multiple doesnt make sense.
			if (action_2_take == REPLACE) {
				action_2_take = DO_NOTHING;
				break;
			}
			action_2_take = REPLACE;
			// assume that actual number of characters copied
			// is i2 - i1 -- destination string doesnt run out.
			int to_copy = i2 - i1;
			strncpy(&parsed[j], &str_2_parse[i1], to_copy);
			j += to_copy;
			parsed[j] = 0;

			*replace = (char *) malloc(strlen(parsed) + 1);
			strcpy(*replace, parsed);
			j = 0;

			i1 = i2 = i2 + 1;
			continue;
		}
		
		++i2;
	}

	// it doesnt make sense to replace ""...
	if (action_2_take == REPLACE && strlen(*replace) == 0) {
		action_2_take = DO_NOTHING;
	}

	return action_2_take;
}


void on_search_entry_changed(GtkEditable *search_entry, gpointer data)
{
	/*
	search entry changed: make sure search-mark points at the beginning of the buffer!
	*/
	
	GtkTextIter start;
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) data;
	gtk_text_buffer_get_start_iter(text_buffer, &start);
	gtk_text_buffer_move_mark_by_name(text_buffer, "search-mark", &start);
}


GtkWidget *create_search_widget(GtkWidget *tab)
{
	LOG_MSG("create_search_widget()\n");

	GtkWidget *search_entry = gtk_search_entry_new();
	GtkWidget *search_revealer = gtk_revealer_new();
	//gtk_revealer_set_reveal_child(GTK_REVEALER(search_revealer), TRUE);

	gtk_container_add(GTK_CONTAINER(search_revealer), search_entry);

	tab_add_widget_4_retrieval(tab, SEARCH_REVEALER, search_revealer);
	tab_add_widget_4_retrieval(tab, SEARCH_ENTRY, search_entry);

	add_class(search_entry, "search-entry");

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));
	assert(text_buffer); // I think tab_retrieve_widget() returns NULL if the widget has'nt been stored because it uses a global array of pointers to store these widgets and global arrays of pointers are initialized to NULL's in C. 

	GtkTextIter start;
	gtk_text_buffer_get_start_iter(text_buffer, &start);
	gtk_text_buffer_create_mark(text_buffer, "search-mark", &start, TRUE);

	g_signal_connect(G_OBJECT(search_entry), "changed",
		G_CALLBACK(on_search_entry_changed), text_buffer);

	return search_revealer;
}


gboolean toggle_search_entry(GdkEventKey *key_event)
{
	LOG_MSG("toggle_search_entry()\n");

	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));

	if (tab == NULL) {
		LOG_MSG("toggle_search_entry(): no tabs open, exiting..\n");
		return FALSE;
	}

	GtkWidget *text_view = (GtkWidget *) tab_retrieve_widget(tab, TEXT_VIEW);
	GtkWidget *search_revealer = (GtkWidget *) tab_retrieve_widget(tab, SEARCH_REVEALER);
	GtkWidget *search_entry = (GtkWidget *) tab_retrieve_widget(tab, SEARCH_ENTRY);

	if (gtk_revealer_get_reveal_child(GTK_REVEALER(search_revealer)))
	{
		if (gtk_widget_is_focus(search_entry))
		{
			gtk_revealer_set_reveal_child(GTK_REVEALER(search_revealer), FALSE);
			gtk_widget_grab_focus(text_view);
		}
		else
		{
			gtk_widget_grab_focus(search_entry);
		}
	}
	else
	{
		gtk_revealer_set_reveal_child(GTK_REVEALER(search_revealer), TRUE);
		gtk_widget_grab_focus(search_entry);
	}

	return TRUE;
}


gboolean do_search(GdkEventKey *key_event)
{
	LOG_MSG("do_search()\n");

	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	if (tab == NULL) return FALSE;

	GtkRevealer *search_revealer = GTK_REVEALER(tab_retrieve_widget(tab, SEARCH_REVEALER));
	//GtkWidget *search_entry = gtk_bin_get_child(GTK_BIN(search_revealer));
	GtkWidget *search_entry = (GtkWidget *) tab_retrieve_widget(tab, SEARCH_ENTRY);
	GtkTextView *text_view = (GtkTextView *) tab_retrieve_widget(tab, TEXT_VIEW);
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);

	if (!gtk_widget_is_focus(search_entry)) 
	{
		LOG_MSG("\twidget not (in?) focus.. exiting..\n");
		return FALSE; // we didnt deal with the event that triggered us..
	}

	const char *text = gtk_entry_get_text(GTK_ENTRY(search_entry)); // "... must not be freed, modified or stored."
	if (strlen(text) == 0) {
		return TRUE;
	}

	/* these will be initialized in parse_str(): */
	int line_num;
	char *search_str, *replace_with_str;
	int action_2_take = parse_str(text, &line_num, &search_str, &replace_with_str);
	printf("action_2_take: %s\n", print_action2take(action_2_take));

	switch (action_2_take) {
		case SEARCH:
			printf("search string: %s\n", search_str);
			break;
		case REPLACE:
			printf("search string: %s, replace string: %s\n", search_str, replace_with_str);
			break;
		case GO_TO_LINE:
			printf("line_num: %d\n", line_num);
			break;
	}

	// maybe it would be a better idea to let the user specify the search flags directly
	// because then we would comply with the convention of using documented GTK-stuff in the settings-file
	// instead of arbitrarily inventing our own stuff
	GtkTextSearchFlags search_flags = (GtkTextSearchFlags) 0;
	{
		const char *case_sensitivity = settings_get_value(settings, "search-case-sensitivity");
		assert(case_sensitivity);
		if (strcmp(case_sensitivity, "false") == 0) {
			search_flags = (GtkTextSearchFlags) (((int) search_flags) | GTK_TEXT_SEARCH_CASE_INSENSITIVE);
		}
	}

	if (action_2_take == SEARCH) {
		assert(search_str);
		GtkTextIter search_iter, match_start, match_end;

		GtkTextMark *search_mark = gtk_text_buffer_get_mark(text_buffer, "search-mark");
		assert(search_mark != NULL);
		gtk_text_buffer_get_iter_at_mark(text_buffer, &search_iter, search_mark);

		gboolean found;
		DO_SEARCH:
		found = gtk_text_iter_forward_search(&search_iter, search_str,
			search_flags, &match_start, &match_end, NULL);

		if (found == TRUE) {
			gtk_text_view_scroll_to_iter(text_view, &match_start, 0.0, TRUE, 0.0, 0.5); // middle
//			gtk_text_view_scroll_to_iter(text_view, &match_start, 0.0, TRUE, 0.0, 0.1);
			gtk_text_buffer_select_range(text_buffer, &match_end, &match_start);
			search_iter = match_end;
			gtk_text_buffer_move_mark(text_buffer, search_mark, &search_iter);
		} else {
			if (gtk_text_iter_is_start(&search_iter) == FALSE) {
				LOG_MSG("\tthere were matches for \"%s\" -> back to the beginning\n", text);	
				gtk_text_buffer_get_start_iter(text_buffer, &search_iter);
				goto DO_SEARCH;
			} else {
				LOG_MSG("\tthere were no matches for \"%s\"\n", text);
			}
		}
	} else if (action_2_take == REPLACE) {

		GtkTextIter iter, end;
	
		gboolean selection_exists = gtk_text_buffer_get_selection_bounds(text_buffer, &iter, &end); // gboolean
		if (!selection_exists) {
			return TRUE;
		}

		GtkTextMark *m1 = gtk_text_buffer_create_mark(text_buffer, NULL, &iter, FALSE);
		GtkTextMark *m2 = gtk_text_buffer_create_mark(text_buffer, NULL, &iter, FALSE);
		GtkTextMark *m_end = gtk_text_buffer_create_mark(text_buffer, NULL, &end, FALSE);

		GtkTextIter match_start, match_end;
		while (gtk_text_iter_forward_search(&iter, search_str, 
				search_flags, &match_start, &match_end, &end))
		{
			gtk_text_buffer_move_mark(text_buffer, m1, &match_start);
			gtk_text_buffer_move_mark(text_buffer, m2, &match_end);
			gtk_text_buffer_delete(text_buffer, &match_start, &match_end);
			gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, m1);
			gtk_text_buffer_insert(text_buffer, &iter, replace_with_str, -1);
			gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, m2);
			gtk_text_buffer_get_iter_at_mark(text_buffer, &end, m_end);
		}
	
		gtk_text_buffer_delete_mark(text_buffer, m1);
		gtk_text_buffer_delete_mark(text_buffer, m2);

	} else if (action_2_take == GO_TO_LINE) {
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_line(text_buffer, &iter, line_num - 1); // ...counting from 0 or 1
		gtk_widget_grab_focus(GTK_WIDGET(text_view));
		gtk_text_buffer_place_cursor(text_buffer, &iter);
		gtk_text_view_scroll_to_iter(text_view, &iter, 0.0, FALSE, 0.0, 0.0);
	}

	//@ free everything
	printf("do_search: returning..\n");

	return TRUE;
}