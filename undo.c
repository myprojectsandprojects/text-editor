#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tab.h"

GtkTextView *tab_get_text_view(GtkWidget *tab);

#define DELETE_ACTION 0
#define INSERT_ACTION 1

struct UserAction {
	int type;
	gboolean can_buffer;
	
	// DELETE_ACTION:
	int start_offset;
	int end_offset;
	char deleted_text_buffer[10000]; //@ crashes if too much text

	// INSERT_ACTION:
	int location_offset;
	int last_location_offset;
	int text_length;
};

struct UserAction *last_actions[100]; // As a global variable it should be initialized to NULLs by default (?)

gboolean ignore = FALSE;
/*
gboolean uninitialized = TRUE;
void init_last_actions_array() {
	g_print("init_last_actions_array()\n");
	int i;
	for(i = 0; i < 100; ++i) last_actions[i] = NULL;
}
*/

void on_text_buffer_delete_range(
	GtkTextBuffer *text_buffer,
	GtkTextIter *start,
	GtkTextIter *end,
	gpointer data)
{
	if(ignore == TRUE) {
		ignore = FALSE;
		return;
	}

	gint start_offset, end_offset;
	start_offset = gtk_text_iter_get_offset(start);
	end_offset = gtk_text_iter_get_offset(end);
	char *deleted_text = gtk_text_buffer_get_text(text_buffer, start, end, FALSE);
	//g_print("delete-range: start offset: %d, end offset: %d\n", start_offset, end_offset);
	//g_print("delete-range: text deleted: %s\n", deleted_text);

	GtkWidget *tab = (GtkWidget *) data;
	struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	unsigned index = tab_info->id;

	assert(index < 100);

	if(last_actions[index] != NULL) {
		if(last_actions[index]->type == DELETE_ACTION
		&& last_actions[index]->can_buffer == TRUE
		&& end_offset - start_offset == 1 // 1 character
		&& start_offset == last_actions[index]->start_offset - 1) {
			// We are buffering...
			last_actions[index]->start_offset = start_offset;
			char buffer[100];
			strcpy(buffer, last_actions[index]->deleted_text_buffer);
			sprintf(last_actions[index]->deleted_text_buffer, "%s%s", deleted_text, buffer);
		} else if(last_actions[index]->type == DELETE_ACTION
		&& last_actions[index]->can_buffer == TRUE
		&& end_offset - start_offset == 1 // 1 character
		&& start_offset == last_actions[index]->start_offset) {
			// We are buffering...
			char buffer[100];
			strcpy(buffer, last_actions[index]->deleted_text_buffer);
			sprintf(last_actions[index]->deleted_text_buffer, "%s%s", buffer, deleted_text);
		} else {
			free(last_actions[index]);

			// Create new action:
			last_actions[index] = malloc(sizeof(struct UserAction));
			last_actions[index]->type = DELETE_ACTION;
			last_actions[index]->start_offset = start_offset;
			last_actions[index]->end_offset = end_offset;
			sprintf(last_actions[index]->deleted_text_buffer, "%s", deleted_text);
			last_actions[index]->can_buffer = (end_offset - start_offset == 1) ? TRUE : FALSE;
		}
	} else {
		// Create new action:
		last_actions[index] = malloc(sizeof(struct UserAction));
		last_actions[index]->type = DELETE_ACTION;
		last_actions[index]->start_offset = start_offset;
		last_actions[index]->end_offset = end_offset;
		sprintf(last_actions[index]->deleted_text_buffer, "%s", deleted_text);
		last_actions[index]->can_buffer = (end_offset - start_offset == 1) ? TRUE : FALSE;
	}
}

void on_text_buffer_insert_text(
	GtkTextBuffer *text_buffer,
	GtkTextIter *location,
	char *inserted_text,
	int length,
	gpointer data)
{
	if(ignore == TRUE) {
		ignore = FALSE;
		return;
	}
	//g_print("[%s: %d] text-buffer: insert-text signal!\n", __FILE__, __LINE__);

	gint location_offset;
	location_offset = gtk_text_iter_get_offset(location);
	//g_print("insert-text: location offset: %d\n", location_offset);
	//g_print("insert-text: length: %d\n", length);

	GtkWidget *tab = (GtkWidget *) data;
	struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	unsigned index = tab_info->id;

	assert(index < 100);

	if(last_actions[index] == NULL) {
		// Create new action:
		last_actions[index] = malloc(sizeof(struct UserAction));
		last_actions[index]->type = INSERT_ACTION;
		last_actions[index]->location_offset = location_offset;
		last_actions[index]->last_location_offset = location_offset;
		last_actions[index]->text_length = length;
		last_actions[index]->can_buffer = (length == 1) ? TRUE : FALSE;
	} else if(last_actions[index]->type == INSERT_ACTION
			&& last_actions[index]->can_buffer == TRUE
			&& location_offset == last_actions[index]->last_location_offset + 1
			&& length == 1) {
		// We are buffering...
		last_actions[index]->last_location_offset = location_offset;
		last_actions[index]->text_length += 1;

	} else {
		free(last_actions[index]);
		// Create new action:
		last_actions[index] = malloc(sizeof(struct UserAction));
		last_actions[index]->type = INSERT_ACTION;
		last_actions[index]->location_offset = location_offset;
		last_actions[index]->last_location_offset = location_offset;
		last_actions[index]->text_length = length;
		last_actions[index]->can_buffer = (length == 1) ? TRUE : FALSE;
	}
}

void init_undo(GtkWidget *tab)
{
	printf("init_undo() called\n");

	GtkTextView *text_view = tab_get_text_view(tab);
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);

	g_signal_connect(G_OBJECT(text_buffer), "insert-text", G_CALLBACK(on_text_buffer_insert_text), tab);
	g_signal_connect(G_OBJECT(text_buffer), "delete-range", G_CALLBACK(on_text_buffer_delete_range), tab);
}

void actually_undo_last_action(GtkWidget *tab)
{
	GtkTextView *text_view = tab_get_text_view(tab);
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);

	struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	//printf("actually_undo_last_action() for tab number %d called.\n", tab_info->id);
	unsigned index = tab_info->id;
	assert(index < 100);

	if(last_actions[index] == NULL) return;

	if(last_actions[index]->type == INSERT_ACTION) {
		GtkTextIter start, end;
		gtk_text_buffer_get_iter_at_offset(text_buffer, &start, last_actions[index]->location_offset);
		gtk_text_buffer_get_iter_at_offset(text_buffer, &end, last_actions[index]->location_offset + last_actions[index]->text_length);
		ignore = TRUE; 
		gtk_text_buffer_delete(text_buffer, &start, &end);
	} else { // DELETE_ACTION
		GtkTextIter location;
		gtk_text_buffer_get_iter_at_offset(text_buffer, &location, last_actions[index]->start_offset);
		ignore = TRUE;
		gtk_text_buffer_insert(text_buffer, &location, last_actions[index]->deleted_text_buffer, -1);
	}
	
	free(last_actions[index]); last_actions[index] = NULL;
}



















