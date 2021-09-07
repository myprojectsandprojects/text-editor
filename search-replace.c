
/*
shift + enter replaces selected text with the text in replace-entry,
so replace entry is a separate feature from search entirely.
(while it is convenient to use them together)
which is why it doesnt make sense to make replace-entry dependent on search-entry..
-- it is definately more convenient than to use search and ctrl+V, because when replacing text with ctrl+V, user needs to switch focus between search-entry and text-view.. but it is merely a matter of convenience and not a unique feature on its own(what is the difference?)

Maybe replace should change all matches at once? I personally have never had any use for such a feature, especially when working with large files, because it's hard to see ahead what will actually get replaced.

Scoped searching (and replacing)?
Searching from the cursor? Forward/backward?

Undo and replacements? Undoing by a (language) token?
*/

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "declarations.h"
#include "tab.h"


extern GtkWidget *notebook;


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
	GtkWidget *replace_revealer = (GtkWidget *) tab_retrieve_widget(tab, REPLACE_REVEALER);

	assert(text_view != NULL && search_revealer != NULL && search_entry != NULL
		&& search_revealer != NULL && search_entry != NULL); // @

	if (gtk_revealer_get_reveal_child(GTK_REVEALER(search_revealer)))
	{
		if (gtk_widget_is_focus(search_entry))
		{
			if (gtk_revealer_get_reveal_child(GTK_REVEALER(replace_revealer)))
			{
				gtk_revealer_set_reveal_child(GTK_REVEALER(replace_revealer), FALSE);
			}
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

gboolean toggle_replace_entry(GdkEventKey *key_event)
{
	LOG_MSG("toggle_replace_entry()\n");

	GtkWidget *text_view = (GtkWidget *) visible_tab_retrieve_widget(
		GTK_NOTEBOOK(notebook), TEXT_VIEW);

	// if we dont have a text-view, we'll assume that no tabs are open, which is a legit condition, so we'll just return:
	if (text_view == NULL)
	{
		LOG_MSG("toggle_search_entry(): no tabs open, exiting..\n");
		return FALSE;
	}

	GtkWidget *replace_revealer = (GtkWidget *) visible_tab_retrieve_widget(
		GTK_NOTEBOOK(notebook), REPLACE_REVEALER);
	GtkWidget *replace_entry = (GtkWidget *) visible_tab_retrieve_widget(
		GTK_NOTEBOOK(notebook), REPLACE_ENTRY);
	GtkWidget *search_revealer = (GtkWidget *) visible_tab_retrieve_widget(
		GTK_NOTEBOOK(notebook), SEARCH_REVEALER);
	GtkWidget *search_entry = (GtkWidget *) visible_tab_retrieve_widget(
		GTK_NOTEBOOK(notebook), SEARCH_ENTRY);

	// if we have a text-view, but not these, we dont know what is going on:
	assert(replace_revealer != NULL && replace_entry != NULL
		&& search_revealer != NULL && search_entry != NULL);


	if (gtk_revealer_get_reveal_child(GTK_REVEALER(replace_revealer)))
	{
		if (gtk_widget_is_focus(replace_entry))
		{
			gtk_revealer_set_reveal_child(GTK_REVEALER(replace_revealer), FALSE);
			gtk_widget_grab_focus(search_entry);
		}
		else
		{
			gtk_widget_grab_focus(replace_entry);
		}
	}
	else
	{
		if (gtk_revealer_get_reveal_child(GTK_REVEALER(search_revealer)))
		{
			gtk_revealer_set_reveal_child(GTK_REVEALER(replace_revealer), TRUE);
			gtk_widget_grab_focus(replace_entry);
		}
	}

	return TRUE;
}
/*
static void go_to_next_match(GtkTextView *view, const char *search_phrase)
{
	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GtkTextIter iter, match_start, match_end;
	gboolean found;

	buffer = gtk_text_view_get_buffer(view);
	mark = gtk_text_buffer_get_mark(buffer, "search");
	assert(mark != NULL);
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

	DO_SEARCH:
	found = gtk_text_iter_forward_search(&iter, search_phrase,
		GTK_TEXT_SEARCH_CASE_INSENSITIVE,
		&match_start, &match_end, NULL);

	if (found == TRUE)
	{
		gtk_text_view_scroll_to_iter(view, &match_start, 0.0, FALSE, 0.0, 0.0);
		gtk_text_buffer_select_range(buffer, &match_end, &match_start);
		iter = match_end;
		gtk_text_buffer_move_mark(buffer, mark, &iter);
	}
	else
	{
		if(gtk_text_iter_is_start(&iter) == FALSE)
		{
			printf("search: there were matches for \"%s\" -> back to the beginning\n", search_phrase);	
			gtk_text_buffer_get_start_iter(buffer, &iter);
			goto DO_SEARCH;
		}
		else
		{
			printf("search: there were no matches for \"%s\"\n", search_phrase);
		}
	}
}
*/
/*
	Let's try the following:

	if search-entry or replace entry are focus:
	enter -> move to the next match, select it
	shift + enter -> replaces selected text with what is in the replace-entry, if empty, then deletes the selection.
*/

gboolean replace_selected_text(GdkEventKey *key_event)
{
	printf("replace_selected_text()\n");

	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	if (tab == NULL)
	{
		return FALSE; // No tabs open..
	}

	GtkWidget *replace_entry = (GtkWidget *) tab_retrieve_widget(tab, REPLACE_ENTRY);
	GtkTextBuffer *buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);

	GtkTextIter s_start, s_end;
	gboolean s = gtk_text_buffer_get_selection_bounds(buffer, &s_start, &s_end);
	if (!s) return FALSE;
	
	gtk_text_buffer_delete(buffer, &s_start, &s_end);
	const char *replacement_phrase = gtk_entry_get_text(GTK_ENTRY(replace_entry));
	gtk_text_buffer_insert(buffer, &s_start, replacement_phrase, -1);

	return TRUE;
}

/*
bounds = scope of the cursor position
*/

/* - bounds are not dependent on text buffer contents.. */
GtkTextMark *m_bounds_start, *m_bounds_end, *m_search_pointer;
gboolean is_new_search = TRUE;
/*
void search_and_highlight_next_match(GtkTextView *view)
{
	printf("search_and_highlight_next_match()\n");
}
*/
static void determine_bounds(GtkTextView *view)
{
	printf("determine_bounds()\n");

	GtkTextBuffer *buffer;

	assert(m_bounds_start != NULL && m_bounds_end != NULL);

	buffer = gtk_text_view_get_buffer(view);

	// determine bounds
	// get iter at cursor
	GtkTextIter i;
	GtkTextMark *m_cursor;
	m_cursor = gtk_text_buffer_get_mark(buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(buffer, &i, m_cursor);
	//printf("search_and_highlight_first_match(): cursor offset: %d\n", gtk_text_iter_get_offset(&i));

	// determine the beginning
	int count = 0;
	while (gtk_text_iter_backward_char(&i))
	{
		gunichar c = gtk_text_iter_get_char(&i);
		if (c == '}')
			count += 1;
		else if (c == '{')
		{
			if (count > 0)
				count -= 1;
			else // count == 0
				break;
		}
	}
	gtk_text_buffer_move_mark(buffer, m_bounds_start, &i);

	// determine the end
	count = 0;
	while (gtk_text_iter_forward_char(&i))
	{
		gunichar c = gtk_text_iter_get_char(&i);
		if (c == '{')
			count += 1;
		else if (c == '}')
		{
			if (count > 0)
				count -= 1;
			else // count == 0
				break;
		}
	}
	gtk_text_buffer_move_mark(buffer, m_bounds_end, &i);

	//GtkTextIter i_bounds_start, i_bounds_end;
	//gtk_text_buffer_get_iter_at_mark(buffer, &i_bounds_start, m_bounds_start);
	//gtk_text_buffer_get_iter_at_mark(buffer, &i_bounds_end, m_bounds_end);
	//char *text = gtk_text_buffer_get_text(buffer, &i_bounds_start, &i_bounds_end, FALSE);
	//printf("determine_bounds(): text: \n%s\n\n", text);
}
/*
void search_and_highlight_first_match(GtkTextView *view, char *search_phrase)
{
	GtkTextIter i_start, i_end;
	char sp_buffer[100];

	printf("search_and_highlight_first_match()\n");

	is_new_search = FALSE;

	if (search_phrase[0] == ':')
	{
		snprintf(sp_buffer, 100, "%s", &search_phrase[1]);

		determine_bounds(view);
	}
	else
	{
		snprintf(sp_buffer, 100, "%s", &search_phrase[0]);

		gtk_text_buffer_get_bounds(buffer, &i_start, &i_end);
	}
	//free(search_phrase);
	search_phrase = sp_buffer;

	search_and_highlight_next_match(view, sp_buffer);
}
*/
/*
This is where the action happens.
When search-entry is focus we find and highlight the next match
and when replace-entry is focus we replace all matches in the file.
*/
gboolean on_search_and_replace(void)
{
	printf("on_search_and_replace()\n");

	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	// what are you smoking?: assert(tab != NULL); // it's arguable, but we'll put an assert here..
	if (tab == NULL) return FALSE;

	GtkWidget *search_entry = (GtkWidget *) tab_retrieve_widget(tab, SEARCH_ENTRY);
	GtkWidget *search_revealer = (GtkWidget *) tab_retrieve_widget(tab, SEARCH_REVEALER);
	GtkWidget *replace_entry = (GtkWidget *) tab_retrieve_widget(tab, REPLACE_ENTRY);
	GtkWidget *replace_revealer = (GtkWidget *) tab_retrieve_widget(tab, REPLACE_REVEALER);
	GtkWidget *view = (GtkWidget *) tab_retrieve_widget(tab, TEXT_VIEW);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	//int *p_next_action = (int *) tab_retrieve_widget(tab, REPLACE_NEXT_ACTION);

	//@ should assert

	if ( !(gtk_widget_is_focus(search_entry) || gtk_widget_is_focus(replace_entry)) ) 
	{
		printf("on_search_and_replace(): widgets not (in?) focus.. exiting..\n");
		return FALSE; // we didnt deal with the event that triggered us..
	}

	const char *search_phrase = gtk_entry_get_text(GTK_ENTRY(search_entry));
	if (strlen(search_phrase) < 1)
	{
		printf("on_search_and_replace(): no search phrase.. exiting..\n");
		return TRUE; // we dealt with the event that triggered us..
	}

	char sp_buffer[100];
	if (search_phrase[0] == ':')
	{
		snprintf(sp_buffer, 100, "%s", &search_phrase[1]);
	}
	else
	{
		snprintf(sp_buffer, 100, "%s", &search_phrase[0]);
	}

	if (is_new_search)
	{
		printf("on_search_and_replace(): starting new search!\n");

		if (search_phrase[0] == ':')
		{
			determine_bounds(GTK_TEXT_VIEW(view));
		}
		else
		{
			GtkTextIter i_start, i_end;
			gtk_text_buffer_get_bounds(buffer, &i_start, &i_end);
			gtk_text_buffer_move_mark(buffer, m_bounds_start, &i_start);
			gtk_text_buffer_move_mark(buffer, m_bounds_end, &i_end);
		}

		// move search-pointer to the beginning of the scope
		GtkTextIter i;
		gtk_text_buffer_get_iter_at_mark(buffer, &i, m_bounds_start);
		gtk_text_buffer_move_mark(buffer, m_search_pointer, &i);

		is_new_search = FALSE;
	}
		
	//free(search_phrase);
	search_phrase = sp_buffer;

	if (gtk_widget_is_focus(search_entry))
	{
		GtkTextIter iter, i_bounds_start, i_bounds_end, match_start, match_end;
		gboolean found;

		gtk_text_buffer_get_iter_at_mark(buffer, &iter, m_search_pointer);
		gtk_text_buffer_get_iter_at_mark(buffer, &i_bounds_start, m_bounds_start);
		gtk_text_buffer_get_iter_at_mark(buffer, &i_bounds_end, m_bounds_end);
	
		DO_SEARCH:
		printf("searching for \"%s\"\n", search_phrase);
		char *text = gtk_text_buffer_get_text(buffer, &iter, &i_bounds_end, FALSE);
		printf("in: \n%s\n", text);

		found = gtk_text_iter_forward_search(&iter, search_phrase,
			GTK_TEXT_SEARCH_CASE_INSENSITIVE,
			&match_start, &match_end, &i_bounds_end);
	
		if (found == TRUE)
		{
			gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(view), &match_start, 0.0, FALSE, 0.0, 0.0);
			gtk_text_buffer_select_range(buffer, &match_end, &match_start);
			iter = match_end;
			gtk_text_buffer_move_mark(buffer, m_search_pointer, &iter);
		}
		else
		{
			if(gtk_text_iter_compare(&iter, &i_bounds_start) != 0)
			{
				printf("search: there were matches for \"%s\" -> back to the beginning\n", search_phrase);	
				//gtk_text_buffer_get_start_iter(buffer, &iter);
				gtk_text_buffer_get_iter_at_mark(buffer, &iter, m_bounds_start);
				goto DO_SEARCH;
			}
			else
			{
				printf("search: there were no matches for \"%s\"\n", search_phrase);
			}
		}
	}
	else if (gtk_widget_is_focus(replace_entry))
	{
		GtkTextIter search, i_bounds_start, i_bounds_end;

		//gtk_text_buffer_get_iter_at_mark(buffer, &iter, m_search_pointer);
		gtk_text_buffer_get_iter_at_mark(buffer, &i_bounds_start, m_bounds_start);
		gtk_text_buffer_get_iter_at_mark(buffer, &i_bounds_end, m_bounds_end);

		search = i_bounds_start;

		const char *replace_phrase = gtk_entry_get_text(GTK_ENTRY(replace_entry));
		printf("on_search_and_replace(): replaceing \"%s\" with \"%s\"\n", search_phrase, replace_phrase);

		GtkTextMark *m1 = gtk_text_buffer_create_mark(buffer, NULL, &i_bounds_start, FALSE);
		GtkTextMark *m2 = gtk_text_buffer_create_mark(buffer, NULL, &i_bounds_start, FALSE); // doesnt matter where they are in the beginning
	
		GtkTextIter match_start, match_end;
		while (gtk_text_iter_forward_search(&search, search_phrase, 
				GTK_TEXT_SEARCH_CASE_INSENSITIVE, &match_start, &match_end, &i_bounds_end))
		{
			gtk_text_buffer_move_mark(buffer, m1, &match_start);
			gtk_text_buffer_move_mark(buffer, m2, &match_end);
			gtk_text_buffer_delete(buffer, &match_start, &match_end);
			gtk_text_buffer_get_iter_at_mark(buffer, &search, m1);
			gtk_text_buffer_insert(buffer, &search, replace_phrase, -1);
			gtk_text_buffer_get_iter_at_mark(buffer, &search, m2);
			//gtk_text_buffer_get_end_iter(buffer, &end);
			gtk_text_buffer_get_iter_at_mark(buffer, &i_bounds_end, m_bounds_end);
		}
	
		gtk_text_buffer_delete_mark(buffer, m1);
		gtk_text_buffer_delete_mark(buffer, m2);
	}

/*
	GtkTextIter bounds_start, bounds_end;
	gtk_text_buffer_get_bounds(buffer, &bounds_start, &bounds_end);

	char sp_buffer[100];
	{ // Figure out what the bounds of the search are:

		GtkTextIter i, match_start, match_end;

		if (search_phrase[0] == '{')
		{
			snprintf(sp_buffer, 100, "%s", &search_phrase[1]);
	
			GtkTextMark *m_cursor = gtk_text_buffer_get_mark(buffer, "insert");
			gtk_text_buffer_get_iter_at_mark(buffer, &i, m_cursor);
			gboolean found = gtk_text_iter_backward_search(&i, "{", GTK_TEXT_SEARCH_CASE_INSENSITIVE, &match_start, &match_end, NULL);
			if (found)
			{
				bounds_start = match_start;
				i = bounds_start;
				// just the first closing-1 for now..:
				found = gtk_text_iter_forward_search(&i, "}", GTK_TEXT_SEARCH_CASE_INSENSITIVE, &match_start, &match_end, NULL);
				if (found)
				{
					//bounds_end = match_end;
					bounds_end = match_start; // ?
				}
			}
		}
		else
		{
			snprintf(sp_buffer, 100, "%s", &search_phrase[0]);
		}
		//free(search_phrase);
		search_phrase = sp_buffer;
	}


	if (gtk_widget_is_focus(search_entry)) 
	{
		GtkTextMark *mark;
		GtkTextIter iter, match_start, match_end;
		gboolean found;

		//printf("on_search_and_replace(): should search\n");

		mark = gtk_text_buffer_get_mark(buffer, "search");
		assert(mark != NULL);
		gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

		int o_search = gtk_text_iter_get_offset(&iter);
		int o_bounds_start = gtk_text_iter_get_offset(&bounds_start);
		int o_bounds_end = gtk_text_iter_get_offset(&bounds_end);
		if (o_search < o_bounds_start || o_search > o_bounds_end)
		{
			iter = bounds_start;
		}

		DO_SEARCH:
		found = gtk_text_iter_forward_search(&iter, search_phrase,
			GTK_TEXT_SEARCH_CASE_INSENSITIVE,
			&match_start, &match_end, &bounds_end);

		if (found == TRUE)
		{
			gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(view), &match_start, 0.0, FALSE, 0.0, 0.0);
			gtk_text_buffer_select_range(buffer, &match_end, &match_start);
			iter = match_end;
			gtk_text_buffer_move_mark(buffer, mark, &iter);
		}
		else
		{
			//if(gtk_text_iter_is_start(&iter) == FALSE)
			int o_iter = gtk_text_iter_get_offset(&iter);
			int o_bounds_start = gtk_text_iter_get_offset(&bounds_start);
			if(o_iter != o_bounds_start)
			{
				printf("search: there were matches for \"%s\" -> back to the beginning\n", search_phrase);	
				//gtk_text_buffer_get_start_iter(buffer, &iter);
				iter = bounds_start;
				goto DO_SEARCH;
			}
			else
			{
				printf("search: there were no matches for \"%s\"\n", search_phrase);
			}
		}

		return TRUE;
	}
*/
/*
	else if (gtk_widget_is_focus(replace_entry))
	{
		//printf("on_search_and_replace(): should search & replace\n");
		const char *replace_phrase = gtk_entry_get_text(GTK_ENTRY(replace_entry));
		printf("on_search_and_replace(): replaceing \"%s\" with \"%s\"\n", search_phrase, replace_phrase);

		GtkTextIter search, start, end;

		//gtk_text_buffer_get_selection_bounds(text_buffer, &start, &end);
		gtk_text_buffer_get_bounds(buffer, &start, &end);
		search = start;
		GtkTextMark *m1 = gtk_text_buffer_create_mark(buffer, NULL, &start, FALSE);
		GtkTextMark *m2 = gtk_text_buffer_create_mark(buffer, NULL, &start, FALSE); // doesnt matter where they are in the beginning
	
		GtkTextIter match_start, match_end;
		while (gtk_text_iter_forward_search(&search, search_phrase, GTK_TEXT_SEARCH_CASE_INSENSITIVE, &match_start, &match_end, &end)) {
			gtk_text_buffer_move_mark(buffer, m1, &match_start);
			gtk_text_buffer_move_mark(buffer, m2, &match_end);
			gtk_text_buffer_delete(buffer, &match_start, &match_end);
			gtk_text_buffer_get_iter_at_mark(buffer, &search, m1);
			gtk_text_buffer_insert(buffer, &search, replace_phrase, -1);
			gtk_text_buffer_get_iter_at_mark(buffer, &search, m2);
			gtk_text_buffer_get_end_iter(buffer, &end);
		}
	
		gtk_text_buffer_delete_mark(buffer, m1);
		gtk_text_buffer_delete_mark(buffer, m2);

		return TRUE;
	}*/


	return TRUE;
}

void on_search_entry_changed(GtkEditable *search_entry, gpointer data)
{
	GtkTextBuffer *buffer;
	GtkTextIter start;

	is_new_search = TRUE;

	buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);
	assert(buffer != NULL);
	gtk_text_buffer_get_start_iter(buffer, &start);

	if (!m_search_pointer)
	{
		printf("CREATING THE MARKS\n");
		m_search_pointer = gtk_text_buffer_create_mark(buffer, NULL, &start, TRUE);
		m_bounds_start = gtk_text_buffer_create_mark(buffer, NULL, &start, TRUE);
		m_bounds_end = gtk_text_buffer_create_mark(buffer, NULL, &start, TRUE);
	}
/*
	GtkTextMark *mark = gtk_text_buffer_get_mark(buffer, "search");
	if (!mark)
	{
		LOG_MSG("on_search_entry_changed(): no mark.. creating one..\n");
		GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, "search", &start, TRUE);
	}
	else
	{
		LOG_MSG("on_search_entry_changed(): we have a mark..\n");
		gtk_text_buffer_move_mark_by_name(buffer, "search", &start);
	}
*/
}

