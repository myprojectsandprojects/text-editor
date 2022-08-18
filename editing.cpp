#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

//#include "tab.h"
#include "declarations.h"

extern GtkWidget *notebook;


static gboolean init(GtkTextView **pview, GtkTextBuffer **pbuffer)
{
	GtkTextView *view;
	GtkTextBuffer *buffer; 

	view = GTK_TEXT_VIEW(
		visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW));

	if (view == NULL) {
		LOG_MSG("init(): no tabs open -> exiting\n");
		return FALSE;
	}

	if (!gtk_widget_is_focus(GTK_WIDGET(view))) {
		LOG_MSG("init(): text-view not in focus\n");
		return FALSE;
	}

	buffer = gtk_text_view_get_buffer(view);
	assert(buffer);

	*pview = view;
	*pbuffer = buffer;

	return TRUE;
}


// m, i, o -- output values
// output values are optional (pass in NULL if you dont want one)
void get_cursor_position(GtkTextBuffer *buffer, GtkTextMark **pm, GtkTextIter *pi, gint *po)
{
	LOG_MSG("get_cursor_position()\n");

	GtkTextMark *m;
	GtkTextIter i;
	gint o;

	//assert(buffer);
	m = gtk_text_buffer_get_mark(buffer, "insert");
	assert(m);
	gtk_text_buffer_get_iter_at_mark(buffer, &i, m);
	o = gtk_text_iter_get_offset(&i);
	LOG_MSG("get_cursor_position(): cursor offset: %d\n", o);

	if (pm) *pm = m;
	if (pi) *pi = i;
	if (po) *po = o;
}


// returns a NULL-terminated list of opening-tag names
char **get_opening_tags(GtkTextIter *i)
{
	LOG_MSG("get_opening_tags()\n");

	GSList *tags = gtk_text_iter_get_toggled_tags(i, TRUE); // toggled on: TRUE
	//guint n_tags = g_slist_length(tags);
	//gunichar c = gtk_text_iter_get_char(i_token_begin);
	//printf("%c: (%d)\n", c, n_tags);

	//const int MAX_NAME_LENGTH = 100; // ... ascii-characters
	const int MAX_LIST_LENGTH = 10; // ... strings

	char **names = (char **) malloc(sizeof(char *) * (MAX_LIST_LENGTH + 1)); // + terminating-NULL

	int j = 0;
	for (GSList *p = tags; p != NULL; p = p->next, ++j) {
		char *tag_name;
		GtkTextTag *tag = (GtkTextTag *) p->data;
		g_object_get(tag, "name", &tag_name, NULL);
		//printf("tag name: %s\n", tag_name);

		//char *name = malloc(MAX_NAME_LENGTH + 1); // + terminating-0
		char *name = (char *) malloc(strlen(tag_name) + 1); // + terminating-0
		sprintf(name, "%s", tag_name);
		names[j] = name;
	}
	names[j] = NULL;

	return names;	
}


// moves iterator to the beginning of the next token (returns TRUE)
// does nothing if no next token (returns FALSE)
gboolean move_to_next_token(GtkTextIter *pi)
{
	LOG_MSG("move_to_next_token()\n");

	GtkTextIter i = *pi;

	char **tag_names;
	gboolean found = FALSE;
	while (!found && !gtk_text_iter_is_end(&i)) {
		gtk_text_iter_forward_char(&i);
		tag_names = get_opening_tags(&i); //@ free this and others
		for (int i = 0; tag_names[i] != NULL; ++i) {
			if (strcmp(tag_names[i], "line-highlight") != 0) {
				found = TRUE;
				break;
			}
		}
	}

	if (found) {
		*pi = i;
		return TRUE;
	} else {
		return FALSE;
	}
}


// moves iterator to the beginning of the previous (or current) token (returns TRUE)
// does nothing if no previous token (returns FALSE)
gboolean move_to_prev_token(GtkTextIter *pi)
{
	LOG_MSG("move_to_prev_token()\n");

	GtkTextIter i = *pi;

	char **tag_names;
	gboolean found = FALSE;
	while (!found && !gtk_text_iter_is_start(&i)) {
		gtk_text_iter_backward_char(&i);
		tag_names = get_opening_tags(&i);
		for (int i = 0; tag_names[i] != NULL; ++i) {
			if (strcmp(tag_names[i], "line-highlight") != 0) {
				found = TRUE;
				break;
			}
		}
	}

	if (found) {
		*pi = i;
		return TRUE;
	} else {
		return FALSE;
	}
}


//
// moves iter to the start of the token and returns TRUE
// or if couldnt find a start of the token, does nothing and returns FALSE
//
gboolean move_to_token_start(GtkTextIter *iter)
{
/*
	gunichar c = gtk_text_iter_get_char(iter);	
	if (c == ' ' || c == '\t' || c == '\n') // oh I dont know..
		return FALSE;
*/
	GtkTextIter i = *iter;
	gboolean found = FALSE;

	do {
		GSList* tags = gtk_text_iter_get_toggled_tags(&i, TRUE); // toggled on: TRUE
		guint n_tags = g_slist_length(tags);
		//gunichar c = gtk_text_iter_get_char(i_token_begin);
		//printf("%c: (%d)\n", c, n_tags);

		if (n_tags == 0) continue;

		for (GSList *p = tags; p != NULL; p = p->next) {
			char *name;
			GtkTextTag *tag = (GtkTextTag *) p->data;
			g_object_get(tag, "name", &name, NULL);
			//printf("%s, ", name);
			if (strcmp(name, "line-highlight") != 0) {
				found = TRUE; // once we find a tag other than line-highlight were done..
				break;
			}
		}
		
		if (found) break;
	} while (gtk_text_iter_backward_char(&i));

	if (found) {
		*iter = i;
	}

	return found;
}


