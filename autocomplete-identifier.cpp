
#include <ctype.h>
#include "declarations.h"

extern GtkWidget *notebook;

//const char *words[] = {
//	"word",
//	"auto",
//	"automatic",
//	"automobile",
//	"autocomplete",
//	0,
//};

//static void
//handle_text_insertion_event(
//GtkTextBuffer *text_buffer, GtkTextIter *location, char *inserted_text, int len, gpointer user_data)
//{
//	printf("handle_text_insertion_event()\n");
//
//	if (len > 1)
//	{
//		printf("dont autocomplete: %s\n", inserted_text);
//		return;
//	}
//}

bool is_prefixed_with(const char *string, const char *prefix){
	bool is_prefixed = true;
	int i;
	for(i = 0; prefix[i] != 0; ++i){
		if(prefix[i] != string[i]){
			is_prefixed = false;
			break;
		}
	}
	if(string[i] == 0) is_prefixed = false;

	return is_prefixed;
}

Array<Identifier *> *find_prefixed_with(Array<Identifier *> *identifiers, const char *prefix){
	Array<Identifier *> *prefixed_with = (Array<Identifier *> *) malloc(sizeof(Array<Identifier *>));
	array_init(prefixed_with);

	for(int i = 0; i < identifiers->count; ++i){
		Identifier *identifier = identifiers->data[i];
		if(is_prefixed_with(identifier->name, prefix)){
			array_add(prefixed_with, identifier); // copying the pointer only
		}
	}

	return prefixed_with;
}

void count_identifier(Array<Identifier *> *identifiers, const char *identifier){
	for(int i = 0; i < identifiers->count; ++i){
		if(strcmp(((Identifier *) identifiers->data[i])->name, identifier) == 0){
//			((Identifier *) identifiers->data[i])->count += 1;
			identifiers->data[i]->count += 1;
			return;
		}
	}
	// didnt find the identifier
	Identifier *identifier_count = (Identifier *) malloc(sizeof(Identifier));
	identifier_count->count = 1;
	identifier_count->name = strdup(identifier);
	array_add(identifiers, identifier_count);
}

#define BUFFER_SIZE 100
//@ UTF
// 32 (space) ... 126 (~) -- I think these are all printable ascii characters
// 127 - 32 = 95
struct TrieNode {
//	bool end_of_word;
	int count;
	char identifier[BUFFER_SIZE];
	TrieNode *chars[95];
};

TrieNode *node_new(){
	TrieNode *n = (TrieNode *) malloc(sizeof(TrieNode));

//	n->end_of_word = false;
	n->count = 0;

	for(int i = 0; i < COUNT(n->chars); ++i){
		n->chars[i] = NULL;
	}

	return n;
}

void count_identifier(TrieNode *root, const char *identifier){
	TrieNode *n = root;
	for(int i = 0; identifier[i] != 0; ++i){
		int index = identifier[i] - 32;
		if(n->chars[index]){
//			printf("found '%c'\n", identifier[i]);
			n = n->chars[index];
		}else{
//			printf("didnt find character '%c'\n", identifier[i]);
			n->chars[index] = node_new();
			n = n->chars[index];
		}
	}

	if(n->count == 0){
		strncpy(n->identifier, identifier, BUFFER_SIZE);
		// if BUFFER_SIZE characters are copied, we dont get a 0-terminated string.
		n->identifier[BUFFER_SIZE-1] = 0;
	}

	n->count += 1;
}

void walk_and_add(TrieNode *tree, Array<Identifier *> *array){
	if(tree->count > 0){
//		printf("%d -- %s\n", identifiers->count, identifiers->identifier);
		Identifier *identifier = (Identifier *) malloc(sizeof(Identifier));
		identifier->name = tree->identifier; // copy a pointer
		identifier->count = tree->count;
		array_add(array, identifier);
	}

	for(int i = 0; i < COUNT(tree->chars); ++i){
		if(tree->chars[i]){
			walk_and_add(tree->chars[i], array);
		}
	}
}

//void print_identifiers(Node *identifiers){
//	walk(identifiers);
//}

Array<Identifier *> *find_prefixed_with(TrieNode *identifiers, const char *prefix){
	Array<Identifier *> *prefixed = (Array<Identifier *> *) malloc(sizeof(Array<Identifier *>));
	array_init(prefixed);

	TrieNode *n = identifiers;
	for(int i = 0; prefix[i] != 0; ++i){
		int index = prefix[i] - 32;
		if(n->chars[index]){
			n = n->chars[index];
		}else{
			// no identifiers which start with 'prefix'
			return prefixed;
		}
	}

	// iterate n's subnodes manually because we want to exclude 'n' so that "printf", for example, is not considered to be prefixed with "printf"
	for(int i = 0; i < COUNT(n->chars); ++i){
		if(n->chars[i]){
			walk_and_add(n->chars[i], prefixed);
		}
	}

	return prefixed;
}