GtkWidget *create_search_and_replace_widget(GtkWidget *tab)
{
	printf("create_search_and_replace_widget()\n");

	GtkWidget *search_revealer = gtk_revealer_new();
	//gtk_revealer_set_reveal_child(GTK_REVEALER(search_revealer), TRUE);
	GtkWidget *replace_revealer = gtk_revealer_new();
	//gtk_revealer_set_reveal_child(GTK_REVEALER(replace_revealer), TRUE);

	GtkWidget *search_entry = gtk_search_entry_new();
	GtkWidget *replace_entry = gtk_entry_new();

	GtkWidget *container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);

	gtk_container_add(GTK_CONTAINER(search_revealer), search_entry);
	gtk_container_add(GTK_CONTAINER(replace_revealer), replace_entry);
	gtk_container_add(GTK_CONTAINER(container), search_revealer);
	gtk_container_add(GTK_CONTAINER(container), replace_revealer);

	tab_add_widget_4_retrieval(tab, SEARCH_REVEALER, search_revealer);
	tab_add_widget_4_retrieval(tab, SEARCH_ENTRY, search_entry);
	tab_add_widget_4_retrieval(tab, REPLACE_REVEALER, replace_revealer);
	tab_add_widget_4_retrieval(tab, REPLACE_ENTRY, replace_entry);

	gtk_widget_set_name(search_entry, "search-entry");
	gtk_widget_set_name(replace_entry, "replace-entry");
	add_class(search_entry, "text-entry-deepskyblue");
	add_class(replace_entry, "text-entry-limegreen");

	g_signal_connect(G_OBJECT(search_entry), "changed",
		G_CALLBACK(on_search_entry_changed), NULL);

	// We could observe cursor-position property and be up-to-date in terms of scope at all times
	// and then, when doing the search, just search in that scope.
	// But we dont really know if the text-buffer is available for us at the time we are called
	// and we dont want to set strict rules as to when exactly we are suppose to be called..
	// Maybe it would be nice to have an event which is fired once the tab is fully created..
	// Or maybe it doesnt matter at all if we enforce strict order in which things should be initialized?
	// or maybe we should export an event handler?
	// or maybe we should register our callback at the time when search-entry receives its 1st changed-signal?
	/*
	g_signal_connect(G_OBJECT(text_buffer), "notify::cursor-position",
		G_CALLBACK(text_buffer_cursor_position_changed), line_nr_value);
	*/

	return container;
}

