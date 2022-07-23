#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "declarations.h"


/*
gboolean autocomplete_character(GdkEventKey *key_event)
{
	GtkWidget *text_view = GTK_WIDGET(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW));
	if (GTK_IS_TEXT_VIEW(text_view) == FALSE || gtk_widget_is_focus(text_view) == FALSE) {
		printf("autocomplete_character(): early-out...\n");
		return FALSE;
	}

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER));
	actually_autocomplete_character(text_buffer, (char) key_event->keyval);

	return TRUE;
}
*/

/*
char autocomplete_map[128];

void actually_autocomplete_character(GtkTextBuffer *text_buffer, char character)
{
	//@ this is nonsense:
	autocomplete_map['\"'] = '\"';
	autocomplete_map['\''] = '\'';
	autocomplete_map['('] = ')';
	autocomplete_map['{'] = '}';
	autocomplete_map['['] = ']';

	GtkTextIter iter;

	GtkTextMark *cursor = gtk_text_buffer_get_mark(text_buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, cursor);

	char *buffer = malloc(3);
	sprintf(buffer, "%c%c", character, autocomplete_map[character]);
	//sprintf(buffer, "%c%c", character, character);
	gtk_text_buffer_insert(text_buffer, &iter, buffer, -1);

	gtk_text_iter_backward_char(&iter);
	gtk_text_buffer_place_cursor(text_buffer, &iter);

	free(buffer);
}
*/

static bool replace_selected_text;

char *deleted_text;

/* The only reason we register this callback is to implement quote-selected-text-feature.
Problem is that if there is a selection, GTK deletes the selected text before it calls our "insert-text"-handler and we have no way to get that text back.
Perhaps it would be easier to do autocompleting when handling key-combinations. */
void on_text_buffer_begin_user_action_4_autocomplete_character(
	GtkTextBuffer *text_buffer, gpointer data)
{
	printf("on_text_buffer_begin_user_action_4_autocomplete_character()\n");
	LOG_MSG("on_text_buffer_begin_user_action_4_autocomplete_character()\n");
	if(deleted_text){
		free(deleted_text);
		deleted_text = NULL;
	}
}


void on_text_buffer_delete_range_4_autocomplete_character(
	GtkTextBuffer *text_buffer,
	GtkTextIter *start,
	GtkTextIter *end,
	gpointer data)
{
	LOG_MSG("on_text_buffer_delete_range_4_autocomplete_character()\n");
	printf("on_text_buffer_delete_range_4_autocomplete_character()\n");

	deleted_text = gtk_text_buffer_get_text(text_buffer, start, end, FALSE);

}