void sort_identifiers(Array<Identifier *> *counts){
	while(true){
		bool no_swaps = true;
		for(int a = 0, b = 1; b < counts->count; a += 1, b += 1){
			Identifier *count_a = (Identifier *) counts->data[a];
			Identifier *count_b = (Identifier *) counts->data[b];
			if(count_a->count < count_b->count){ // sort in ascending order
				counts->data[a] = count_b;
				counts->data[b] = count_a;
				no_swaps = false;
			}
		}
		if(no_swaps) break; // if no swaps were made, array is sorted
	}
}

void print_identifiers_beautified(Array<Identifier *> *identifiers){
	int longest = 0;
//	const char *name = NULL;
	for(int i = 0; i < identifiers->count; ++i){
		Identifier *identifier = (Identifier *) identifiers->data[i];
		int l = strlen(identifier->name);
		if(l > longest){
			longest = l;
//			name = identifier->name;
		}
	}
//	printf("longest identifier: %s (%d)\n", name, longest);
	int x = longest + 3;

	for(int i = 0; i < identifiers->count; ++i){
		Identifier *identifier = (Identifier *) identifiers->data[i];
		int indent = x - strlen(identifier->name);
		printf("%s ", identifier->name);
		for(int i = 0; i < indent; ++i) printf(".");
		printf(" %d", identifier->count);
		printf("\n");
	}
	printf("---\n");
	printf("%d total\n\n", identifiers->count);
}

//void clear_suggestions(GObject *object, GParamSpec *pspec, gpointer _tab)
//{
//	printf("clear_suggestions()\n");
//	GtkWidget *tab = (GtkWidget *) _tab;
//
//	State *state = (State *) tab_retrieve_widget(tab, AUTOCOMPLETE_STATE);
//
//	if (state->dont_clear)
//	{
//		state->dont_clear = false;
//		printf("clear_suggestions(): do nothing\n");
//	}
//	else
//	{
//		if (state->suggestions)
//		{
//			printf("clear_suggestions(): reset suggestions begin\n");
//			free(state->suggestions);
//			state->suggestions = 0;
//			state->index = 0;
//
//			if (state->can_undo)
//			{
//				state->can_undo = false;
//				GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);
//				printf("AFTER ASSERTION\n");
//				gtk_text_buffer_delete_mark(text_buffer, state->start);
//				gtk_text_buffer_delete_mark(text_buffer, state->end);
//				printf("clear_suggestions(): reset suggestions end\n");
//			}
//		}
//	}
//}


/////////
// EXPORTED STUFF:
/////////

void autocomplete_print_identifiers(Array<Identifier *> *identifiers){
	for(int i = 0; i < identifiers->count; ++i){
		Identifier *identifier_count = (Identifier *) identifiers->data[i];
		printf("%s (%d)\n", identifier_count->name, identifier_count->count);
	}
}

Array<Identifier *> *autocomplete_get_identifiers(const char *text)
{
	Array<Identifier *> *identifiers = array_new<Identifier *>();

	//@ UTF8
	int i = 0;
	while(text[i] != 0){
		if(isalpha(text[i]) || text[i] == '_'){
			int start = i;
			while(isalnum(text[i]) || text[i] == '_') ++i;
			int end = i;

//			#define BUFFER_SIZE 100
//			char identifier[BUFFER_SIZE];
//			int j = 0;
//			for(int k = start; k < end && j < BUFFER_SIZE-1; ++k, ++j){
//				identifier[j] = text[k];
//			}
//			identifier[j] = 0;

			#define BUFFER_SIZE 100
			char identifier[BUFFER_SIZE];
//			memset(identifier, 'x', BUFFER_SIZE);
			int len = MIN(end - start, BUFFER_SIZE - 1);
			strncpy(identifier, text + start, len);
			identifier[len] = 0;

			count_identifier(identifiers, identifier);

			continue;
		}
		i += 1;
	}

	sort_identifiers(identifiers);
	return identifiers;
}

