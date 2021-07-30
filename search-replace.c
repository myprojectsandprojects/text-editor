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
		&& search_revealer != NULL && search_entry != NULL);

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
	assert(s);
	
	gtk_text_buffer_delete(buffer, &s_start, &s_end);
	const char *replacement_phrase = gtk_entry_get_text(GTK_ENTRY(replace_entry));
	gtk_text_buffer_insert(buffer, &s_start, replacement_phrase, -1);

	return TRUE;
}

gboolean search(void)
{
	printf("search()\n");

	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	assert(tab != NULL); // it's arguable, but we'll put an assert here..

	GtkWidget *search_entry = (GtkWidget *) tab_retrieve_widget(tab, SEARCH_ENTRY);
	GtkWidget *search_revealer = (GtkWidget *) tab_retrieve_widget(tab, SEARCH_REVEALER);
	GtkWidget *replace_entry = (GtkWidget *) tab_retrieve_widget(tab, REPLACE_ENTRY);
	GtkWidget *replace_revealer = (GtkWidget *) tab_retrieve_widget(tab, REPLACE_REVEALER);
	GtkWidget *text_view = (GtkWidget *) tab_retrieve_widget(tab, TEXT_VIEW);
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	//int *p_next_action = (int *) tab_retrieve_widget(tab, REPLACE_NEXT_ACTION);

	const char *search_phrase = gtk_entry_get_text(GTK_ENTRY(search_entry));
	if (strlen(search_phrase) < 1)
	{
		printf("search(): no search phrase.. exiting..\n");
		return FALSE;
	}

	if ((gtk_widget_is_focus(search_entry) || gtk_widget_is_focus(replace_entry))) 
	{
		go_to_next_match(GTK_TEXT_VIEW(text_view), search_phrase);
		return TRUE;
	}

	return FALSE;
}

void on_search_entry_changed(GtkEditable *search_entry, gpointer data)
{
	GtkTextBuffer *buffer;
	GtkTextIter start;

	buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);
	assert(buffer != NULL);
	gtk_text_buffer_get_start_iter(buffer, &start);

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

	g_signal_connect(G_OBJECT(search_entry), "changed", G_CALLBACK(on_search_entry_changed), NULL);

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