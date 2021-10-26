#include <stdio.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "tab.h"
#include "declarations.h"


extern GtkWidget *notebook;


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
	printf("create_search_widget()\n");

	GtkWidget *search_entry = gtk_search_entry_new();
	GtkWidget *search_revealer = gtk_revealer_new();
	//gtk_revealer_set_reveal_child(GTK_REVEALER(search_revealer), TRUE);

	gtk_container_add(GTK_CONTAINER(search_revealer), search_entry);

	tab_add_widget_4_retrieval(tab, SEARCH_REVEALER, search_revealer);
	tab_add_widget_4_retrieval(tab, SEARCH_ENTRY, search_entry);

	gtk_widget_set_name(search_entry, "search-entry");
	add_class(search_entry, "text-entry-deepskyblue");

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
	//LOG_MSG("toggle_search_entry()\n");
	printf("toggle_search_entry()\n");

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
	printf("do_search()\n");

	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	if (tab == NULL) return FALSE;

	GtkRevealer *search_revealer = GTK_REVEALER(tab_retrieve_widget(tab, SEARCH_REVEALER));
	//GtkWidget *search_entry = gtk_bin_get_child(GTK_BIN(search_revealer));
	GtkWidget *search_entry = tab_retrieve_widget(tab, SEARCH_ENTRY);
	GtkTextView *text_view = (GtkTextView *) tab_retrieve_widget(tab, TEXT_VIEW);
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);

	if (!gtk_widget_is_focus(search_entry)) 
	{
		printf("do_search(): widget not (in?) focus.. exiting..\n");
		return FALSE; // we didnt deal with the event that triggered us..
	}

	const char *text = gtk_entry_get_text(GTK_ENTRY(search_entry)); //@ free?
	if (strlen(text) == 0) {
		return TRUE;
	}

	if (text[0] == ':') {
		printf("search phrase is a command...\n");

		int line_number = atoi(&text[1]);
		//@ Should check if valid line number maybe...
		printf("atoi() returned: %d\n", line_number);

		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_line(text_buffer, &iter, line_number - 1); // ...counting from 0 or 1
		gtk_widget_grab_focus(GTK_WIDGET(text_view));
		gtk_text_buffer_place_cursor(text_buffer, &iter);
		gtk_text_view_scroll_to_iter(text_view, &iter, 0.0, FALSE, 0.0, 0.0);

		return TRUE;
	}

	printf("search: searching \"%s\"\n", text);

	GtkTextIter search_iter, match_start, match_end;

	GtkTextMark *search_mark = gtk_text_buffer_get_mark(text_buffer, "search-mark");
	assert(search_mark != NULL);
	gtk_text_buffer_get_iter_at_mark(text_buffer, &search_iter, search_mark);

	gboolean found;
	DO_SEARCH:
	found = gtk_text_iter_forward_search(&search_iter, text,
		GTK_TEXT_SEARCH_CASE_INSENSITIVE, &match_start, &match_end, NULL);

	if (found == TRUE) {
		gtk_text_view_scroll_to_iter(text_view, &match_start, 0.0, FALSE, 0.0, 0.0);
		gtk_text_buffer_select_range(text_buffer, &match_end, &match_start);
		search_iter = match_end;
		gtk_text_buffer_move_mark(text_buffer, search_mark, &search_iter);
	} else {
		if (gtk_text_iter_is_start(&search_iter) == FALSE) {
			printf("search: there were matches for \"%s\" -> back to the beginning\n", text);	
			gtk_text_buffer_get_start_iter(text_buffer, &search_iter);
			goto DO_SEARCH;
		} else {
			printf("search: there were no matches for \"%s\"\n", text);
		}
	}

	return TRUE;
}