AutocompleteState *autocomplete_state_new(Array<Identifier *> *identifiers)
{
	AutocompleteState *state = (AutocompleteState *) malloc(sizeof(AutocompleteState));
	state->identifiers = identifiers;
	state->possible_completions_next = 0;
	state->possible_completions = 0;
	state->last_completion = 0;
	state->last_cursor_offset = 0; //@
	return state;
}

void autocomplete_state_free(AutocompleteState *state, GtkTextBuffer *text_buffer)
{
	for (int i = 0; i < state->identifiers->count; ++i)
	{
		free(state->identifiers->data[i]->name);
		free(state->identifiers->data[i]);
	}
	free(state->identifiers->data);
	free(state->identifiers);

	if (state->possible_completions)
	{
		free(state->possible_completions->data); // These are just "Identifier *"
		free(state->possible_completions);
	}

	if (state->last_completion)
	{
		assert(GTK_IS_TEXT_MARK(state->last_completion->start));
		assert(GTK_IS_TEXT_MARK(state->last_completion->end));
		gtk_text_buffer_delete_mark(text_buffer, state->last_completion->start);
		gtk_text_buffer_delete_mark(text_buffer, state->last_completion->end);
	}
	free(state->last_completion);

	free(state);
}

gboolean autocomplete_clear(GdkEventKey *key_event)
{
	printf("clear_autocompletion()\n");

	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	if (!tab)
	{
		return FALSE;
	}
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));
	assert(text_buffer);

	{
		GtkTextIter start, end;
		if(gtk_text_buffer_get_selection_bounds(text_buffer, &start, &end)){
			return FALSE;
		}
	}

	AutocompleteState *state = (AutocompleteState *) tab_retrieve_widget(tab, AUTOCOMPLETE_STATE);

	if (state->last_completion)
	{
		GtkTextIter start, end;
		gtk_text_buffer_get_iter_at_mark(text_buffer, &start, state->last_completion->start);
		gtk_text_buffer_get_iter_at_mark(text_buffer, &end, state->last_completion->end);
		gtk_text_buffer_delete(text_buffer, &start, &end);
		gtk_text_buffer_delete_mark(text_buffer, state->last_completion->start);
		gtk_text_buffer_delete_mark(text_buffer, state->last_completion->end);
		free(state->last_completion);
		state->last_completion = 0;
	}

	return TRUE;
}

/*
cases:
	- nothing to autocomplete
	- something to autocomplete
		- should start new autocomplete
		- pick up where we left off last time


was there a previous autocompletion?
yes:
	should we undo that? (is the cursor location same it was after previous autocompletion?)
	yes:
		undo that
	no:
	both cases, get rid of old autocompletion info
no:
	do nothing
	
is there something to autocomplete?
yes:
	should we get a new list of possible completions? (check previous cursor position)
	(no list OR cursor pos not the same)
	yes:
		if we have them, get rid of old completions
		get completions
		pick first completion, autocomplete
		save autocompletion location and cursor pos (maybe store iterator, then later also check if the iterator is valid?)
	no:
		pick next completion, autocomplete
		save autocompletion location
no:
	just exit

---

is there something to autocomplete?
yes:
	do we have a list of possible completions?
	yes:
		pick next completion, autocomplete
		save autocompletion location
	no:
		get completions
		pick first completion, autocomplete
		save autocompletion location
no:
	just exit
*/

//@ First character of an identifier cant be a digit. Should we check for this?
// You know what, lets not even check if we are at the end of an identifier -- we can autocomplete in the middle of an identifier just as easily.
// use cursor position to determine if we need a new list of completions?
gboolean autocomplete_emacs_style(GdkEventKey *key_event)
{
	printf("autocomplete_emacs_style()\n");

	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	if (!tab)
	{
		printf("NO TABS\n");
		return FALSE;
	}
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));
	assert(text_buffer);

	{
		GtkTextIter start, end;
		if(gtk_text_buffer_get_selection_bounds(text_buffer, &start, &end)){
			return FALSE;
		}
	}

	AutocompleteState *state = (AutocompleteState *) tab_retrieve_widget(tab, AUTOCOMPLETE_STATE);
