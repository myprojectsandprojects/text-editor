/*
autocomplete.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <gtk/gtk.h>
#include "tab.h"
#include "declarations.h"

int reading = 0;
int i = 0;
char word[100];
int j = 0;
char *words[100];

extern GtkWidget *window;

void on_popup_window_key_press_event()
{
	LOG_MSG("on_popup_window_key_press_event()\n");
}

void create_popup(GtkTextView *text_view, GtkTextIter *location)
{
	GdkRectangle rect;
	gint x, y, o_x, o_y;
	GdkWindow *w = gtk_widget_get_window(GTK_WIDGET(text_view));

	gtk_text_view_get_iter_location(text_view, location, &rect);
	printf("x: %d, y: %d\n", rect.x, rect.y);
	gtk_text_view_buffer_to_window_coords(text_view, GTK_TEXT_WINDOW_WIDGET, rect.x, rect.y, &x, &y);
	printf("x: %d, y: %d\n", x, y);
	gdk_window_get_origin(w, &o_x, &o_y);

	GtkWidget *popup_window = gtk_window_new(GTK_WINDOW_POPUP);
	g_signal_connect(popup_window, "key-press-event", G_CALLBACK(on_popup_window_key_press_event), NULL);
	gtk_style_context_add_class (gtk_widget_get_style_context(popup_window), "suggestions-popup");
	GtkWidget *test_label1 = gtk_label_new("Hello world!");
	GtkWidget *test_label2 = gtk_label_new("Hello universe!");
	GtkWidget *test_label3 = gtk_label_new("Hello multiverse!");
	GtkWidget *suggestions = gtk_list_box_new();
	gtk_list_box_insert(GTK_LIST_BOX(suggestions), test_label1, -1);
	gtk_list_box_insert(GTK_LIST_BOX(suggestions), test_label2, -1);
	gtk_list_box_insert(GTK_LIST_BOX(suggestions), test_label3, -1);
	gtk_container_add(GTK_CONTAINER(popup_window), suggestions);
	//gtk_window_set_decorated(GTK_WINDOW(popup_window), FALSE);
	//gtk_window_set_position(GTK_WINDOW(popup_window), GTK_WIN_POS_CENTER);
	//gtk_window_set_position(GTK_WINDOW(popup_window), GTK_WIN_POS_MOUSE);
	//gtk_window_set_transient_for(GTK_WINDOW(popup_window), GTK_WINDOW(window));
	gtk_window_set_attached_to(GTK_WINDOW(popup_window), window);
	gtk_window_move(GTK_WINDOW(popup_window), x + o_x, y + o_y + 20);
	gtk_widget_show_all(popup_window);
}

void text_buffer_insert_text_4_autocomplete(
	GtkTextBuffer *buffer,
	GtkTextIter *location,
	char *text,
	int len,
	gpointer user_data)
{
	printf("text_buffer_insert_text_4_autocomplete() called\n");
	//printf("text_buffer_insert_text_4_autocomplete(): %s\n", text);

	GtkTextView *text_view = (GtkTextView *) user_data;

	if (text[0] == 'x') {
		printf("You typed X!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		create_popup(text_view, location);
		
	}
	return;

	assert(strlen(text) == 1);

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
}

void init_autocomplete(GtkWidget *tab)
{
	LOG_MSG("init_autocomplete()\n");

	GObject *text_buffer = (GObject *) tab_retrieve_widget(tab, TEXT_BUFFER);
	void *text_view = tab_retrieve_widget(tab, TEXT_VIEW);
	g_signal_connect(text_buffer, "insert-text", G_CALLBACK(text_buffer_insert_text_4_autocomplete), text_view);
}





