#include <gtk/gtk.h>
#include <stdlib.h>
#include <assert.h>

#include "tab.h"

extern GtkWidget *notebook;

//@ Shouldnt call add_highlighting() every single time!
void indent_selected_block(GtkTextBuffer *text_buffer, GtkTextIter *selection_start, GtkTextIter *selection_end)
{
	g_print("indent_selected_block: called!\n");

	int last_line = gtk_text_iter_get_line(selection_end);

	gtk_text_iter_set_line_offset(selection_start, 0);

	while(gtk_text_iter_get_line(selection_start) <= last_line) {
		gtk_text_buffer_insert(text_buffer, selection_start, "\t", -1);
		if(gtk_text_iter_forward_line(selection_start) == FALSE) return;
	}
}

void unindent_selected_block(GtkTextBuffer *text_buffer, GtkTextIter *selection_start, GtkTextIter *selection_end)
{
	g_print("unindent_selected_block: called!\n");

	GtkTextIter match_start, match_end;

	gtk_text_iter_set_line_offset(selection_start, 0);
	GtkTextMark *start = gtk_text_buffer_create_mark(text_buffer, NULL, selection_start, TRUE);
	GtkTextMark *end = gtk_text_buffer_create_mark(text_buffer, NULL, selection_end, TRUE);
	if(gtk_text_iter_forward_search(selection_start, "\t", 0, &match_start, &match_end, NULL) == TRUE) {
		if(gtk_text_iter_compare(&match_start, selection_end) < 0)
			gtk_text_buffer_delete(text_buffer, &match_start, &match_end);
	}
	gtk_text_buffer_get_iter_at_mark(text_buffer, selection_start, start);
	gtk_text_buffer_get_iter_at_mark(text_buffer, selection_end, end);

	gtk_text_iter_forward_line(selection_start);

	while(gtk_text_iter_compare(selection_start, selection_end) < 0) {
		gtk_text_buffer_move_mark(text_buffer, start, selection_start);
		if(gtk_text_iter_forward_search(selection_start, "\t", 0, &match_start, &match_end, NULL) == TRUE) {
			if(gtk_text_iter_compare(&match_start, selection_end) < 0)
				gtk_text_buffer_delete(text_buffer, &match_start, &match_end);
		}
		gtk_text_buffer_get_iter_at_mark(text_buffer, selection_start, start);
		gtk_text_buffer_get_iter_at_mark(text_buffer, selection_end, end);
		gtk_text_iter_forward_line(selection_start);
	}

	// Should delete marks???
}

/*
	returns TRUE if handled the tab, otherwise FALSE
*/
gboolean handle_tab_key(GtkTextBuffer *text_buffer, GdkEventKey *key_event)
{
	g_print("handle_tab_key: called!\n");

/* In Gedit tab + shift only unindents lines regardless of where in the line the cursor is... 
tab inserts tabs or replaces selected text with a tab as long as the selection doesnt span across multiple lines in which case it indents the selected lines. */

	assert(GTK_IS_TEXT_BUFFER(text_buffer));

	GtkTextIter selection_start, selection_end;
	gtk_text_buffer_get_selection_bounds(text_buffer, &selection_start, &selection_end);


	if(key_event->state & GDK_SHIFT_MASK) {
		g_print("handle_tab_key: tab + shift\n");

		/*if(gtk_text_iter_compare(&selection_start, &selection_end) == 0) {
			g_print("handle_tab_key: no selection\n");
			return;
		}*/

		if(gtk_text_iter_get_line(&selection_start) == gtk_text_iter_get_line(&selection_end)) {
			g_print("handle_tab_key: selection: single line\n");
			return FALSE;
		}

		g_print("handle_tab_key: selection offsets: %d, %d\n",
				gtk_text_iter_get_offset(&selection_start),
				gtk_text_iter_get_offset(&selection_end));

		unindent_selected_block(text_buffer, &selection_start, &selection_end);
	} else {
		g_print("handle_tab_key: tab (without shift)\n");
		/* Maybe we would like to rely on the default functionality (insert a tab or if selection then replace selected text with a tab) unless the selection spans over multiple lines... */
		/*if(gtk_text_iter_compare(&selection_start, &selection_end) == 0) {
			g_print("handle_tab_key: no selection\n");
			return;
		}*/

		if(gtk_text_iter_get_line(&selection_start) == gtk_text_iter_get_line(&selection_end)) {
			//g_print("handle_tab_key: selection: single line\n");
			return FALSE;
		}

		g_print("handle_tab_key: selection offsets: %d, %d\n",
				gtk_text_iter_get_offset(&selection_start),
				gtk_text_iter_get_offset(&selection_end));

		indent_selected_block(text_buffer, &selection_start, &selection_end);
	}
	
	return TRUE;
}

