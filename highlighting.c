#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

//@ multiple tabs share these global variables... could this be a problem?
int global_location_offset; // location of last insertion or deletion
int global_length; // length of text inserted or 0 if deletion
char *global_text;


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
	printf("adding highlighting to range: %d -> %d\n", gtk_text_iter_get_offset(start), gtk_text_iter_get_offset(end));

	gtk_text_buffer_remove_all_tags(text_buffer, start, end); // @ shouldnt do that here at all?

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
				|| c1 == ';' || c1 == ',' || c1 == '?' || c1 == ':') {
			GtkTextIter begin = iter;
			gtk_text_iter_forward_char(&iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "operator", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
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
			char *identifier = gtk_text_buffer_get_text(text_buffer, &begin, &iter, FALSE);
			gboolean is_keyword = FALSE;
			char *keywords[] = {"if", "else", "return", "for", "while", "break", "continue", NULL};
			int k;
			for(k = 0; keywords[k] != NULL; ++k) {
				if(strcmp(identifier, keywords[k]) == 0) {
					is_keyword = TRUE;
					break;
				}
			}
			free(identifier);

			if (is_keyword) {
				gtk_text_buffer_apply_tag_by_name(text_buffer, "keyword", &begin, &iter);
			} else {
				gtk_text_buffer_apply_tag_by_name(text_buffer, "identifier", &begin, &iter);
			}

			gtk_text_iter_backward_char(&iter);
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
	printf("create_tags called\n");
	gtk_text_buffer_create_tag(text_buffer, "comment", "style", PANGO_STYLE_ITALIC, "foreground", "green", NULL);
	gtk_text_buffer_create_tag(text_buffer, "operator", "foreground", "red", NULL);
	gtk_text_buffer_create_tag(text_buffer, "number", "foreground", "blue", NULL);
	gtk_text_buffer_create_tag(text_buffer, "identifier", "foreground", "black", NULL);
	gtk_text_buffer_create_tag(text_buffer, "keyword", "weight", "bold", "foreground", "black", NULL);
	gtk_text_buffer_create_tag(text_buffer, "preprocessor-directive", "foreground", "purple", NULL);
	gtk_text_buffer_create_tag(text_buffer, "unknown", "foreground", "orange", NULL);
	gtk_text_buffer_create_tag(text_buffer, "string", "foreground", "gray", NULL);
}

/* 
	Maybe consider highlighting by line
	or maybe highlight 1 statement at a time (;)?
	Block-comments begin and end sequences potentially affect a whole file, but these seem to be an exception.
	Strings can span across multiple lines when backslashed. Preprocessor directives as well.
*/

void on_text_buffer_changed_for_highlighting(GtkTextBuffer *buffer, gpointer data)
{
	g_print("on_text_buffer_changed_for_highlighting() called!\n");

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

	GtkTextIter iter, start, end;

	TOKEN_RANGE:
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, global_location_offset);

	//g_print("offset: %d\n", offset);

	start = iter;
	gtk_text_iter_backward_char(&start);
	while (gtk_text_iter_backward_char(&start)) {
		if (gtk_text_iter_begins_tag(&start, NULL)) break;
	}

	end = iter;
	while (gtk_text_iter_forward_char(&end)) {
		//g_print("character: %c\n", gtk_text_iter_get_char(&end));
		if (gtk_text_iter_ends_tag(&end, NULL)) break;
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
	global_location_offset = gtk_text_iter_get_offset(start);
	global_text = gtk_text_buffer_get_text(buffer, start, end, FALSE);
	global_length = 0;
}

void init_highlighting(GtkTextBuffer *text_buffer)
{
	printf("init_highlighting() called...\n");
	create_tags(text_buffer);
	GtkTextIter start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
	highlight(text_buffer, &start_buffer, &end_buffer);

	g_signal_connect(text_buffer, "changed", G_CALLBACK(on_text_buffer_changed_for_highlighting), NULL);
	g_signal_connect(text_buffer, "insert-text", G_CALLBACK(on_text_buffer_insert_text_for_highlighting), NULL);
	g_signal_connect(text_buffer, "delete-range", G_CALLBACK(on_text_buffer_delete_range_for_highlighting), NULL);
}