//	autocomplete_print_identifiers(state->identifiers);

	GtkTextIter cursor_pos;
	GtkTextMark *cursor_pos_mark = gtk_text_buffer_get_insert(text_buffer); // special mark, cant be deleted

	//@ using cursor position is not actually correct, but it should work most of the time
	// it does crop up sometimes, could store the autocompleted identifier alongside cursor position maybe?
	bool keep_last_completion = false;
	{
		gtk_text_buffer_get_iter_at_mark(text_buffer, &cursor_pos, cursor_pos_mark);
		guint cursor_pos_offset = gtk_text_iter_get_offset(&cursor_pos);
		printf("current cursor position: %d, last cursor position: %d\n", cursor_pos_offset, state->last_cursor_offset);
		if (cursor_pos_offset != state->last_cursor_offset)
		{
			keep_last_completion = true;
		}
	}
	printf("keep_last_completion: %s\n", keep_last_completion ? "true" : "false");

	if (state->last_completion)
	{
		if (!keep_last_completion)
		{
			GtkTextIter start, end;
			gtk_text_buffer_get_iter_at_mark(text_buffer, &start, state->last_completion->start);
			gtk_text_buffer_get_iter_at_mark(text_buffer, &end, state->last_completion->end);
			gtk_text_buffer_delete(text_buffer, &start, &end);

		}

		gtk_text_buffer_delete_mark(text_buffer, state->last_completion->start);
		gtk_text_buffer_delete_mark(text_buffer, state->last_completion->end);
		free(state->last_completion);
		state->last_completion = 0;
	}

	//@ UTF8
	gtk_text_buffer_get_iter_at_mark(text_buffer, &cursor_pos, cursor_pos_mark);
	GtkTextIter iter = cursor_pos;
	GtkTextIter start, end;
	end = iter;
	bool exclude_char = false;
	while (gtk_text_iter_backward_char(&iter))
	{
		gunichar c = gtk_text_iter_get_char(&iter);
		if (!isalnum(c) && c != '_'){
			exclude_char = true;
			break;
		}
	}
	if(exclude_char){
		gtk_text_iter_forward_char(&iter);
	}
	start = iter;

	if (gtk_text_iter_compare(&start, &end) >= 0) // start - end >= 0 means that 'start' is greater or equal to 'end'
	{
		printf("nothing to autocomplete\n");
		return FALSE;
	}

	char *text_to_autocomplete = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
	printf("we have text to autocomplete: %s\n", text_to_autocomplete);

	if (!state->possible_completions || keep_last_completion)
	{
		printf("GETTING A NEW SET OF POSSIBLE COMPLETIONS\n");
		Array<Identifier *> *possible_completions = find_prefixed_with(state->identifiers, text_to_autocomplete);
		state->possible_completions = possible_completions;
		state->possible_completions_next = 0;
	}

	if (state->possible_completions->count)
	{
		int len = strlen(text_to_autocomplete);
		const char *text_to_insert =
			&state->possible_completions->data[state->possible_completions_next]->name[len];
	//	const char *text_to_insert = state->possible_completions->data[state->possible_completions_next]->name;
		
		// After insertion the mark with the left gravity should stay at the beginning of the inserted text and the mark with the right gravity should move to the end of the inserted text.
		GtkTextMark *left = gtk_text_buffer_create_mark(text_buffer, NULL, &cursor_pos, TRUE);
		GtkTextMark *right = gtk_text_buffer_create_mark(text_buffer, NULL, &cursor_pos, FALSE);
		gtk_text_buffer_insert(text_buffer, &cursor_pos, text_to_insert, -1);
	
		state->last_completion = (CompletionInfo *) malloc(sizeof(CompletionInfo));
		state->last_completion->start = left;
		state->last_completion->end = right;
	
		state->possible_completions_next += 1;
		state->possible_completions_next %= state->possible_completions->count;
	}
	else
	{
		printf("no possible completions for '%s'\n", text_to_autocomplete);
	}
	
	gtk_text_buffer_get_iter_at_mark(text_buffer, &cursor_pos, cursor_pos_mark);
	guint cursor_pos_offset = gtk_text_iter_get_offset(&cursor_pos);
	state->last_cursor_offset = cursor_pos_offset;

	return TRUE;
}

// initializes autocomplete-identifier-feature for a tab
void autocomplete_identifier_init(GtkWidget *tab, GtkTextBuffer *text_buffer)
{
	LOG_MSG("initialize_identifier_autocompletion()\n");

	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(text_buffer, &start, &end);
	char *contents = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);

	Array<Identifier *> *identifiers = autocomplete_get_identifiers(contents);
	sort_identifiers(identifiers);
//	autocomplete_print_identifiers(identifiers);

	AutocompleteState *state = autocomplete_state_new(identifiers);
	tab_add_widget_4_retrieval(tab, AUTOCOMPLETE_STATE, state);

	free(contents);

//	g_signal_connect(G_OBJECT(text_buffer),
//		"notify::cursor-position", G_CALLBACK(clear_suggestions), tab);
}