void on_text_buffer_insert_text_4_autocomplete_character(GtkTextBuffer *text_buffer,
	GtkTextIter *location, char *inserted_text, int length, gpointer data)
{
	LOG_MSG("on_text_buffer_insert_text_4_autocomplete_character()\n");
	printf("on_text_buffer_insert_text_4_autocomplete_character()\n");

	if (length == 1) {
		char ch = inserted_text[0];

		switch (ch) {
			
			case '\"':
				if(replace_selected_text){
					gtk_text_buffer_insert(text_buffer, location, "\"\"", -1);
					gtk_text_iter_backward_char(location);
					gtk_text_buffer_place_cursor(text_buffer, location);
				}else{
					char to_insert[100];
					snprintf(to_insert, 100, "\"%s\"", deleted_text);
					int o1 = gtk_text_iter_get_offset(location);
					int o2 = o1 + strlen(to_insert);
					gtk_text_buffer_insert(text_buffer, location, to_insert, -1);
					//printf("...offsets: %d -> %d\n", o1, o2);
					GtkTextIter i1, i2;
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i1, o1);
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i2, o2);
					gtk_text_buffer_select_range(text_buffer, &i1, &i2);
				}
	
				g_signal_stop_emission_by_name(text_buffer, "insert-text");
				break;
			case '\'':
				if(replace_selected_text){
					gtk_text_buffer_insert(text_buffer, location, "''", -1);
					gtk_text_iter_backward_char(location);
					gtk_text_buffer_place_cursor(text_buffer, location);
				}else{
					char to_insert[100];
					snprintf(to_insert, 100, "'%s'", deleted_text);
					int o1 = gtk_text_iter_get_offset(location);
					int o2 = o1 + strlen(to_insert);
					gtk_text_buffer_insert(text_buffer, location, to_insert, -1);
					//printf("...offsets: %d -> %d\n", o1, o2);
					GtkTextIter i1, i2;
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i1, o1);
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i2, o2);
					gtk_text_buffer_select_range(text_buffer, &i1, &i2);
				}
	
				g_signal_stop_emission_by_name(text_buffer, "insert-text");
				break;
			case '(':
				if(replace_selected_text){
					gtk_text_buffer_insert(text_buffer, location, "()", -1);
					gtk_text_iter_backward_char(location);
					gtk_text_buffer_place_cursor(text_buffer, location);
				}else{
					char to_insert[100];
					snprintf(to_insert, 100, "(%s)", deleted_text);
					int o1 = gtk_text_iter_get_offset(location);
					int o2 = o1 + strlen(to_insert);
					gtk_text_buffer_insert(text_buffer, location, to_insert, -1);
					//printf("...offsets: %d -> %d\n", o1, o2);
					GtkTextIter i1, i2;
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i1, o1);
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i2, o2);
					gtk_text_buffer_select_range(text_buffer, &i1, &i2);
				}
	
				g_signal_stop_emission_by_name(text_buffer, "insert-text");
				break;
			case '{':
				if(replace_selected_text){
					gtk_text_buffer_insert(text_buffer, location, "{}", -1);
					gtk_text_iter_backward_char(location);
					gtk_text_buffer_place_cursor(text_buffer, location);
				}else{
					char to_insert[100];
					snprintf(to_insert, 100, "{%s}", deleted_text);
					int o1 = gtk_text_iter_get_offset(location);
					int o2 = o1 + strlen(to_insert);
					gtk_text_buffer_insert(text_buffer, location, to_insert, -1);
					//printf("...offsets: %d -> %d\n", o1, o2);
					GtkTextIter i1, i2;
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i1, o1);
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i2, o2);
					gtk_text_buffer_select_range(text_buffer, &i1, &i2);
				}
	
				g_signal_stop_emission_by_name(text_buffer, "insert-text");
				break;
			case '[':
				if(replace_selected_text){
					gtk_text_buffer_insert(text_buffer, location, "[]", -1);
					gtk_text_iter_backward_char(location);
					gtk_text_buffer_place_cursor(text_buffer, location);
				}else{
					char to_insert[100];
					snprintf(to_insert, 100, "[%s]", deleted_text);
					int o1 = gtk_text_iter_get_offset(location);
					int o2 = o1 + strlen(to_insert);
					gtk_text_buffer_insert(text_buffer, location, to_insert, -1);
					//printf("...offsets: %d -> %d\n", o1, o2);
					GtkTextIter i1, i2;
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i1, o1);
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i2, o2);
					gtk_text_buffer_select_range(text_buffer, &i1, &i2);
				}
	
				g_signal_stop_emission_by_name(text_buffer, "insert-text");
				break;
		}
	}

	// When text is pasted into the text buffer, "begin-user-action" is not signaled (probably because we overwrote this handler?), which means that "deleted_text" is not cleared. This results in buggy behaviour.
	if(deleted_text){
		free(deleted_text);
		deleted_text = NULL;
	}
}

void init_autocomplete_character(GtkTextBuffer *text_buffer, Node *settings)
{
	LOG_MSG("init_autocomplete_character()\n");

	replace_selected_text = false; // it probably already is "false"
	const char *value = settings_get_value(settings, "autocomplete-character/replace-selected-text");
	if(value)
		replace_selected_text = (strcmp(value, "true") == 0) ? true : false;
	else
		display_error("Setting \"autocomplete-character/replace-selected-text\" doesnt seem to be set in the settings file.", "Reverting to default value then: false");

	g_signal_connect(G_OBJECT(text_buffer),
		"insert-text", G_CALLBACK(on_text_buffer_insert_text_4_autocomplete_character), NULL);

	g_signal_connect(G_OBJECT(text_buffer),
		"delete-range", G_CALLBACK(on_text_buffer_delete_range_4_autocomplete_character), NULL);

	g_signal_connect(G_OBJECT(text_buffer),
		"begin-user-action", G_CALLBACK(on_text_buffer_begin_user_action_4_autocomplete_character), NULL);
}