void actually_open_line_before(GtkTextBuffer *text_buffer)
{
	g_print("open_line_before() called!");

	GtkTextIter iter_start, iter;
	gtk_text_buffer_get_start_iter(text_buffer, &iter_start);

	GtkTextMark *cursor = gtk_text_buffer_get_mark(text_buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, cursor);
	gtk_text_iter_set_line_offset(&iter, 0); // to the beginning of the line 
	gtk_text_buffer_insert(text_buffer, &iter, "\n", -1);
	gtk_text_iter_backward_char(&iter);
	gtk_text_buffer_place_cursor(text_buffer, &iter);
	//gtk_text_buffer_insert_at_cursor (global_text_buffer, "\n", -1);
	//gtk_text_view_scroll_to_iter(global_text_view, &iter_start, 0.0, FALSE, 0.0, 0.0);
}

void actually_open_line_after(GtkTextBuffer *text_buffer)
{
	GtkTextIter iter;

	GtkTextMark *cursor = gtk_text_buffer_get_mark(text_buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, cursor);

	gtk_text_iter_forward_line(&iter); // to the beginning of the next line or to the end of the current line (if already on the last line)
	if (gtk_text_iter_is_end(&iter) == FALSE) {
		gtk_text_buffer_insert(text_buffer, &iter, "\n", -1);
		gtk_text_iter_backward_char(&iter);
	} else {
		gtk_text_buffer_insert(text_buffer, &iter, "\n", -1);
	}
	gtk_text_buffer_place_cursor(text_buffer, &iter);
}

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

gboolean move_lines_up(GdkEventKey *key_event)
{
	GtkTextBuffer *text_buffer;
	text_buffer = GTK_TEXT_BUFFER(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER));
	if (text_buffer == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	// Delete the line before selected lines.
	// Insert the line after selected lines

	GtkTextIter selection_start_iter, selection_end_iter, iter;
	gtk_text_buffer_get_selection_bounds(text_buffer, &selection_start_iter, &selection_end_iter);

	iter = selection_start_iter;
	gboolean result = gtk_text_iter_backward_lines(&iter, 1); // Go to the beginning of the previous line
	if (result == FALSE) {
		printf("No lines before..\n");
		return TRUE;
	}
	//printf("iter offset: %d\n", gtk_text_iter_get_offset(&iter));
	GtkTextIter start_prev_line, end_prev_line;
	start_prev_line = iter;
	gtk_text_iter_forward_line(&iter);
	end_prev_line = iter;
	char *text = gtk_text_buffer_get_text(text_buffer, &start_prev_line, &end_prev_line, FALSE);
	printf("previous line: %s\n", text);

	iter = selection_end_iter;
	gtk_text_iter_forward_line(&iter);
	GtkTextMark *keep_this = gtk_text_buffer_create_mark(text_buffer, NULL, &iter, TRUE);
	gtk_text_buffer_delete(text_buffer, &start_prev_line, &end_prev_line);
	gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, keep_this);
	gtk_text_buffer_insert(text_buffer, &iter, text, -1);
	gtk_text_buffer_delete_mark(text_buffer, keep_this);
}

gboolean move_lines_down(GdkEventKey *key_event)
{
	GtkTextBuffer *text_buffer;
	text_buffer = GTK_TEXT_BUFFER(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER));
	if (text_buffer == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	printf("Should move lines down...\n");

	// Delete the line after selected lines.
	// Insert the line before selected lines

	GtkTextIter selection_start_iter, selection_end_iter, iter;
	gtk_text_buffer_get_selection_bounds(text_buffer, &selection_start_iter, &selection_end_iter);

	iter = selection_end_iter;
	gboolean result = gtk_text_iter_forward_line(&iter);
	if (result == FALSE) {
		printf("No lines after..\n");
		return TRUE;
	}
	//printf("iter offset: %d\n", gtk_text_iter_get_offset(&iter));
	GtkTextIter start_next_line, end_next_line;
	start_next_line = iter;
	gtk_text_iter_forward_line(&iter);
	end_next_line = iter;
	char *text = gtk_text_buffer_get_text(text_buffer, &start_next_line, &end_next_line, FALSE);
	//printf("next line: %s\n", text);

	iter = selection_start_iter;
	gtk_text_iter_set_line_offset(&iter, 0);
	GtkTextMark *keep_this = gtk_text_buffer_create_mark(text_buffer, NULL, &iter, TRUE);
	gtk_text_buffer_delete(text_buffer, &start_next_line, &end_next_line);
	gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, keep_this);
	gtk_text_buffer_insert(text_buffer, &iter, text, -1);
	gtk_text_buffer_delete_mark(text_buffer, keep_this);
}

