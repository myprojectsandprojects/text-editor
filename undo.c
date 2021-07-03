#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tab.h"

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

#define CAPACITY 3
#define IS_EMPTY 0
#define IS_FULL 1
#define IS_NEITHER 2

struct ActionsStack {
	struct UserAction *actions[CAPACITY];
	int top;
	int bottom;
	int state;
};

struct ActionsStack *create_actions_stack()
{
	struct ActionsStack *actions = malloc(sizeof(struct ActionsStack));
	actions->top = 0;
	actions->bottom = 0;
	actions->state = IS_EMPTY;
	return actions;
}

void add_action(struct ActionsStack *actions, struct UserAction *action) {
	if(actions->state == IS_EMPTY) {
		actions->state = IS_NEITHER;
	}
	if(actions->state == IS_FULL) {
		free(actions->actions[actions->bottom]);
		if(actions->bottom == CAPACITY - 1) {
			actions->bottom = 0;
		}else{
			actions->bottom++;
		}
	}
	actions->actions[actions->top] = action;
	if(actions->top == CAPACITY - 1) {
		actions->top = 0;
	}else{
		actions->top++;
	}
	if(actions->top == actions->bottom) {
		actions->state = IS_FULL;
	}
}

struct UserAction *get_action(struct ActionsStack *actions) {
	if(actions->state == IS_FULL) {
		actions->state = IS_NEITHER;
	}
	if(actions->state == IS_EMPTY) {
		return NULL;
	}
	if(actions->top == 0) {
		actions->top = CAPACITY - 1;
	}else{
		actions->top--;
	}
	if(actions->top == actions->bottom) {
		actions->state = IS_EMPTY;
	}
	return actions->actions[actions->top];
}

// This is a global variable, so it should be initialized to defaults (NULLs in this case) (?)
struct ActionsStack *tab_actions[100];

gboolean ignore = FALSE;

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

	if (tab_actions[index] == NULL) {
		tab_actions[index] = create_actions_stack();
	}

	struct UserAction *action = malloc(sizeof(struct UserAction));
	action->type = DELETE_ACTION;
	action->start_offset = start_offset;
	action->end_offset = end_offset;
	sprintf(action->deleted_text_buffer, "%s", deleted_text);
	add_action(tab_actions[index], action);

	/*if(last_actions[index] != NULL) {
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
	}*/
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

	if (tab_actions[index] == NULL) {
		tab_actions[index] = create_actions_stack();
	}

	struct UserAction *action = malloc(sizeof(struct UserAction));
	action->type = INSERT_ACTION;
	action->location_offset = location_offset;
	action->text_length = length;
	add_action(tab_actions[index], action);

	/*if(last_actions[index] == NULL) {
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
	}*/
}

void init_undo(GtkWidget *tab)
{
	printf("init_undo() called\n");

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));

	/* @ Could just pass in tab-id directly? Performance? */
	g_signal_connect(G_OBJECT(text_buffer), "insert-text", G_CALLBACK(on_text_buffer_insert_text), tab);
	g_signal_connect(G_OBJECT(text_buffer), "delete-range", G_CALLBACK(on_text_buffer_delete_range), tab);
}

void actually_undo_last_action(GtkWidget *tab)
{
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));

	struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	//printf("actually_undo_last_action() for tab number %d called.\n", tab_info->id);
	unsigned index = tab_info->id;
	assert(index < 100);

	struct UserAction *action = get_action(tab_actions[index]);
	if(action == NULL) {
		printf("No actions to undo for tab %d!\n", index);
		return;
	}

	if(action->type == INSERT_ACTION) {
		GtkTextIter start, end;
		gtk_text_buffer_get_iter_at_offset(text_buffer, &start, action->location_offset);
		gtk_text_buffer_get_iter_at_offset(text_buffer, &end, action->location_offset + action->text_length);
		ignore = TRUE; 
		gtk_text_buffer_delete(text_buffer, &start, &end);
	} else { // DELETE_ACTION
		GtkTextIter location;
		gtk_text_buffer_get_iter_at_offset(text_buffer, &location, action->start_offset);
		ignore = TRUE;
		gtk_text_buffer_insert(text_buffer, &location, action->deleted_text_buffer, -1);
	}

	free(action);
}



















