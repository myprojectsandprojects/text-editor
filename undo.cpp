#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//#include "tab.h"
#include "declarations.h"

#define DELETE_ACTION 0
#define INSERT_ACTION 1

struct UserAction {
	int type;
	gboolean can_buffer;
	
	// DELETE_ACTION:
	int start_offset;
	int end_offset;
//	char deleted_text_buffer[10000]; //@ crashes if too much text
	char *deleted_text;

	// INSERT_ACTION:
	int location_offset;
	int last_location_offset;
	int text_length;
};

#define CAPACITY 100
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
	struct ActionsStack *actions = (struct ActionsStack *) malloc(sizeof(struct ActionsStack));
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

//gboolean ignore = FALSE;

const int MAX_TABS = 64;
const int MAX_HANDLERS = 1024;
struct Undo {
	int insert_handlers_count;
	int delete_handlers_count;
	gulong insert_handlers[MAX_HANDLERS];
	gulong delete_handlers[MAX_HANDLERS];
};
static Undo per_tab_undo_data[MAX_TABS]; // 64 tabs

void undo_text_buffer_delete_range(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end, gpointer data)
{
	LOG_MSG("%s()\n", __FUNCTION__);
	printf("%s()\n", __FUNCTION__);

//	if(ignore == TRUE) {
//		ignore = FALSE;
//		return;
//	}

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

	struct UserAction *action = (struct UserAction *) malloc(sizeof(struct UserAction));
	action->type = DELETE_ACTION;
	action->start_offset = start_offset;
	action->end_offset = end_offset;
//	sprintf(action->deleted_text_buffer, "%s", deleted_text);
	action->deleted_text = deleted_text;
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

void undo_text_buffer_insert_text(GtkTextBuffer *text_buffer, GtkTextIter *location, char *inserted_text, int length, gpointer data)
{
	LOG_MSG("%s()\n", __FUNCTION__);

//	if(ignore == TRUE) {
//		ignore = FALSE;
//		return;
//	}

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

	struct UserAction *action = (struct UserAction *) malloc(sizeof(struct UserAction));
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

void undo_init(gulong insert_handlers[], int insert_handlers_count, gulong *delete_handlers, int delete_handlers_count, unsigned int tab_id)
{
	LOG_MSG("%s()\n", __FUNCTION__);
//	printf("tab_id: %d\n", tab_id);

	unsigned int index = tab_id - 1;
	assert(index < MAX_TABS);
	for(int i = 0; i < insert_handlers_count; ++i) {
		per_tab_undo_data[index].insert_handlers[i] = insert_handlers[i];
	}
	for(int i = 0; i < delete_handlers_count; ++i) {
		per_tab_undo_data[index].delete_handlers[i] = delete_handlers[i];
	}
	per_tab_undo_data[index].insert_handlers_count = insert_handlers_count;
	per_tab_undo_data[index].delete_handlers_count = delete_handlers_count;
}

//@ still just a hack
//@ oh, I forgot that each tab has its own handlers
static void text_buffer_insert(GtkTextBuffer *text_buffer, int pos_offset, char *text, unsigned int tab_id)
{
	GtkTextIter pos;
	gtk_text_buffer_get_iter_at_offset(text_buffer, &pos, pos_offset);

	unsigned int index = tab_id - 1;

	for(int handler_index = 0; handler_index < per_tab_undo_data[index].insert_handlers_count; ++handler_index) {
		g_signal_handler_block(text_buffer, per_tab_undo_data[index].insert_handlers[handler_index]);
	}

	gtk_text_buffer_insert(text_buffer, &pos, text, -1);

	for(int handler_index = 0; handler_index < per_tab_undo_data[index].insert_handlers_count; ++handler_index) {
		g_signal_handler_unblock(text_buffer, per_tab_undo_data[index].insert_handlers[handler_index]);
	}
}
static void text_buffer_delete(GtkTextBuffer *text_buffer, int start_offset, int end_offset, unsigned int tab_id)
{
	GtkTextIter start, end;
	gtk_text_buffer_get_iter_at_offset(text_buffer, &start, start_offset);
	gtk_text_buffer_get_iter_at_offset(text_buffer, &end, end_offset);

	unsigned int index = tab_id - 1;

	for(int handler_index = 0; handler_index < per_tab_undo_data[index].delete_handlers_count; ++handler_index) {
		g_signal_handler_block(text_buffer, per_tab_undo_data[index].delete_handlers[handler_index]);
	}

	gtk_text_buffer_delete(text_buffer, &start, &end);

	for(int handler_index = 0; handler_index < per_tab_undo_data[index].delete_handlers_count; ++handler_index) {
		g_signal_handler_unblock(text_buffer, per_tab_undo_data[index].delete_handlers[handler_index]);
	}
}

void actually_undo_last_action(GtkWidget *tab)
{
	LOG_MSG("%s()\n", __FUNCTION__);

	struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	//printf("actually_undo_last_action() for tab number %d called.\n", tab_info->id);
	unsigned index = tab_info->id;
	assert(index < 100);

	struct UserAction *action = get_action(tab_actions[index]);
	if(action == NULL) {
		printf("No actions to undo for tab %d!\n", index);
		return;
	}

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));

	if(action->type == INSERT_ACTION) {
//		GtkTextIter start, end;
//		gtk_text_buffer_get_iter_at_offset(text_buffer, &start, action->location_offset);
//		gtk_text_buffer_get_iter_at_offset(text_buffer, &end, action->location_offset + action->text_length);
//		ignore = TRUE; 
//		gtk_text_buffer_delete(text_buffer, &start, &end);

		int From = action->location_offset;
		int To = action->location_offset + action->text_length;
		text_buffer_delete(text_buffer, From, To, tab_info->id);
	} else { // DELETE_ACTION
//		GtkTextIter location;
//		gtk_text_buffer_get_iter_at_offset(text_buffer, &location, action->start_offset);
//		ignore = TRUE;
//
//		//@ hack
//		// we block the autocomplete-character's "insert-text" handler to prevent it from autocompleting our undos's.
//		// autocomplete-character should only complete user-level insertions, but how do we differentiate between user-level and hmm program-level?
////		g_signal_emit_by_name(text_buffer, "begin-user-action");
//		gulong id = (gulong) tab_retrieve_widget(tab, AUTOCOMPLETE_CHARACTER_HANDLER_ID);
//		g_signal_handler_block(text_buffer, id);
////		gtk_text_buffer_insert(text_buffer, &location, action->deleted_text_buffer, -1);
//		gtk_text_buffer_insert(text_buffer, &location, action->deleted_text, -1);
//		g_signal_handler_unblock(text_buffer, id);

		text_buffer_insert(text_buffer, action->start_offset, action->deleted_text, tab_info->id);
		free(action->deleted_text);
	}

	free(action);
}