//
// moves iter to the end of the token and returns TRUE
// or if couldnt find an end of the token, does nothing and returns FALSE
//
gboolean move_to_token_end(GtkTextIter *iter)
{
	GtkTextIter i = *iter; 
	gboolean found = FALSE;
	gboolean not_at_end = TRUE;

	while (not_at_end) { // returns false when reaches the end
		not_at_end = gtk_text_iter_forward_char(&i); // if this hits the end, it returns FALSE
		GSList* tags = gtk_text_iter_get_toggled_tags(&i, FALSE); // toggled on: FALSE
		guint n_tags = g_slist_length(tags);

		/*gunichar c = gtk_text_iter_get_char(&i);
		printf("%c: (%d)\n", c, n_tags);*/

		if (n_tags == 0) continue;
		for (GSList *p = tags; p != NULL; p = p->next) {
			GtkTextTag *tag = (GtkTextTag *) p->data;
			char *name;
			g_object_get(tag, "name", &name, NULL);
			//printf("%s, ", name);
			if (strcmp(name, "line-highlight") != 0) {
				found = TRUE; // once we find a tag other than line-highlight were done..
				break;
			}
		}
		
		if (found) break;
	}

	if (found) {
		*iter = i;
	}

	return found;
}


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
	if(gtk_text_iter_forward_search(selection_start, "\t", (GtkTextSearchFlags) 0, &match_start, &match_end, NULL) == TRUE) {
		if(gtk_text_iter_compare(&match_start, selection_end) < 0)
			gtk_text_buffer_delete(text_buffer, &match_start, &match_end);
	}
	gtk_text_buffer_get_iter_at_mark(text_buffer, selection_start, start);
	gtk_text_buffer_get_iter_at_mark(text_buffer, selection_end, end);

	gtk_text_iter_forward_line(selection_start);

	while(gtk_text_iter_compare(selection_start, selection_end) < 0) {
		gtk_text_buffer_move_mark(text_buffer, start, selection_start);
		if(gtk_text_iter_forward_search(selection_start, "\t", (GtkTextSearchFlags) 0, &match_start, &match_end, NULL) == TRUE) {
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

	@ just have 2 functions: 1 handles tab, another handles shift+tab.
	and map them directly to key-combinations
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


char *get_line_indent(GtkTextBuffer *buffer, GtkTextIter *pi)
{
	GtkTextIter i, i_indent_begin, i_indent_end;

	i = *pi;
	gtk_text_iter_set_line_offset(&i, 0);
	i_indent_end = i_indent_begin = i;

	while (1) {
		gunichar c = gtk_text_iter_get_char(&i_indent_end);
		if (c == ' ' || c == '\t')
			gtk_text_iter_forward_char(&i_indent_end);
		else
			break;
	}
	char *indent = gtk_text_buffer_get_text(buffer, &i_indent_begin, &i_indent_end, FALSE);
	if (strlen(indent) > 0)
		LOG_MSG("get_line_indent(): indent: %s\n", indent);
	else
		LOG_MSG("get_line_indent(): no indent\n");

	return indent;
}


gboolean insert_line_before(GdkEventKey *key_event)
{
	GtkTextView *view;
	GtkTextBuffer *buffer;

	printf("insert_line_before()\n");
	
	if (!init(&view, &buffer)) return FALSE;

	GtkTextIter iter_start, i;
	gtk_text_buffer_get_start_iter(buffer, &iter_start);

	get_cursor_position(buffer, NULL, &i, NULL);

	char insert[100];
	char *indent = get_line_indent(buffer, &i);
	snprintf(insert, 100, "%s\n", indent);

	gtk_text_iter_set_line_offset(&i, 0);

	GtkTextMark *m = gtk_text_buffer_create_mark(buffer, NULL, &i, FALSE);
	gtk_text_buffer_insert(buffer, &i, insert, -1);
	gtk_text_buffer_get_iter_at_mark(buffer, &i, m);
	//gtk_text_iter_forward_chars(&i, indent_len);
	gtk_text_iter_backward_char(&i);
	gtk_text_buffer_place_cursor(buffer, &i);
	gtk_text_buffer_delete_mark(buffer, m);

	return TRUE;
}


gboolean insert_line_after(GdkEventKey *key_event)
{
	GtkTextView *view;
	GtkTextBuffer *buffer;

	printf("insert_line_before()\n");
	
	if (!init(&view, &buffer)) return FALSE;

	GtkTextIter i;

	get_cursor_position(buffer, NULL, &i, NULL);

	char insert[100];
	char *indent = get_line_indent(buffer, &i);
	snprintf(insert, 100, "%s\n", indent);

	gtk_text_iter_forward_line(&i); // to the beginning of the next line or to the end of the current line (if already on the last line)
	GtkTextMark *m = gtk_text_buffer_create_mark(buffer, NULL, &i, FALSE);
	if (!gtk_text_iter_is_end(&i)) {
		snprintf(insert, 100, "%s\n", indent);
		gtk_text_buffer_insert(buffer, &i, insert, -1);
		gtk_text_buffer_get_iter_at_mark(buffer, &i, m);
		gtk_text_iter_backward_char(&i);
	} else {
		snprintf(insert, 100, "\n%s", indent);
		gtk_text_buffer_insert(buffer, &i, insert, -1);
		gtk_text_buffer_get_iter_at_mark(buffer, &i, m);
	}
	gtk_text_buffer_place_cursor(buffer, &i);

	return TRUE;
}

gboolean move_lines_up(GdkEventKey *key_event)
{
	LOG_MSG("move_lines_up()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	// Delete the line before selected lines.
	// Insert the line after selected lines

	GtkTextIter selection_start_iter, selection_end_iter, iter;
	gtk_text_buffer_get_selection_bounds(buffer, &selection_start_iter, &selection_end_iter);

	iter = selection_start_iter;
	gboolean result = gtk_text_iter_backward_lines(&iter, 1); // Go to the beginning of the previous line
	if (result == FALSE) {
		LOG_MSG("move_lines_up(): no lines before selection -> earlyout\n");
		return TRUE;
	}
	
	GtkTextIter start_prev_line, end_prev_line;
	start_prev_line = iter;
	gtk_text_iter_forward_line(&iter);
	end_prev_line = iter;
	char *text = gtk_text_buffer_get_text(buffer, &start_prev_line, &end_prev_line, FALSE);
	//printf("move_lines_up(): previous line: %s\n", text);

	iter = selection_end_iter;
	gtk_text_iter_forward_line(&iter);
	GtkTextMark *keep_this = gtk_text_buffer_create_mark(buffer, NULL, &iter, TRUE);
	gtk_text_buffer_delete(buffer, &start_prev_line, &end_prev_line);
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, keep_this);
	gtk_text_buffer_insert(buffer, &iter, text, -1);
	gtk_text_buffer_delete_mark(buffer, keep_this);

	return TRUE;
}


gboolean move_lines_down(GdkEventKey *key_event)
{
	LOG_MSG("move_lines_down()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	// Delete the line after selected lines.
	// Insert the line before selected lines

	GtkTextIter selection_start_iter, selection_end_iter, iter;
	gtk_text_buffer_get_selection_bounds(buffer, &selection_start_iter, &selection_end_iter);

	iter = selection_end_iter;
	gboolean result = gtk_text_iter_forward_line(&iter);
	if (result == FALSE) {
		LOG_MSG("move_lines_down(): no lines after selection -> earlyout\n");
		return TRUE;
	}
	GtkTextIter start_next_line, end_next_line;
	start_next_line = iter;
	gtk_text_iter_forward_line(&iter);
	end_next_line = iter;
	char *text = gtk_text_buffer_get_text(buffer, &start_next_line, &end_next_line, FALSE);
	//printf("move_lines_down(): next line: %s\n", text);

	iter = selection_start_iter;
	gtk_text_iter_set_line_offset(&iter, 0);
	GtkTextMark *keep_this = gtk_text_buffer_create_mark(buffer, NULL, &iter, TRUE);
	gtk_text_buffer_delete(buffer, &start_next_line, &end_next_line);
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, keep_this);
	gtk_text_buffer_insert(buffer, &iter, text, -1);
	gtk_text_buffer_delete_mark(buffer, keep_this);

	return TRUE;
}


gboolean duplicate_line(GdkEventKey *key_event)
{
	LOG_MSG("duplicate_line()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter start, end;

	GtkTextMark *cursor = gtk_text_buffer_get_mark(buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(buffer, &start, cursor);
	end = start,
	gtk_text_iter_set_line_offset(&start, 0);
	gtk_text_iter_forward_line(&end);
	GtkTextMark *start_mark = gtk_text_buffer_create_mark(buffer, NULL, &start, FALSE);
	GtkTextMark *end_mark = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE);
	if (gtk_text_iter_is_end(&end)) {
		gtk_text_iter_backward_char(&end);
		gunichar c = gtk_text_iter_get_char(&end);
		gtk_text_iter_forward_char(&end);
		if (c != '\n') {
			gtk_text_buffer_insert(buffer, &end, "\n", -1);
			gtk_text_buffer_get_iter_at_mark(buffer, &start, start_mark);
			gtk_text_buffer_get_iter_at_mark(buffer, &end, end_mark);
		}
	}
	char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
	//printf("duplicate_line(): inserting: %s\n", text);
	gtk_text_buffer_insert(buffer, &end, text, -1);
	/*gtk_text_iter_backward_char(&iter); //@ at the end of the buffer -- bug?
	gtk_text_buffer_place_cursor(buffer, &iter);*/
	gtk_text_buffer_delete_mark(buffer, start_mark);
	gtk_text_buffer_delete_mark(buffer, end_mark);

	return TRUE;
}


gboolean delete_line(GdkEventKey *key_event)
{
	LOG_MSG("delete_line()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter start, end;

	GtkTextMark *cursor = gtk_text_buffer_get_mark(buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(buffer, &start, cursor);
	end = start,
	gtk_text_iter_set_line_offset(&start, 0);
	gtk_text_iter_forward_line(&end);
	gtk_text_buffer_delete(buffer, &start, &end);

	return TRUE;
}

gboolean change_line(GdkEventKey *key_event)
{
	printf("change_line()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i, i1, i2;

	get_cursor_position(buffer, NULL, &i, NULL);

	gtk_text_iter_set_line_offset(&i, 0);
	while (1) {
		gunichar c = gtk_text_iter_get_char(&i);
		if (c == ' ' || c == '\t')
			gtk_text_iter_forward_char(&i);
		else
			break;
	}
	i1 = i;
	gtk_text_iter_forward_line(&i);
	if (!gtk_text_iter_is_end(&i))
		gtk_text_iter_backward_char(&i);
	i2 = i;
	gtk_text_buffer_delete(buffer, &i1, &i2);

	return TRUE;
}

gboolean delete_end_of_line(GdkEventKey *key_event)
{
	printf("delete_end_of_line()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i, i1, i2;

	get_cursor_position(buffer, NULL, &i, NULL);
/*
	while (1) {
		gunichar c = gtk_text_iter_get_char(&i);
		if (c == ' ' || c == '\t')
			gtk_text_iter_forward_char(&i);
		else
			break;
	}
*/
	i1 = i;
	gtk_text_iter_forward_line(&i);
	if (!gtk_text_iter_is_end(&i))
		gtk_text_iter_backward_char(&i);
	i2 = i;
	gtk_text_buffer_delete(buffer, &i1, &i2);

	return TRUE;
}


gboolean delete_word(GdkEventKey *key_event)
{
	printf("delete_word()\n");

	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);
	if (!text_buffer) {
		return FALSE;
	}

	GtkTextIter i;
	get_cursor_position(text_buffer, NULL, &i, NULL);
	gunichar c = gtk_text_iter_get_char(&i);
	printf("*** character at cursor: %c\n", c);

	// check if we are at the beginning of a word
	gboolean is_iterator_moved = gtk_text_iter_backward_char(&i);
	gunichar c_before;
	gboolean at_start = FALSE; // whether we are at the beginning of the buffer
	if (is_iterator_moved) {
		c_before = gtk_text_iter_get_char(&i);
	} else {
		c_before = '\0'; // what value should we use here? if an iterator points at the end of the buffer, then what is the value?
		at_start = TRUE;
	}
	printf("*** character before cursor: %c\n", c_before);
	// let's also include numbers
	if ( !((isalnum(c) || c == '_') && !(isalnum(c_before) || c_before == '_')) ) {
		// we are not at the beginning of a word
		return TRUE;
	}
	if (!at_start) {
		gtk_text_iter_forward_char(&i);
	}
	GtkTextIter start = i;
	gunichar ch;
	while (gtk_text_iter_forward_char(&i)) {
		ch = gtk_text_iter_get_char(&i);
		if (!(isalnum(ch) || ch == '_')) {
			break;
		}
	}
	gtk_text_buffer_delete(text_buffer, &start, &i);

	return TRUE;
}


//@ think more about it
// for the completness sake, we should also do [] and <>
// doublequotes seem complicated because if we see a doublequote
// then how would we know if its a closing or opening doublequote without
// counting doublequotes from the beginning of the file?
gboolean delete_inside(GdkEventKey *key_event)
{
	printf("delete_inside()\n");

	GtkTextBuffer *text_buffer =
		(GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);
	if (!text_buffer) return FALSE;

	GtkTextIter i;
	get_cursor_position(text_buffer, NULL, &i, NULL);
	//gunichar c = gtk_text_iter_get_char(&i);
	//printf("*** character at cursor: %c\n", c);

/*
	// lets try doublequotes
	// this feature only makes sense if the language is C!
	// take this line of code: "gtk_text_iter_get_char(&k) == '"'" -- there is a single doublequote here

	bool inside_str = false;
	bool inside_comment = false;
	GtkTextIter start, end;

	GtkTextIter j;
	for (gtk_text_buffer_get_start_iter(text_buffer, &j);
		gtk_text_iter_compare(&j, &i) < 0;
		gtk_text_iter_forward_char(&j))
	{
		//printf(" %c", gtk_text_iter_get_char(&j));
		if (gtk_text_iter_get_char(&j) == '/') {
			gtk_text_iter_forward_char(&j);
			if (gtk_text_iter_get_char(&j) == '*') {
				// ignore characters inside a block comment
				while (gtk_text_iter_compare(&j, &i) < 0) {
					if (gtk_text_iter_get_char(&j) == '*') {
						gtk_text_iter_forward_char(&j);
						if (gtk_text_iter_get_char(&j) == '/') {
							break;
						}
					}
					gtk_text_iter_forward_char(&j);
				}
				continue;
			}
		} else if (gtk_text_iter_get_char(&j) == '*')
			if (gtk_text_iter_get_char(&j) == '"') {
				inside_str = !inside_str;
				if (inside_str) {
					start = j;
					gtk_text_iter_forward_char(&start);
				}
			}
		}
	}

	if (inside_str) {
		printf("is inside a string\n");
		// print the string
		GtkTextIter k;
		for (k = i; !gtk_text_iter_is_end(&k); gtk_text_iter_forward_char(&k)) {
			if (gtk_text_iter_get_char(&k) == '"') {
				end = k;
				break;
			}
		}
		gtk_text_buffer_delete(text_buffer, &start, &end);
	} else {
		printf("not inside a string\n");
	}
*/

	bool parenthesis_found_open,
		parenthesis_found_close,
		curlybrace_found_open,
		curlybrace_found_close;
	parenthesis_found_open = parenthesis_found_close = curlybrace_found_open = curlybrace_found_close = false;
	int parenthesis_nestedness, curlybrace_nestedness;
	curlybrace_nestedness = parenthesis_nestedness = 0;

	GtkTextIter start, end;
	start = end = i;

	while (gtk_text_iter_backward_char(&start)) {
		if (gtk_text_iter_get_char(&start) == ')') {
			parenthesis_nestedness += 1;
		} else if (gtk_text_iter_get_char(&start) == '(') {
			if (parenthesis_nestedness > 0) {
				parenthesis_nestedness -= 1;
			} else {
				parenthesis_found_open = true;
				gtk_text_iter_forward_char(&start);
				break;
			}
		} else if (gtk_text_iter_get_char(&start) == '}') {
			curlybrace_nestedness += 1;
		} else if (gtk_text_iter_get_char(&start) == '{') {
			if (curlybrace_nestedness > 0) {
				curlybrace_nestedness -= 1;
			} else {
				curlybrace_found_open = true;
				gtk_text_iter_forward_char(&start);
				break;
			}
		}
	}
	curlybrace_nestedness = parenthesis_nestedness = 0;

	do {
		if (gtk_text_iter_get_char(&end) == '(') {
			parenthesis_nestedness += 1;
		} else if (gtk_text_iter_get_char(&end) == ')') {
			if (parenthesis_nestedness > 0) {
				parenthesis_nestedness -= 1;
			} else {
				parenthesis_found_close = true;
				break;
			}
		} else if (gtk_text_iter_get_char(&end) == '{') {
			curlybrace_nestedness += 1;
		} else if (gtk_text_iter_get_char(&end) == '}') {
			if (curlybrace_nestedness > 0) {
				curlybrace_nestedness -= 1;
			} else {
				curlybrace_found_close = true;
				break;
			}
		}
	} while (gtk_text_iter_forward_char(&end));

	if (parenthesis_found_open && parenthesis_found_close && gtk_text_iter_compare(&start, &end) != 0
		|| curlybrace_found_open && curlybrace_found_close && gtk_text_iter_compare(&start, &end) != 0)
	{
		gtk_text_buffer_delete(text_buffer, &start, &end);
	}

	return TRUE;
}

gboolean select_inside(GdkEventKey *key_event)
{
	printf("select_inside()\n");

	GtkTextBuffer *text_buffer =
		(GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);
	if (!text_buffer) return FALSE;

	GtkTextIter i;
	get_cursor_position(text_buffer, NULL, &i, NULL);

	bool parenthesis_found_open,
		parenthesis_found_close,
		curlybrace_found_open,
		curlybrace_found_close;
	parenthesis_found_open = parenthesis_found_close = curlybrace_found_open = curlybrace_found_close = false;
	int parenthesis_nestedness, curlybrace_nestedness;
	curlybrace_nestedness = parenthesis_nestedness = 0;

	GtkTextIter start, end;
	start = end = i;

	while (gtk_text_iter_backward_char(&start)) {
		if (gtk_text_iter_get_char(&start) == ')') {
			parenthesis_nestedness += 1;
		} else if (gtk_text_iter_get_char(&start) == '(') {
			if (parenthesis_nestedness > 0) {
				parenthesis_nestedness -= 1;
			} else {
				parenthesis_found_open = true;
				gtk_text_iter_forward_char(&start);
				break;
			}
		} else if (gtk_text_iter_get_char(&start) == '}') {
			curlybrace_nestedness += 1;
		} else if (gtk_text_iter_get_char(&start) == '{') {
			if (curlybrace_nestedness > 0) {
				curlybrace_nestedness -= 1;
			} else {
				curlybrace_found_open = true;
				gtk_text_iter_forward_char(&start);
				break;
			}
		}
	}
	curlybrace_nestedness = parenthesis_nestedness = 0;

	do {
		if (gtk_text_iter_get_char(&end) == '(') {
			parenthesis_nestedness += 1;
		} else if (gtk_text_iter_get_char(&end) == ')') {
			if (parenthesis_nestedness > 0) {
				parenthesis_nestedness -= 1;
			} else {
				parenthesis_found_close = true;
				break;
			}
		} else if (gtk_text_iter_get_char(&end) == '{') {
			curlybrace_nestedness += 1;
		} else if (gtk_text_iter_get_char(&end) == '}') {
			if (curlybrace_nestedness > 0) {
				curlybrace_nestedness -= 1;
			} else {
				curlybrace_found_close = true;
				break;
			}
		}
	} while (gtk_text_iter_forward_char(&end));

	if (parenthesis_found_open && parenthesis_found_close && gtk_text_iter_compare(&start, &end) != 0
		|| curlybrace_found_open && curlybrace_found_close && gtk_text_iter_compare(&start, &end) != 0)
	{
		gtk_text_buffer_select_range(text_buffer, &start, &end);
	}

	return TRUE;
}

gboolean comment_block(GdkEventKey *key_event)
{
	printf("comment_block()\n");

	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	if (!tab) {
		return FALSE;
	}

	GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);

	GtkTextIter selection_start, selection_end;
	gboolean is_selection = gtk_text_buffer_get_selection_bounds(text_buffer, &selection_start, &selection_end);

/*
	if (!is_selection) {
		return FALSE; // not sure what makes sense here
	}
	printf("we have a selection\n");
*/

	// it seems that "selection_start" is always closer to the beginning of the buffer than end
	// also seems that "selection_start" and "selection_end" are never equal
	/*
	int offset_start 	= gtk_text_iter_get_offset(&selection_start);
	int offset_end 	= gtk_text_iter_get_offset(&selection_end);
	printf("start: %d, end: %d\n", offset_start, offset_end);
	*/

	GtkTextIter iter = selection_start;
	gtk_text_iter_set_line_offset(&iter, 0);

	GtkTextMark *m_iter = gtk_text_buffer_create_mark(text_buffer, NULL, &iter, TRUE);
	GtkTextMark *m_selection_end = gtk_text_buffer_create_mark(text_buffer, NULL, &selection_end, TRUE);
	
	gtk_text_buffer_insert(text_buffer, &iter, "//", -1);
	gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, m_iter);
	gtk_text_buffer_get_iter_at_mark(text_buffer, &selection_end, m_selection_end);

	gtk_text_iter_forward_line(&iter);
	while (!gtk_text_iter_is_end(&iter) && gtk_text_iter_compare(&iter, &selection_end) <= 0) {
		gtk_text_buffer_move_mark(text_buffer, m_iter, &iter);
		gtk_text_buffer_insert(text_buffer, &iter, "//", -1);
		gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, m_iter);
		gtk_text_buffer_get_iter_at_mark(text_buffer, &selection_end, m_selection_end);

		gtk_text_iter_forward_line(&iter);
	}

	// if there is selection, then get rid of it
	gtk_text_buffer_place_cursor(text_buffer, &selection_end);

/*
	// do it the other way around, see if we still get the invalidated interators
	//GtkTextIter iter = selection_end;
	gtk_text_iter_set_line_offset(&selection_end, 0);
	gtk_text_buffer_insert(text_buffer, &selection_end, "//", -1);

	gtk_text_iter_backward_line(&selection_end);
	while (gtk_text_iter_compare(&selection_end, &selection_start) > 0) {
		gtk_text_buffer_insert(text_buffer, &selection_end, "//", -1);
		gtk_text_iter_backward_line(&selection_end);
	}
*/
	return TRUE;
}

gboolean uncomment_block(GdkEventKey *key_event)
{
	printf("uncomment_block()\n");

	GtkWidget *tab = get_visible_tab(GTK_NOTEBOOK(notebook));
	if (!tab) {
		return FALSE;
	}

	GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);

	GtkTextIter selection_start, selection_end;
	gboolean is_selection = gtk_text_buffer_get_selection_bounds(text_buffer, &selection_start, &selection_end);

	GtkTextIter iter = selection_start;
	gtk_text_iter_set_line_offset(&iter, 0);

	while (!gtk_text_iter_is_end(&iter) && gtk_text_iter_compare(&iter, &selection_end) <= 0) {
		GtkTextIter range_start, range_end;
		if (gtk_text_iter_get_char(&iter) == '/') {
			range_start = iter;
			if (gtk_text_iter_forward_char(&iter) && gtk_text_iter_get_char(&iter) == '/') {
				range_end = iter;
				gtk_text_iter_forward_char(&range_end);
	
				GtkTextMark *m_iter = gtk_text_buffer_create_mark(text_buffer, NULL, &iter, TRUE);
				GtkTextMark *m_selection_end = gtk_text_buffer_create_mark(text_buffer, NULL, &selection_end, TRUE);
				gtk_text_buffer_delete(text_buffer, &range_start, &range_end);
				gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, m_iter);
				gtk_text_buffer_get_iter_at_mark(text_buffer, &selection_end, m_selection_end);
			}
		}
		gtk_text_iter_forward_line(&iter);
	}

	// if there is selection, then get rid of it
	gtk_text_buffer_place_cursor(text_buffer, &selection_end);
	
	return TRUE;
}

gboolean move_cursor_start_line(GdkEventKey *key_event)
{
	printf("move_cursor_start_line()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i;

	get_cursor_position(buffer, NULL, &i, NULL);

	gtk_text_iter_set_line_offset(&i, 0);
	while (1) {
		gunichar c = gtk_text_iter_get_char(&i);
		if (c == ' ' || c == '\t')
			gtk_text_iter_forward_char(&i);
		else
			break;
	}

	gtk_text_buffer_place_cursor(buffer, &i);

	return TRUE;
}

gboolean move_cursor_start_line_shift(GdkEventKey *key_event)
{
	printf("move_cursor_start_line_shift()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i, insertion, bound;

	get_cursor_position(buffer, NULL, &i, NULL);

	bound = i;

	gtk_text_iter_set_line_offset(&i, 0);
	while (1) {
		gunichar c = gtk_text_iter_get_char(&i);
		if (c == ' ' || c == '\t')
			gtk_text_iter_forward_char(&i);
		else
			break;
	}

	insertion = i;

	gtk_text_buffer_select_range(buffer, &insertion, &bound); // 1st -- insertion, 2nd -- selection bound

	return TRUE;
}


gboolean move_cursor_end_line(GdkEventKey *key_event)
{
	printf("move_cursor_end_line()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i;

	get_cursor_position(buffer, NULL, &i, NULL);

	gtk_text_iter_forward_line(&i);
	if (!gtk_text_iter_is_end(&i))
		gtk_text_iter_backward_char(&i);

	gtk_text_buffer_place_cursor(buffer, &i);

	return TRUE;
}

gboolean move_cursor_end_line_shift(GdkEventKey *key_event)
{
	printf("move_cursor_end_line_shift()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i, insertion, bound;

	get_cursor_position(buffer, NULL, &i, NULL);

	bound = i;

	gtk_text_iter_forward_line(&i);
	if (!gtk_text_iter_is_end(&i))
		gtk_text_iter_backward_char(&i);

	insertion = i;

	gtk_text_buffer_select_range(buffer, &insertion, &bound);

	return TRUE;
}

gboolean move_cursor_start_word(GdkEventKey *key_event)
{
	printf("move_cursor_start_word()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i;

	get_cursor_position(buffer, NULL, &i, NULL);

	//gboolean u = g_unichar_isalnum('ä');
	//int a = isalnum('ä');
	//printf("a: %d, u: %d\n", a, u); // a: 0, u: 1

	gunichar c = gtk_text_iter_get_char(&i);

	if (!g_unichar_isalnum(c) && c != '_') {
		gtk_text_iter_backward_char(&i);
		c = gtk_text_iter_get_char(&i);
		if (!g_unichar_isalnum(c) && c != '_') {
			// we are not "inside" a "word"
			printf("we are NOT \"inside\" a \"word\"\n");
			return TRUE;
		}
	}

	//printf("we are \"inside\" a \"word\"\n");
	while (gtk_text_iter_backward_char(&i)) {
		c = gtk_text_iter_get_char(&i);
		if (!g_unichar_isalnum(c) && c != '_') {
			gtk_text_iter_forward_char(&i);
			break;
		}
	}

	gtk_text_buffer_place_cursor(buffer, &i);

	return TRUE;
}

gboolean move_cursor_start_word_shift(GdkEventKey *key_event)
{
	printf("move_cursor_start_word()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i, insertion, bound;

	get_cursor_position(buffer, NULL, &i, NULL);

	bound = i;

	//gboolean u = g_unichar_isalnum('ä');
	//int a = isalnum('ä');
	//printf("a: %d, u: %d\n", a, u); // a: 0, u: 1

	gunichar c = gtk_text_iter_get_char(&i);

	if (!g_unichar_isalnum(c) && c != '_') {
		gtk_text_iter_backward_char(&i);
		c = gtk_text_iter_get_char(&i);
		if (!g_unichar_isalnum(c) && c != '_') {
			// we are not "inside" a "word"
			printf("we are NOT \"inside\" a \"word\"\n");
			return TRUE;
		}
	}

	//printf("we are \"inside\" a \"word\"\n");
	while (gtk_text_iter_backward_char(&i)) {
		c = gtk_text_iter_get_char(&i);
		if (!g_unichar_isalnum(c) && c != '_') {
			gtk_text_iter_forward_char(&i);
			break;
		}
	}

	insertion = i;
	gtk_text_buffer_select_range(buffer, &insertion, &bound);

	return TRUE;
}

gboolean move_cursor_end_word(GdkEventKey *key_event)
{
	printf("move_cursor_end_word()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i;

	get_cursor_position(buffer, NULL, &i, NULL);

	//gboolean u = g_unichar_isalnum('ä');
	//int a = isalnum('ä');
	//printf("a: %d, u: %d\n", a, u); // a: 0, u: 1

	gunichar c = gtk_text_iter_get_char(&i);

	if (!g_unichar_isalnum(c) && c != '_') {
		return TRUE;
	}

	while (gtk_text_iter_forward_char(&i)) {
		c = gtk_text_iter_get_char(&i);
		if (!g_unichar_isalnum(c) && c != '_') {
			break;
		}
	}

	gtk_text_buffer_place_cursor(buffer, &i);

	return TRUE;
}

gboolean move_cursor_end_word_shift(GdkEventKey *key_event)
{
	printf("move_cursor_end_word()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i, insertion, bound;

	get_cursor_position(buffer, NULL, &i, NULL);
	bound = i;

	//gboolean u = g_unichar_isalnum('ä');
	//int a = isalnum('ä');
	//printf("a: %d, u: %d\n", a, u); // a: 0, u: 1

	gunichar c = gtk_text_iter_get_char(&i);

	if (!g_unichar_isalnum(c) && c != '_') {
		return TRUE;
	}

	while (gtk_text_iter_forward_char(&i)) {
		c = gtk_text_iter_get_char(&i);
		if (!g_unichar_isalnum(c) && c != '_') {
			break;
		}
	}

	insertion = i;
	gtk_text_buffer_select_range(buffer, &insertion, &bound);

	return TRUE;
}

// eventually we would like to deal with '()', '{}', '[]' and maybe '<>'
// '<>' are also used as less than and greater than operators
gboolean move_cursor_opening(GdkEventKey *key_event)
{
	printf("move_cursor_opening()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i;
	get_cursor_position(buffer, NULL, &i, NULL);

	int nestedness = 0;
	while (gtk_text_iter_backward_char(&i)) {
		gunichar c = gtk_text_iter_get_char(&i);
		if (c == '}') {
			nestedness += 1;
		} else if (c == '{') {
			if (nestedness > 0) {
				nestedness -= 1;
			} else {
				gtk_text_buffer_place_cursor(buffer, &i);
//				// should scroll ONLY if necessary?
//				gtk_text_view_scroll_to_iter(view, &i, 0.0, TRUE, 0.0, 0.1);
				gtk_text_view_scroll_to_iter(view, &i, 0.0, FALSE, 0.0, 0.0);
				break;
			}
		}
	}

	return TRUE;
}

gboolean move_cursor_closing(GdkEventKey *key_event)
{
	printf("move_cursor_closing()\n");

	GtkTextView *view;
	GtkTextBuffer *buffer;

	gboolean rv = init(&view, &buffer);
	if (!rv)
		return rv;

	GtkTextIter i;
	get_cursor_position(buffer, NULL, &i, NULL);

	int nestedness = 0;
	while (gtk_text_iter_forward_char(&i)) {
		gunichar c = gtk_text_iter_get_char(&i);
		if (c == '{') {
			nestedness += 1;
		} else if (c == '}') {
			if (nestedness > 0) {
				nestedness -= 1;
			} else {
				gtk_text_buffer_place_cursor(buffer, &i);
//				// should scroll ONLY if necessary?
//				gtk_text_view_scroll_to_iter(view, &i, 0.0, TRUE, 0.0, 0.1);
				gtk_text_view_scroll_to_iter(view, &i, 0.0, FALSE, 0.0, 0.0);
				break;
			}
		}
	}

	return TRUE;
}


//gboolean move_cursor_left(GdkEventKey *key_event)
//{
//	printf("move_cursor_left()\n");
//
//	GtkTextView *view;
//	GtkTextBuffer *buffer;
//
//	gboolean rv = init(&view, &buffer);
//	if (!rv)
//		return rv;
//
//	GtkTextIter i, i_prev, i_sel_start, i_sel_end;
//
//	get_cursor_position(buffer, NULL, &i, NULL);
//	//printf("move_cursor_left(): cursor location: %d\n", o_cursor);
//
//	// store the previous location in case we should select (shift)
//	// get_selection_bounds() gives us the selection bounds in the ascending order
//	// we want the bound which is not equal to the cursor
//	if (key_event->state & GDK_SHIFT_MASK) {
//		if (gtk_text_buffer_get_selection_bounds(buffer, &i_sel_start, &i_sel_end)) {
//			if (gtk_text_iter_compare(&i_sel_start, &i) == 0) {
//				// i_sel_start is where the cursor is..
//				i_prev = i_sel_end;
//			} else {
//				// assuming i_sel_end is where the cursor is..
//				i_prev = i_sel_start;
//			}
//		} else {
//			i_prev = i;
//		}
//	}
//
//	gboolean moved = move_to_prev_token(&i);
//	if (!moved) {
//		// didnt find next token
//		//gtk_text_buffer_get_start_iter(&i);
//	}
//
//	// if we have shift, we should select
//	if (key_event->state & GDK_SHIFT_MASK) {
//		printf("move_cursor_left(): we have a shift\n");
//		gtk_text_buffer_select_range(buffer, &i, &i_prev); // 1st insertion, 2nd selection-bound
//	} else {
//		gtk_text_buffer_place_cursor(buffer, &i);
//		//gtk_text_view_set_cursor_visible(view, FALSE);
//		//gtk_text_view_set_cursor_visible(view, TRUE);
//		//gtk_text_view_reset_cursor_blink(view); 3.20 or something..
//	}
//	gtk_text_view_scroll_to_iter(view, &i, 0.0, FALSE, 0.0, 0.0);
//
//	return TRUE;
//}


//gboolean move_cursor_right(GdkEventKey *key_event)
//{
//	printf("move_cursor_right()\n");
//
//	GtkTextView *view;
//	GtkTextBuffer *buffer;
//
//	gboolean rv = init(&view, &buffer);
//	if (!rv)
//		return rv;
//
//	GtkTextIter i, i_prev, i_sel_start, i_sel_end;
//
//	get_cursor_position(buffer, NULL, &i, NULL);
//
//	// store the previous location in case we should select (shift)
//	// get_selection_bounds() gives us the selection bounds in the ascending order
//	// we want the bound which is not equal to the cursor
//	if (key_event->state & GDK_SHIFT_MASK) {
//		if (gtk_text_buffer_get_selection_bounds(buffer, &i_sel_start, &i_sel_end)) {
//			if (gtk_text_iter_compare(&i_sel_start, &i) == 0) {
//				// i_sel_start is where the cursor is..
//				i_prev = i_sel_end;
//			} else {
//				// assuming i_sel_end is where the cursor is..
//				i_prev = i_sel_start;
//			}
//		} else {
//			i_prev = i;
//		}
//	}
//
//	gboolean moved = move_to_next_token(&i);
//	if (!moved) {
//		// didnt find next token
//		gtk_text_buffer_get_end_iter(buffer, &i);
//	}
//
//	// if we have shift, we should select
//	if (key_event->state & GDK_SHIFT_MASK) {
//		printf("move_cursor_left(): we have a shift\n");
//		gtk_text_buffer_select_range(buffer, &i, &i_prev); // 1st insertion, 2nd selection-bound
//	} else {
//		gtk_text_buffer_place_cursor(buffer, &i);
//		//gtk_text_view_set_cursor_visible(view, FALSE);
//		//gtk_text_view_set_cursor_visible(view, TRUE);
//		//gtk_text_view_reset_cursor_blink(view); 3.20 or something..
//	}
//	gtk_text_view_scroll_to_iter(view, &i, 0.0, FALSE, 0.0, 0.0);
//
//	return TRUE;
//}

gboolean cursor_short_jump_left(GdkEventKey *key_event) {
	printf("cursor_short_jump_left()\n");

	GtkTextView *text_view;
	GtkTextBuffer *text_buffer;
	if (!init(&text_view, &text_buffer)) return FALSE;

	GtkTextIter iter;
	get_cursor_position(text_buffer, NULL, &iter, NULL);

	GtkTextIter cursor_pos = iter;

	gtk_text_iter_backward_char(&iter);
	gunichar ch = gtk_text_iter_get_char(&iter);

	if (g_unichar_isalpha(ch) && g_unichar_islower(ch))
	{
		// lowercase alpha
		do {
			if (!gtk_text_iter_backward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_isalpha(ch) && g_unichar_islower(ch));
	}
	else if (g_unichar_isalpha(ch) && g_unichar_isupper(ch))
	{
		// uppercase alpha
		do {
			if (!gtk_text_iter_backward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_isalpha(ch) && g_unichar_isupper(ch));
	}
	else if (g_unichar_isdigit(ch))
	{
		// digit
		do {
			if (!gtk_text_iter_backward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_isdigit(ch));
	}
	else if (g_unichar_ispunct(ch))
	{
		// punctuation
		do {
			if (!gtk_text_iter_backward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_ispunct(ch));
	}
	else if (g_unichar_isspace(ch))
	{
		// whitespace
		do {
			if (!gtk_text_iter_backward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_isspace(ch));
	}
	else
	{
		// everything else?
	}

	//@ this is not quite correct
	if (!gtk_text_iter_is_start(&iter))
		gtk_text_iter_forward_char(&iter);

	if (key_event->state & GDK_SHIFT_MASK) {
		// Check if we already have a selection.
		// If so, then add to the existing selection
		GtkTextIter sel_start, sel_end, new_sel_bound;
		if (gtk_text_buffer_get_selection_bounds(text_buffer, &sel_start, &sel_end)) {
			if (gtk_text_iter_compare(&sel_start, &cursor_pos) == 0) {
				// sel_start is where the cursor is
				new_sel_bound = sel_end;
			} else {
				// sel_end is where the cursor is
				new_sel_bound = sel_start;
			}
		} else {
			new_sel_bound = cursor_pos;
		}
		// 1st insertion, 2nd selection-bound
		gtk_text_buffer_select_range(text_buffer, &iter, &new_sel_bound);
	} else {
		gtk_text_buffer_place_cursor(text_buffer, &iter);
	}

	// if the cursor has moved outside the visible region, then scroll
	gtk_text_view_scroll_to_iter(text_view, &iter, 0.0, FALSE, 0.0, 0.0);

	return TRUE;
}

gboolean cursor_short_jump_right(GdkEventKey *key_event) {
	printf("cursor_short_jump_right()\n");

	GtkTextView *text_view;
	GtkTextBuffer *text_buffer;
	if (!init(&text_view, &text_buffer)) return FALSE;

	GtkTextIter iter;
	get_cursor_position(text_buffer, NULL, &iter, NULL);

	GtkTextIter cursor_pos = iter;

	gunichar ch = gtk_text_iter_get_char(&iter);

	if (g_unichar_isalpha(ch) && g_unichar_islower(ch))
	{
		// lowercase alpha
		do {
			if (!gtk_text_iter_forward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_isalpha(ch) && g_unichar_islower(ch));
	}
	else if (g_unichar_isalpha(ch) && g_unichar_isupper(ch))
	{
		// uppercase alpha
		do {
			if (!gtk_text_iter_forward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_isalpha(ch) && g_unichar_isupper(ch));
	}
	else if (g_unichar_isdigit(ch))
	{
		// digit
		do {
			if (!gtk_text_iter_forward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_isdigit(ch));
	}
	else if (g_unichar_ispunct(ch))
	{
		// punctuation
		do {
			if (!gtk_text_iter_forward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_ispunct(ch));
	}
	else if (g_unichar_isspace(ch))
	{
		// whitespace
		do {
			if (!gtk_text_iter_forward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_isspace(ch));
	}
	else
	{
		// everything else?
	}

//	if (!gtk_text_iter_is_start(&iter))
//		gtk_text_iter_forward_char(&iter);

	if (key_event->state & GDK_SHIFT_MASK) {
		// Check if we already have a selection.
		// If so, then add to the existing selection
		GtkTextIter sel_start, sel_end, new_sel_bound;
		if (gtk_text_buffer_get_selection_bounds(text_buffer, &sel_start, &sel_end)) {
			if (gtk_text_iter_compare(&sel_start, &cursor_pos) == 0) {
				// sel_start is where the cursor is
				new_sel_bound = sel_end;
			} else {
				// sel_end is where the cursor is
				new_sel_bound = sel_start;
			}
		} else {
			new_sel_bound = cursor_pos;
		}
		// 1st insertion, 2nd selection-bound
		gtk_text_buffer_select_range(text_buffer, &iter, &new_sel_bound);
	} else {
		gtk_text_buffer_place_cursor(text_buffer, &iter);
	}

	// if the cursor has moved outside the visible region, then scroll
	gtk_text_view_scroll_to_iter(text_view, &iter, 0.0, FALSE, 0.0, 0.0);

	return TRUE;
}

gboolean cursor_long_jump_left(GdkEventKey *key_event) {
	printf("cursor_long_jump_left()\n");

	GtkTextView *text_view;
	GtkTextBuffer *text_buffer;
	if (!init(&text_view, &text_buffer)) return FALSE;

	GtkTextIter iter;
	get_cursor_position(text_buffer, NULL, &iter, NULL);

	GtkTextIter cursor_pos = iter;

	gtk_text_iter_backward_char(&iter);
	gunichar ch = gtk_text_iter_get_char(&iter);

	if (g_unichar_isalnum(ch) || ch == '_')
	{
		do {
			if (!gtk_text_iter_backward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_isalnum(ch) || ch == '_');
	} else {
		do {
			if (!gtk_text_iter_backward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (!(g_unichar_isalnum(ch) || ch == '_'));
	}
//	else if (g_unichar_ispunct(ch))
//	{
//		// punctuation
//		do {
//			if (!gtk_text_iter_backward_char(&iter)) break;
//			ch = gtk_text_iter_get_char(&iter);
//		} while (g_unichar_ispunct(ch));
//	}
//	else if (g_unichar_isspace(ch))
//	{
//		// whitespace
//		do {
//			if (!gtk_text_iter_backward_char(&iter)) break;
//			ch = gtk_text_iter_get_char(&iter);
//		} while (g_unichar_isspace(ch));
//	}
//	else
//	{
//		// everything else?
//		assert(false);
//	}

	//@ this is not quite correct
	if (!gtk_text_iter_is_start(&iter))
		gtk_text_iter_forward_char(&iter);

	if (key_event->state & GDK_SHIFT_MASK) {
		// Check if we already have a selection.
		// If so, then add to the existing selection
		GtkTextIter sel_start, sel_end, new_sel_bound;
		if (gtk_text_buffer_get_selection_bounds(text_buffer, &sel_start, &sel_end)) {
			if (gtk_text_iter_compare(&sel_start, &cursor_pos) == 0) {
				// sel_start is where the cursor is
				new_sel_bound = sel_end;
			} else {
				// sel_end is where the cursor is
				new_sel_bound = sel_start;
			}
		} else {
			new_sel_bound = cursor_pos;
		}
		// 1st insertion, 2nd selection-bound
		gtk_text_buffer_select_range(text_buffer, &iter, &new_sel_bound);
	} else {
		gtk_text_buffer_place_cursor(text_buffer, &iter);
	}

	// if the cursor has moved outside the visible region, then scroll
	gtk_text_view_scroll_to_iter(text_view, &iter, 0.0, FALSE, 0.0, 0.0);

	return TRUE;
}

gboolean cursor_long_jump_right(GdkEventKey *key_event) {
	printf("cursor_long_jump_right()\n");

	GtkTextView *text_view;
	GtkTextBuffer *text_buffer;
	if (!init(&text_view, &text_buffer)) return FALSE;

	GtkTextIter iter;
	get_cursor_position(text_buffer, NULL, &iter, NULL);

	GtkTextIter cursor_pos = iter;

	gunichar ch = gtk_text_iter_get_char(&iter);

	if (g_unichar_isalnum(ch) || ch == '_')
	{
		// lowercase alpha
		do {
			if (!gtk_text_iter_forward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (g_unichar_isalnum(ch) || ch == '_');
	} else {
		do {
			if (!gtk_text_iter_forward_char(&iter)) break;
			ch = gtk_text_iter_get_char(&iter);
		} while (!(g_unichar_isalnum(ch) || ch == '_'));
	}
//	else if (g_unichar_ispunct(ch))
//	{
//		// punctuation
//		do {
//			if (!gtk_text_iter_forward_char(&iter)) break;
//			ch = gtk_text_iter_get_char(&iter);
//		} while (g_unichar_ispunct(ch));
//	}
//	else if (g_unichar_isspace(ch))
//	{
//		// whitespace
//		do {
//			if (!gtk_text_iter_forward_char(&iter)) break;
//			ch = gtk_text_iter_get_char(&iter);
//		} while (g_unichar_isspace(ch));
//	}
//	else
//	{
//		// everything else?
//		assert(false);
//	}

//	if (!gtk_text_iter_is_start(&iter))
//		gtk_text_iter_forward_char(&iter);

	if (key_event->state & GDK_SHIFT_MASK) {
		// Check if we already have a selection.
		// If so, then add to the existing selection
		GtkTextIter sel_start, sel_end, new_sel_bound;
		if (gtk_text_buffer_get_selection_bounds(text_buffer, &sel_start, &sel_end)) {
			if (gtk_text_iter_compare(&sel_start, &cursor_pos) == 0) {
				// sel_start is where the cursor is
				new_sel_bound = sel_end;
			} else {
				// sel_end is where the cursor is
				new_sel_bound = sel_start;
			}
		} else {
			new_sel_bound = cursor_pos;
		}
		// 1st insertion, 2nd selection-bound
		gtk_text_buffer_select_range(text_buffer, &iter, &new_sel_bound);
	} else {
		gtk_text_buffer_place_cursor(text_buffer, &iter);
	}

	// if the cursor has moved outside the visible region, then scroll
	gtk_text_view_scroll_to_iter(text_view, &iter, 0.0, FALSE, 0.0, 0.0);

	return TRUE;
}


gboolean move_cursor_up(GdkEventKey *key_event)
{
	LOG_MSG("move_cursor_up()\n");

	GtkTextView *view;
	GtkTextBuffer *text_buffer;

	gboolean rv = init(&view, &text_buffer);
	if (!rv)
		return rv;

	GtkTextIter iter, start, end;

	GtkTextMark *cursor = gtk_text_buffer_get_mark(text_buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, cursor);

	GtkTextIter cursor_pos = iter;

	while (gtk_text_iter_get_char(&iter) == '\n' && !gtk_text_iter_is_start(&iter))
		gtk_text_iter_backward_char(&iter);

	if (gtk_text_iter_backward_search(&iter, "\n\n", (GtkTextSearchFlags) 0, &start, &end, NULL)) {
		iter = end;
		gtk_text_iter_backward_char(&iter);
	} else {
		gtk_text_buffer_get_start_iter(text_buffer, &iter);
	}

	if (key_event->state & GDK_SHIFT_MASK) {
		// Check if we already have a selection.
		// If so, then add to the existing selection
		GtkTextIter sel_start, sel_end, new_sel_bound;
		if (gtk_text_buffer_get_selection_bounds(text_buffer, &sel_start, &sel_end)) {
			if (gtk_text_iter_compare(&sel_start, &cursor_pos) == 0) {
				// sel_start is where the cursor is
				new_sel_bound = sel_end;
			} else {
				// sel_end is where the cursor is
				new_sel_bound = sel_start;
			}
		} else {
			new_sel_bound = cursor_pos;
		}
		// 1st insertion, 2nd selection-bound
		gtk_text_buffer_select_range(text_buffer, &iter, &new_sel_bound);
	} else {
		gtk_text_buffer_place_cursor(text_buffer, &iter);
	}

	gtk_text_view_scroll_to_iter(view, &iter, 0.0, FALSE, 0.0, 0.0);

	return TRUE;
}


gboolean move_cursor_down(GdkEventKey *key_event)
{
	LOG_MSG("move_cursor_down()\n");

	GtkTextView *view;
	GtkTextBuffer *text_buffer;

	gboolean rv = init(&view, &text_buffer);
	if (!rv)
		return rv;

	GtkTextIter iter, start, end;

	GtkTextMark *cursor = gtk_text_buffer_get_mark(text_buffer, "insert");
	gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, cursor);

	GtkTextIter cursor_pos = iter;

	while (gtk_text_iter_get_char(&iter) == '\n' && !gtk_text_iter_is_end(&iter))
		gtk_text_iter_forward_char(&iter);

	if (gtk_text_iter_forward_search(&iter, "\n\n", (GtkTextSearchFlags) 0, &start, &end, NULL)) {
		iter = end;
		gtk_text_iter_backward_char(&iter);
	} else {
		gtk_text_buffer_get_end_iter(text_buffer, &iter);
	}

	if (key_event->state & GDK_SHIFT_MASK) {
		// Check if we already have a selection.
		// If so, then add to the existing selection
		GtkTextIter sel_start, sel_end, new_sel_bound;
		if (gtk_text_buffer_get_selection_bounds(text_buffer, &sel_start, &sel_end)) {
			if (gtk_text_iter_compare(&sel_start, &cursor_pos) == 0) {
				// sel_start is where the cursor is
				new_sel_bound = sel_end;
			} else {
				// sel_end is where the cursor is
				new_sel_bound = sel_start;
			}
		} else {
			new_sel_bound = cursor_pos;
		}
		// 1st insertion, 2nd selection-bound
		gtk_text_buffer_select_range(text_buffer, &iter, &new_sel_bound);
	} else {
		gtk_text_buffer_place_cursor(text_buffer, &iter);
	}

	gtk_text_view_scroll_to_iter(view, &iter, 0.0, FALSE, 0.0, 0.0);

	return TRUE;
}


// returns FALSE if iterator not on a token
gboolean get_token_bounds(const GtkTextIter *pi, GtkTextIter *pi_start, GtkTextIter *pi_end)
{
	GtkTextIter i = *pi;

	gunichar c = gtk_text_iter_get_char(&i);	
	if (c == ' ' || c == '\t' || c == '\n' || gtk_text_iter_is_end(&i)) return FALSE;

	if (!move_to_token_start(&i)) {
		return FALSE;
	}
	*pi_start = i;

	if (!move_to_token_end(&i)) {
		return FALSE;
	}
	*pi_end = i;

	return TRUE;
}


//// rather than swapping tokens, maybe try moving character at a time..
//gboolean move_token_left(GdkEventKey *key_event)
//{
//	GtkTextView *view;
//	GtkTextBuffer *buffer;
//
//	printf("move_words_left()\n");
//
//	view = GTK_TEXT_VIEW(
//		visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_VIEW));
//	if (view == NULL) {
//		printf("move_words_left(): no tabs open\n");
//		return FALSE;
//	}
//	if (!gtk_widget_is_focus(GTK_WIDGET(view))) {
//		printf("move_words_left(): text-view not in focus\n");
//		return FALSE;
//	}
//	buffer = gtk_text_view_get_buffer(view);
//
//	GtkTextIter i_cursor, i_token_start, i_token_end;
//
//	get_cursor_position(buffer, NULL, &i_cursor, NULL);
//
//	if (!get_token_bounds(&i_cursor, &i_token_start, &i_token_end)) {
//		printf("move_token_left(): cursor not on a token\n");
//		return TRUE;
//	}
//
//	i_cursor = i_token_start;
//
///*
//	if (!move_to_prev_token(&i_cursor)) {
//		printf("move_token_left(): already the first token\n");
//		return TRUE;
//	}
//*/
//	gtk_text_iter_backward_char(&i_cursor);
//
//	char *token = gtk_text_buffer_get_text(buffer, &i_token_start, &i_token_end, FALSE);
//	//printf("move_token_left(): token: %s\n", token);
//
//	GtkTextMark *m_temp = gtk_text_buffer_create_mark(buffer, NULL, &i_cursor, TRUE);
//	gtk_text_buffer_delete(buffer, &i_token_start, &i_token_end);
//	gtk_text_buffer_get_iter_at_mark(buffer, &i_cursor, m_temp);
//	gtk_text_buffer_insert(buffer, &i_cursor, token, -1);
//	gtk_text_buffer_get_iter_at_mark(buffer, &i_cursor, m_temp);
//	gtk_text_buffer_place_cursor(buffer, &i_cursor);
//	gtk_text_view_scroll_to_iter(view, &i_cursor, 0.0, FALSE, 0.0, 0.0);
//
//	gtk_text_buffer_delete_mark(buffer, m_temp);
//	return TRUE;
//}
//
//gboolean move_token_right(GdkEventKey *key_event)
//{
//	GtkTextView *view;
//	GtkTextBuffer *buffer;
//
//	printf("move_words_right()\n");
//
//	gboolean rv = init(&view, &buffer);
//	if (!rv)
//		return rv;
//
//	GtkTextIter i_cursor, i_token_start, i_token_end;
//
//	get_cursor_position(buffer, NULL, &i_cursor, NULL);
//
//	if (!get_token_bounds(&i_cursor, &i_token_start, &i_token_end)) {
//		printf("move_token_right(): cursor not on a token\n");
//		return TRUE;
//	}
//
//	i_cursor = i_token_end;
//
//	gtk_text_iter_forward_char(&i_cursor);
//
//	char *token = gtk_text_buffer_get_text(buffer, &i_token_start, &i_token_end, FALSE);
//	//printf("move_token_right(): token: %s\n", token);
//
//	GtkTextMark *m_temp = gtk_text_buffer_create_mark(buffer, NULL, &i_cursor, TRUE);
//	gtk_text_buffer_delete(buffer, &i_token_start, &i_token_end);
//	gtk_text_buffer_get_iter_at_mark(buffer, &i_cursor, m_temp);
//	gtk_text_buffer_insert(buffer, &i_cursor, token, -1);
//	gtk_text_buffer_get_iter_at_mark(buffer, &i_cursor, m_temp);
//	gtk_text_buffer_place_cursor(buffer, &i_cursor);
//	gtk_text_view_scroll_to_iter(view, &i_cursor, 0.0, FALSE, 0.0, 0.0);
//
//	gtk_text_buffer_delete_mark(buffer, m_temp);
//	return TRUE;
//}
