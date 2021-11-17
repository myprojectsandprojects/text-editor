#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "declarations.h"

//@ multiple tabs share these global variables... could this be a problem?
int global_location_offset; // location of last insertion or deletion
int global_length; // length of text inserted or 0 if deletion
char *global_text;


extern struct Settings settings;


gunichar peek_next_character(GtkTextIter *iter)
{
	gunichar c;
	if (gtk_text_iter_is_end(iter) == TRUE) { // ...So that we are not moving the iterator backwards if already at the end.
		c = gtk_text_iter_get_char(iter); // returns 0 if iter is not dereferenceable
	} else {
		gtk_text_iter_forward_char(iter);
		c = gtk_text_iter_get_char(iter); // returns 0 if iter is not dereferenceable
		gtk_text_iter_backward_char(iter);
	}
	return c;
}

gunichar get_next_character(GtkTextIter *iter)
{
	gtk_text_iter_forward_char(iter);
	gunichar c = gtk_text_iter_get_char(iter); // returns 0 if iter is not dereferenceable
	return c;
}

void highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end)
{
	//char *text = gtk_text_buffer_get_text(text_buffer, start, end, FALSE);
	//LOG_MSG("highlighting: \"%s\"\n", text);
	//free(text);

	//printf("adding highlighting to range: %d -> %d\n", gtk_text_iter_get_offset(start), gtk_text_iter_get_offset(end));

	gtk_text_buffer_remove_all_tags(text_buffer, start, end); // @ shouldnt do that here at all?

	char possible_type_identifier[100];
	possible_type_identifier[0] = 0;
	GtkTextIter i1, i2;

	GtkTextIter iter;
	for (iter = *start;
			gtk_text_iter_compare(&iter, end) != 1 && gtk_text_iter_is_end(&iter) != TRUE;
			gtk_text_iter_forward_char(&iter)) {

		gunichar c1 = gtk_text_iter_get_char(&iter);
		gunichar c2 = peek_next_character(&iter);
		//g_print("%c ", c);

		if (c1 == '/' && c2 == '*') {
			GtkTextIter begin = iter;
			c1 = get_next_character(&iter);
			while (c1 != 0) {
				c1 = get_next_character(&iter);
				c2 = peek_next_character(&iter);
				if (c1 == '*' && c2 == '/') break;
			}
			if (c1 != 0) {
				// comment /* -> */
				gtk_text_iter_forward_chars(&iter, 2);
			}
			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "comment", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		if (c1 == '/' && c2 == '/') {
			GtkTextIter begin = iter;
			gtk_text_iter_forward_line(&iter);
			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "comment", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		//@ range is still not correct for strings...
		if (c1 == '\"') { //@ string can be continued on a new line using backslash, but whatever for now. 
			GtkTextIter begin = iter;
			do {
				c1 = get_next_character(&iter);
				if (c1 == '\"') {
					gtk_text_iter_backward_char(&iter);
					gunichar prev = gtk_text_iter_get_char(&iter);
					if (prev != 92) { // backslash
						gtk_text_iter_forward_char(&iter);
						break;
					}
					gtk_text_iter_forward_char(&iter);
				}
			} while (c1 != '\n' && c1 != 0);
			
			gtk_text_iter_forward_char(&iter);
			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "string", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		if (c1 == '\'') {
			GtkTextIter begin = iter;
			do {
				c1 = get_next_character(&iter);
				if (c1 == '\'') {
					gtk_text_iter_backward_char(&iter);
					gunichar prev = gtk_text_iter_get_char(&iter);
					if (prev != 92) { // backslash
						gtk_text_iter_forward_char(&iter);
						break;
					}
					gtk_text_iter_forward_char(&iter);
				}
			} while (c1 != '\n' && c1 != 0);
			
			gtk_text_iter_forward_char(&iter);
			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "string", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		if (c1 == '#') { // @ something wrong here?
			GtkTextIter begin = iter;
			while (gtk_text_iter_forward_line(&iter)) { // Is there a next line?
				gtk_text_iter_backward_chars(&iter, 2);
				if (gtk_text_iter_get_char(&iter) == 92) {
					gtk_text_iter_forward_chars(&iter, 2);
					continue;
					//gtk_text_iter_forward_line(&iter);
				} else {
					gtk_text_iter_forward_chars(&iter, 2);
					break;
				}
			}
			
			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "preprocessor-directive", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		if (c1 == '+' || c1 == '-' || c1 == '*' || c1 == '/' || c1 == '='
				|| c1 == '(' || c1 == ')' || c1 == '[' || c1 == ']' || c1 == '{' || c1 == '}'
				|| c1 == '<' || c1 == '>' || c1 == '!'  || c1 == '|' || c1 == '~' || c1 == '&'
				|| c1 == ';' || c1 == ',' || c1 == '?' || c1 == ':' || c1 == '.') {
			GtkTextIter begin = iter;
			gtk_text_iter_forward_char(&iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "operator", &begin, &iter);
			gtk_text_iter_backward_char(&iter);

			// it could also be an arithmetic operator
			// we could look at the specific way its used:
			// if ident *ident then we could tend to suspect a pointer and highlight that way
			// all other cases like ident * ident or ident*ident or ident* ident etc. 
			// we would highlight as a multiplication operator 
			if (c1 == '*' && (g_unichar_isalpha(c2) || c2 == '_' || c2 == '*'))
				continue; // if immediately followed by an identifier well assume pointer 

			possible_type_identifier[0] = 0;
			continue;
		}

		if (g_unichar_isdigit(c1)) {
			GtkTextIter begin = iter;
			while(gtk_text_iter_forward_char(&iter) == TRUE) {
				c1 = gtk_text_iter_get_char(&iter);
				if(!g_unichar_isalnum(c1) && c1 != '.') break;
			}
			gtk_text_buffer_apply_tag_by_name(text_buffer, "number", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		if(g_unichar_isalpha(c1) || c1 == '_') {
			GtkTextIter begin = iter;
			while(gtk_text_iter_forward_char(&iter)) {
				c1 = gtk_text_iter_get_char(&iter);
				if(!g_unichar_isalnum(c1) && c1 != '_') break;
			}

			// Maybe our identifier is a keyword?
			gboolean is_keyword = FALSE;
			char *keywords[] = {
				"if", "else", "return", "for", "while", "break", "continue", "struct", "const", "extern", "static", NULL};

			char *identifier = gtk_text_buffer_get_text(text_buffer, &begin, &iter, FALSE);
			int k;
			for(k = 0; keywords[k] != NULL; ++k) {
				if(strcmp(identifier, keywords[k]) == 0) {
					is_keyword = TRUE;
					break;
				}
			}

// identifier could also identify a type... lets see if we can recognize that...
// lets just assume for now, that if we have an identifier followed by an identifier,
// then the first identifier identifies a type.

			if (is_keyword == TRUE) {
				gtk_text_buffer_apply_tag_by_name(text_buffer, "keyword", &begin, &iter);
			} else {
				if (possible_type_identifier[0] != 0) {
					gtk_text_buffer_apply_tag_by_name(text_buffer, "type", &i1, &i2);
					possible_type_identifier[0] = 0;
					gtk_text_buffer_apply_tag_by_name(text_buffer, "identifier", &begin, &iter);
				} else {
					gtk_text_buffer_apply_tag_by_name(text_buffer, "identifier", &begin, &iter);
					strncpy(possible_type_identifier, identifier, 100);
					i1 = begin; i2 = iter;
				}
			}

			gtk_text_iter_backward_char(&iter);
			free(identifier);
			continue;
		}

		if (c1 != ' ' && c1 != '\t' && c1 != '\n') {
			GtkTextIter begin = iter;
			/*do {
				c1 = get_next_character(&iter);
			} while (c1 != ' ' && c1 != '\t' && c1 != '\n' && c1 != 0);*/
			
			gtk_text_iter_forward_char(&iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "unknown", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
		}
	}
}

void create_tags(GtkTextBuffer *text_buffer)
{
	LOG_MSG("create_tags()\n");

	// check if we already have the tags to avoid gtk warnings..
	GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);
	int size = gtk_text_tag_table_get_size(table);
	if (size != 0) {
		//LOG_MSG("create_tags(): tags already created. no need to create them.\n");
		printf("create_tags(): tags already created. no need to create them.\n");
		return;
	}

	//LOG_MSG("create_tags(): creating tags.\n");
	printf("create_tags(): creating tags.\n");

	gtk_text_buffer_create_tag(text_buffer, "identifier", "foreground", settings.identifier_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "keyword", "foreground", settings.keyword_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "type", "foreground", settings.type_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "operator", "foreground", settings.operator_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "number", "foreground", settings.number_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "comment", "foreground", settings.comment_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "preprocessor-directive", "foreground", settings.preproccessor_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "unknown", "foreground", settings.unknown_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "string", "foreground", settings.string_color, NULL);
}

/* 
	Maybe consider highlighting by line
	or maybe highlight 1 statement at a time (;)?
	Block-comments begin and end sequences potentially affect a whole file, but these seem to be an exception.
	Strings can span across multiple lines when backslashed. Preprocessor directives as well.
*/

void on_text_buffer_changed_for_highlighting(GtkTextBuffer *buffer, gpointer data)
{
	LOG_MSG("on_text_buffer_changed_for_highlighting()\n");

	/*GtkTextIter abs_start, abs_end;
	gtk_text_buffer_get_bounds(buffer, &abs_start, &abs_end);
	highlight(buffer, &abs_start, &abs_end);
	return;*/

	GtkTextIter iter, start, end;

	if (strchr(global_text, '\"') || strchr(global_text, '\\')) { // backslash could escape a doublequote, hence
		GtkTextIter text_start, text_end;
		gtk_text_buffer_get_iter_at_offset(buffer, &text_start, global_location_offset);
		gtk_text_buffer_get_iter_at_offset(buffer, &text_end, global_location_offset + global_length);
g_print("before\n");
		if (gtk_text_iter_get_char(&text_start) == '\n') goto TOKEN_RANGE;
		gtk_text_iter_forward_char(&text_start);
		if (gtk_text_iter_get_char(&text_start) == '\n') goto TOKEN_RANGE;
		gtk_text_iter_backward_char(&text_start);
g_print("after\n");
		for (; !gtk_text_iter_is_start(&text_start); gtk_text_iter_backward_char(&text_start)) {
			if (gtk_text_iter_get_char(&text_start) == '\n') break;
		}
		for (; !gtk_text_iter_is_end(&text_end); gtk_text_iter_forward_char(&text_end)) {
			if (gtk_text_iter_get_char(&text_end) == '\n') break;
		}
		highlight(buffer, &text_start, &text_end);
		return;
	}

TOKEN_RANGE:
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, global_location_offset);

	//g_print("offset: %d\n", offset);

	int count = 0;
	start = iter;
	gtk_text_iter_backward_char(&start);
	while (gtk_text_iter_backward_char(&start)) {
		if (gtk_text_iter_begins_tag(&start, NULL)) {
			if (count > 0) break;
			count += 1;
		}
	}

	count = 0;
	end = iter;
	while (gtk_text_iter_forward_char(&end)) {
		if (gtk_text_iter_ends_tag(&end, NULL)) {
			if (count > 1) break; // weve seen tag end more than once!
			count += 1;
		}
	}

	/*GtkTextIter abs_start, abs_end;
	gtk_text_buffer_get_bounds(buffer, &abs_start, &abs_end);
	print_tags(buffer, &abs_start, &abs_end);*/

	highlight(buffer, &start, &end);

	//print_tags(buffer, &abs_start, &abs_end);

	/*GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	add_highlighting(buffer, &start, &end);*/
}

void on_text_buffer_insert_text_for_highlighting(GtkTextBuffer *buffer, GtkTextIter *location, char *text, int length, gpointer data)
{
	LOG_MSG("on_text_buffer_insert_text_for_highlighting() called!\n");
	/*GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	const char *contents = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);*/
	//g_print("contents: %s\n", contents);

	global_location_offset = gtk_text_iter_get_offset(location);
	global_length = length;
	global_text = text;
}

void on_text_buffer_delete_range_for_highlighting(GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end)
{
	LOG_MSG("on_text_buffer_delete_range_for_highlighting()\n");
	global_location_offset = gtk_text_iter_get_offset(start);
	global_text = gtk_text_buffer_get_text(buffer, start, end, FALSE);
	global_length = 0;
}

void init_highlighting(GtkTextBuffer *text_buffer)
{
	LOG_MSG("init_highlighting()\n");
	create_tags(text_buffer);
	GtkTextIter start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
	highlight(text_buffer, &start_buffer, &end_buffer);


	unsigned long ids;

	ids = g_signal_connect(text_buffer, "changed", G_CALLBACK(on_text_buffer_changed_for_highlighting), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "changed-handler-4-highlighting", (void *) ids);

	ids = g_signal_connect(text_buffer, "insert-text", G_CALLBACK(on_text_buffer_insert_text_for_highlighting), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "insert-text-handler-4-highlighting", (void *) ids);

	ids = g_signal_connect(text_buffer, "delete-range", G_CALLBACK(on_text_buffer_delete_range_for_highlighting), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "delete-range-handler-4-highlighting", (void *) ids);
}

void remove_highlighting(GtkTextBuffer *text_buffer)
{
	LOG_MSG("remove_highlighting()\n");
	GtkTextIter range_start, range_end;
	gtk_text_buffer_get_bounds(text_buffer, &range_start, &range_end);
	gtk_text_buffer_remove_all_tags(text_buffer, &range_start, &range_end);


	// remove signal handlers
	unsigned long ids;

	ids = (unsigned long) g_object_get_data(G_OBJECT(text_buffer), "changed-handler-4-highlighting");
	g_signal_handler_disconnect(text_buffer, ids);

	ids = (unsigned long) g_object_get_data(G_OBJECT(text_buffer), "insert-text-handler-4-highlighting");
	g_signal_handler_disconnect(text_buffer, ids);

	ids = (unsigned long) g_object_get_data(G_OBJECT(text_buffer), "delete-range-handler-4-highlighting");
	g_signal_handler_disconnect(text_buffer, ids);
}