/*
GtkWidget *create_search_and_replace_widget(GtkWidget *tab)
{
	printf("create_search_and_replace_widget()\n");

	GtkWidget *search_revealer = gtk_revealer_new();
	gtk_revealer_set_reveal_child(GTK_REVEALER(search_revealer), TRUE);

	GtkWidget *replace_revealer = gtk_revealer_new();
	gtk_revealer_set_reveal_child(GTK_REVEALER(replace_revealer), TRUE);
	gtk_revealer_set_transition_type(GTK_REVEALER(replace_revealer),
		GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);

	GtkWidget *search_entry = gtk_search_entry_new();
	GtkWidget *replace_entry = gtk_entry_new();

	gtk_widget_set_hexpand(search_revealer, TRUE);
	gtk_widget_set_hexpand(replace_revealer, TRUE);
	gtk_widget_set_hexpand(search_entry, TRUE);
	gtk_widget_set_hexpand(replace_entry, TRUE);

	GtkWidget *container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	gtk_container_add(GTK_CONTAINER(search_revealer), container);
	gtk_container_add(GTK_CONTAINER(container), search_entry);
	gtk_container_add(GTK_CONTAINER(container), replace_revealer);
	gtk_container_add(GTK_CONTAINER(replace_revealer), replace_entry);

	tab_add_widget_4_retrieval(tab, SEARCH_REVEALER, search_revealer);
	tab_add_widget_4_retrieval(tab, SEARCH_ENTRY, search_entry);
	tab_add_widget_4_retrieval(tab, REPLACE_REVEALER, replace_revealer);
	tab_add_widget_4_retrieval(tab, REPLACE_ENTRY, replace_entry);

	gtk_widget_set_name(search_entry, "search-entry");
	gtk_widget_set_name(replace_entry, "replace-entry");
	add_class(search_entry, "text-entry");
	add_class(replace_entry, "text-entry");

	return search_revealer;
}
*/