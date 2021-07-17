#include <stdio.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <string.h>

#include "tab.h"
#include "declarations.h"

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

void init_search(GtkWidget *tab)
{
	LOG_MSG("init_search() called\n");

	GtkRevealer *search_revealer = GTK_REVEALER(tab_retrieve_widget(tab, SEARCH_REVEALER));
	GtkEntry *search_entry = GTK_ENTRY(tab_retrieve_widget(tab, SEARCH_ENTRY));
	GtkTextView *text_view = GTK_TEXT_VIEW(tab_retrieve_widget(tab, TEXT_VIEW));
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));

	GtkTextIter start;
	gtk_text_buffer_get_start_iter(text_buffer, &start);
	gtk_text_buffer_create_mark(text_buffer, "search-mark", &start, TRUE);

	g_signal_connect(G_OBJECT(search_entry), "changed", G_CALLBACK(on_search_entry_changed), text_buffer);
}

void do_search(GtkWidget *tab)
{
	printf("do_search() called!\n");

	GtkRevealer *search_revealer = GTK_REVEALER(tab_retrieve_widget(tab, SEARCH_REVEALER));
	//GtkWidget *search_entry = gtk_bin_get_child(GTK_BIN(search_revealer));
	GtkWidget *search_entry = tab_retrieve_widget(tab, SEARCH_ENTRY);
	GtkTextView *text_view = (GtkTextView *) tab_retrieve_widget(tab, TEXT_VIEW);
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);

	const char *text = gtk_entry_get_text(GTK_ENTRY(search_entry)); //@ free?
	if(strlen(text) == 0) {
		return;
	}

	g_print("search: searching \"%s\"\n", text);

	GtkTextIter search_iter, match_start, match_end;
	gboolean found;

	GtkTextMark *search_mark = gtk_text_buffer_get_mark(text_buffer, "search-mark");
	assert(search_mark != NULL);
	gtk_text_buffer_get_iter_at_mark(text_buffer, &search_iter, search_mark);

	DO_SEARCH:
	found = gtk_text_iter_forward_search(&search_iter, text, GTK_TEXT_SEARCH_CASE_INSENSITIVE, &match_start, &match_end, NULL);
	if (found == TRUE) {
		gtk_text_view_scroll_to_iter(text_view, &match_start, 0.0, FALSE, 0.0, 0.0);
		gtk_text_buffer_select_range(text_buffer, &match_end, &match_start);
		search_iter = match_end;
		gtk_text_buffer_move_mark(text_buffer, search_mark, &search_iter);
	} else {
		if(gtk_text_iter_is_start(&search_iter) == FALSE) {
			g_print("search: there were matches for \"%s\" -> back to the beginning\n", text);	
			gtk_text_buffer_get_start_iter(text_buffer, &search_iter);
			goto DO_SEARCH;
		} else {
			g_print("search: there were no matches for \"%s\"\n", text);
		}
	}
}