gboolean duplicate_line(GdkEventKey *key_event)
{
	GtkTextBuffer *text_buffer;
	text_buffer = GTK_TEXT_BUFFER(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER));
	if (text_buffer == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	//printf("Should duplicate line...\n");

	GtkTextIter start, end;

	GtkTextMark *cursor = gtk_text_buffer_get_mark(text_buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &start, cursor);
	end = start,
	gtk_text_iter_set_line_offset(&start, 0);
	gtk_text_iter_forward_line(&end);
	GtkTextMark *start_mark = gtk_text_buffer_create_mark(text_buffer, NULL, &start, FALSE);
	GtkTextMark *end_mark = gtk_text_buffer_create_mark(text_buffer, NULL, &end, FALSE);
	if (gtk_text_iter_is_end(&end)) {
		gtk_text_iter_backward_char(&end);
		gunichar c = gtk_text_iter_get_char(&end);
		gtk_text_iter_forward_char(&end);
		if (c != '\n') {
			gtk_text_buffer_insert(text_buffer, &end, "\n", -1);
			gtk_text_buffer_get_iter_at_mark(text_buffer, &start, start_mark);
			gtk_text_buffer_get_iter_at_mark(text_buffer, &end, end_mark);
		}
	}
	char *text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
	printf("inserting: %s\n", text);
	gtk_text_buffer_insert(text_buffer, &end, text, -1);
	/*gtk_text_iter_backward_char(&iter); //@ at the end of the buffer -- bug?
	gtk_text_buffer_place_cursor(text_buffer, &iter);*/
	gtk_text_buffer_delete_mark(text_buffer, start_mark);
	gtk_text_buffer_delete_mark(text_buffer, end_mark);
}

gboolean delete_line(GdkEventKey *key_event)
{
	printf("delete_line()\n");

	GtkTextBuffer *text_buffer;
	text_buffer = GTK_TEXT_BUFFER(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER));
	if (text_buffer == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	GtkTextIter start, end;

	GtkTextMark *cursor = gtk_text_buffer_get_mark(text_buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &start, cursor);
	end = start,
	gtk_text_iter_set_line_offset(&start, 0);
	gtk_text_iter_forward_line(&end);
	gtk_text_buffer_delete(text_buffer, &start, &end);

	return TRUE;
}

gboolean move_up_by_block(GdkEventKey *key_event)
{
	printf("move_up_by_block()\n");

	GtkTextBuffer *text_buffer;
	text_buffer = GTK_TEXT_BUFFER(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER));
	if (text_buffer == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	GtkTextIter iter, start, end;

	GtkTextMark *cursor = gtk_text_buffer_get_mark(text_buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, cursor);

	while (gtk_text_iter_get_char(&iter) == '\n' && !gtk_text_iter_is_start(&iter))
		gtk_text_iter_backward_char(&iter);

	if (gtk_text_iter_backward_search(&iter, "\n\n", 0, &start, &end, NULL)) {
		iter = end;
		gtk_text_iter_backward_char(&iter);
	} else {
		gtk_text_buffer_get_start_iter(text_buffer, &iter);
	}

	gtk_text_buffer_place_cursor(text_buffer, &iter);

	return TRUE;
}

gboolean move_down_by_block(GdkEventKey *key_event)
{
	printf("move_down_by_block()\n");

	GtkTextBuffer *text_buffer;
	text_buffer = GTK_TEXT_BUFFER(visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER));
	if (text_buffer == NULL) {
		printf("No tabs open! Nothing to do...\n");
		return FALSE;
	}

	GtkTextIter iter, start, end;

	GtkTextMark *cursor = gtk_text_buffer_get_mark(text_buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, cursor);

	while (gtk_text_iter_get_char(&iter) == '\n' && !gtk_text_iter_is_end(&iter))
		gtk_text_iter_forward_char(&iter);

	if (gtk_text_iter_forward_search(&iter, "\n\n", 0, &start, &end, NULL)) {
		iter = end;
		gtk_text_iter_backward_char(&iter);
	} else {
		gtk_text_buffer_get_end_iter(text_buffer, &iter);
	}

	gtk_text_buffer_place_cursor(text_buffer, &iter);

	return TRUE;
}
