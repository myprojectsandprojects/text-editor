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

//extern GtkWidget *window;
GtkWidget					*_suggestions_window;
GtkApplicationWindow		*_app_window;

gboolean autocomplete_on_window_key_press(GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	#define ESCAPE 9
	#define UP 111
	#define DOWN 116
	#define ENTER 36

	GdkEventKey *key_event = (GdkEventKey *) event;
	guint16 keycode = key_event->hardware_keycode;
	LOG_MSG("autocomplete_on_window_key_press(): keycode: %d\n", keycode);

	if (_suggestions_window != NULL) {
		printf("We have a suggestions window!\n");
		if (keycode == ESCAPE) {
			gtk_widget_destroy(_suggestions_window);
			_suggestions_window = NULL;
		}
	} else {
		printf("No suggestions window!\n");
	}

	return FALSE;
}

void create_suggestions_window(GtkTextView *text_view, GtkTextIter *location)
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
	//GtkWidget *popup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	//g_signal_connect(popup_window, "key-press-event", G_CALLBACK(on_popup_window_key_press_event), NULL);
	gtk_style_context_add_class (gtk_widget_get_style_context(_suggestions_window), "suggestions-popup");
	GtkWidget *test_label1 = gtk_label_new("Hello world!");
	GtkWidget *test_label2 = gtk_label_new("Hello universe!");
	GtkWidget *test_label3 = gtk_label_new("Hello multiverse!");
	GtkWidget *suggestions = gtk_list_box_new();
	gtk_list_box_insert(GTK_LIST_BOX(suggestions), test_label1, -1);
	gtk_list_box_insert(GTK_LIST_BOX(suggestions), test_label2, -1);
	gtk_list_box_insert(GTK_LIST_BOX(suggestions), test_label3, -1);
	gtk_container_add(GTK_CONTAINER(_suggestions_window), suggestions);
	//gtk_window_set_decorated(GTK_WINDOW(popup_window), FALSE);
	//gtk_window_set_position(GTK_WINDOW(popup_window), GTK_WIN_POS_CENTER);
	//gtk_window_set_position(GTK_WINDOW(popup_window), GTK_WIN_POS_MOUSE);
	//gtk_window_set_transient_for(GTK_WINDOW(popup_window), GTK_WINDOW(window));
	gtk_window_set_attached_to(GTK_WINDOW(_suggestions_window), GTK_WIDGET(_app_window));
	gtk_window_move(GTK_WINDOW(_suggestions_window), x + o_x, y + o_y + 20);
	gtk_widget_show_all(_suggestions_window);
}

void
autocomplete_on_text_buffer_insert_text(GtkTextBuffer *buffer, GtkTextIter *location,
												char *text, int len, gpointer user_data)
{
	printf("autocomplete_on_text_buffer_insert_text() called\n");
	//printf("text_buffer_insert_text_4_autocomplete(): %s\n", text);

	GtkTextView *text_view = (GtkTextView *) user_data;

	if (text[0] == 'x') {
		create_suggestions_window(text_view, location);
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

void init_autocomplete(GtkApplicationWindow *app_window,
							GtkTextView *text_view,
							GtkTextBuffer *text_buffer)
{
	printf("init_autocomplete()\n");

	_app_window = app_window;
/*
	GObject 	*text_buffer 	= (GObject *) tab_retrieve_widget(tab, TEXT_BUFFER);
	void 		*text_view 		= (gpointer) tab_retrieve_widget(tab, TEXT_VIEW);
*/
	g_signal_connect(G_OBJECT(text_buffer), "insert-text",
							G_CALLBACK(autocomplete_on_text_buffer_insert_text), (gpointer) text_view);

	// Can we get a list of already connected signal handlers?
	return;

	// We would like to handle application key-press events and do it before the applications main handler gets them..
/*
	g_signal_connect(G_OBJECT(_app_window), "key-press-event",
							G_CALLBACK(on_popup_window_key_press_event), NULL);
*/
}





