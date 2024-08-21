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

static bool enclose_selected_text;

char *deleted_text;

/* The only reason we register this callback is to implement quote-selected-text-feature.
Problem is that if there is a selection, GTK deletes the selected text before it calls our "insert-text"-handler and we have no way to get that text back.
Perhaps it would be easier to do autocompleting when handling key-combinations. */
void text_expansion_text_buffer_begin_user_action(GtkTextBuffer *text_buffer, gpointer data)
{
	LOG_MSG("%s()\n", __FUNCTION__);
	if(deleted_text){
		free(deleted_text);
		deleted_text = NULL;
	}
}

void text_expansion_text_buffer_delete_range(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end, gpointer data)
{
	LOG_MSG("%s()\n", __FUNCTION__);

	deleted_text = gtk_text_buffer_get_text(text_buffer, start, end, FALSE);

}

void text_expansion_text_buffer_insert_text(GtkTextBuffer *text_buffer, GtkTextIter *location, char *inserted_text, int length, gpointer data)
{
	LOG_MSG("%s()\n", __FUNCTION__);

	if (length == 1) {
		char ch = inserted_text[0];

		switch (ch) {
			case '\"':
				if(deleted_text && enclose_selected_text){
					int to_insert_size = strlen(deleted_text) + 2 + 1;
					char *to_insert = (char *) alloca(to_insert_size);
					sprintf(to_insert, "\"%s\"", deleted_text);
					int o1 = gtk_text_iter_get_offset(location);
					int o2 = o1 + to_insert_size - 1;
					gtk_text_buffer_insert(text_buffer, location, to_insert, -1);
					//printf("...offsets: %d -> %d\n", o1, o2);
					GtkTextIter i1, i2;
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i1, o1);
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i2, o2);
					gtk_text_buffer_select_range(text_buffer, &i1, &i2);
				}else{
					gtk_text_buffer_insert(text_buffer, location, "\"\"", -1);
					gtk_text_iter_backward_char(location);
					gtk_text_buffer_place_cursor(text_buffer, location);
				}
	
				g_signal_stop_emission_by_name(text_buffer, "insert-text");
				break;
			case '\'':
				if(deleted_text && enclose_selected_text){
					int to_insert_size = strlen(deleted_text) + 2 + 1;
					char *to_insert = (char *) alloca(to_insert_size);
					sprintf(to_insert, "\'%s\'", deleted_text);
					int o1 = gtk_text_iter_get_offset(location);
					int o2 = o1 + to_insert_size - 1;
					gtk_text_buffer_insert(text_buffer, location, to_insert, -1);
					//printf("...offsets: %d -> %d\n", o1, o2);
					GtkTextIter i1, i2;
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i1, o1);
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i2, o2);
					gtk_text_buffer_select_range(text_buffer, &i1, &i2);
				}else{
					gtk_text_buffer_insert(text_buffer, location, "''", -1);
					gtk_text_iter_backward_char(location);
					gtk_text_buffer_place_cursor(text_buffer, location);
				}
	
				g_signal_stop_emission_by_name(text_buffer, "insert-text");
				break;
			case '(':
				if(deleted_text && enclose_selected_text){
					int to_insert_size = strlen(deleted_text) + 2 + 1;
					char *to_insert = (char *) alloca(to_insert_size);
					sprintf(to_insert, "(%s)", deleted_text);
					int o1 = gtk_text_iter_get_offset(location);
					int o2 = o1 + to_insert_size - 1;
					gtk_text_buffer_insert(text_buffer, location, to_insert, -1);
					//printf("...offsets: %d -> %d\n", o1, o2);
					GtkTextIter i1, i2;
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i1, o1);
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i2, o2);
					gtk_text_buffer_select_range(text_buffer, &i1, &i2);
				}else{
					gtk_text_buffer_insert(text_buffer, location, "()", -1);
					gtk_text_iter_backward_char(location);
					gtk_text_buffer_place_cursor(text_buffer, location);
				}
	
				g_signal_stop_emission_by_name(text_buffer, "insert-text");
				break;
			case '{':
				if(deleted_text && enclose_selected_text){
					int to_insert_size = strlen(deleted_text) + 2 + 1;
					char *to_insert = (char *) alloca(to_insert_size);
					sprintf(to_insert, "{%s}", deleted_text);
					int o1 = gtk_text_iter_get_offset(location);
					int o2 = o1 + to_insert_size - 1;
					gtk_text_buffer_insert(text_buffer, location, to_insert, -1);
					//printf("...offsets: %d -> %d\n", o1, o2);
					GtkTextIter i1, i2;
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i1, o1);
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i2, o2);
					gtk_text_buffer_select_range(text_buffer, &i1, &i2);
				}else{
					gtk_text_buffer_insert(text_buffer, location, "{}", -1);
					gtk_text_iter_backward_char(location);
					gtk_text_buffer_place_cursor(text_buffer, location);
				}
	
				g_signal_stop_emission_by_name(text_buffer, "insert-text");
				break;
			case '[':
				if(deleted_text && enclose_selected_text){
					int to_insert_size = strlen(deleted_text) + 2 + 1;
					char *to_insert = (char *) alloca(to_insert_size);
					sprintf(to_insert, "[%s]", deleted_text);
					int o1 = gtk_text_iter_get_offset(location);
					int o2 = o1 + to_insert_size - 1;
					gtk_text_buffer_insert(text_buffer, location, to_insert, -1);
					//printf("...offsets: %d -> %d\n", o1, o2);
					GtkTextIter i1, i2;
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i1, o1);
					gtk_text_buffer_get_iter_at_offset(text_buffer, &i2, o2);
					gtk_text_buffer_select_range(text_buffer, &i1, &i2);
				}else{
					gtk_text_buffer_insert(text_buffer, location, "[]", -1);
					gtk_text_iter_backward_char(location);
					gtk_text_buffer_place_cursor(text_buffer, location);
				}
	
				g_signal_stop_emission_by_name(text_buffer, "insert-text");
				break;
		}
	}

	// When text is pasted into the text buffer, "begin-user-action" is not signaled (probably because we overwrote paste-handler?), which means that "deleted_text" is not cleared. This results in buggy behaviour.
	if(deleted_text){
		free(deleted_text);
		deleted_text = NULL;
	}
}

//@ If "enclose_selected_text" is false, we dont need to handle "begin-user-action" and "delete-range" signals at all, which makes me think about factoring the whole thing differently?
void text_expansion_init(GtkTextBuffer *text_buffer, Node *settings, GtkWidget *tab)
{
	LOG_MSG("init_autocomplete_character()\n");

	enclose_selected_text = true;
	const char *value = settings_get_value(settings, "autocomplete-character/enclose-selected-text");
	if(value)
	{
			enclose_selected_text = (strcmp(value, "true") == 0) ? true : false;
	}
	else
	{
//		display_error("Setting \"autocomplete-character/enclose-selected-text\" doesnt seem to be set in the settings file.", "Reverting to default value then: true");
			ERROR("Setting \"autocomplete-character/enclose-selected-text\" doesnt seem to be set in the settings file. (Reverting to default value then: true)")
	}
}
