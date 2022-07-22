#include <ctype.h>
#include <string.h>

#include "declarations.h"

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

void c_highlight(GtkTextBuffer *text_buffer, GtkTextIter *a, GtkTextIter *b)
{
	printf("c_highlight()\n");

	// make sure we have all the tags we expect.
	//@ eventually we need proper error-handling here
	GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);
	GtkTextTag *comment_tag 			= gtk_text_tag_table_lookup(table, "comment");
	GtkTextTag *string_tag 				= gtk_text_tag_table_lookup(table, "string");
	GtkTextTag *preprocessor_tag 		= gtk_text_tag_table_lookup(table, "preprocessor");
	GtkTextTag *operator_tag 			= gtk_text_tag_table_lookup(table, "operator");
	GtkTextTag *function_tag 			= gtk_text_tag_table_lookup(table, "function");
	GtkTextTag *identifier_tag 		= gtk_text_tag_table_lookup(table, "identifier");
	GtkTextTag *number_tag 				= gtk_text_tag_table_lookup(table, "number");
	GtkTextTag *keyword_tag 			= gtk_text_tag_table_lookup(table, "keyword");
	GtkTextTag *type_tag 				= gtk_text_tag_table_lookup(table, "type");
	GtkTextTag *unknown_tag 			= gtk_text_tag_table_lookup(table, "unknown");
	assert(comment_tag); assert(string_tag); assert(preprocessor_tag); assert(operator_tag); assert(function_tag);
	assert(identifier_tag); assert(number_tag); assert(keyword_tag); assert(type_tag); assert(unknown_tag);

	GtkTextIter j1, j2;
	GtkTextIter *start, *end;
	j1 = *a;
	j2 = b ? *b : *a;
	start = &j1, end = &j2;
	//@ hacking our way around

	while (gtk_text_iter_backward_char(start)) {
		printf("moving backwards %c\n", gtk_text_iter_get_char(start));
		//@ what if ';' is inside a comment/string?
		if (gtk_text_iter_get_char(start) == ';') {
			if (gtk_text_iter_has_tag(start, comment_tag) || gtk_text_iter_has_tag(start, string_tag)) {
				continue; // ignore if inside a comment/string
			}
			break;
		}
	}

	while (gtk_text_iter_forward_char(end)) {
		printf("moving forward %c\n", gtk_text_iter_get_char(end));
		//@ what if ';' is inside a comment?
		if (gtk_text_iter_get_char(end) == ';') {
			if (gtk_text_iter_has_tag(end, comment_tag) || gtk_text_iter_has_tag(end, string_tag)) {
				continue; // ignore if inside a comment/string
			}
			break;
		}
	}
	TIME_START

	char *text = gtk_text_buffer_get_text(text_buffer, start, end, FALSE);
	printf("\"%s\"\n", text);
	free(text);

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
//			gtk_text_buffer_apply_tag_by_name(text_buffer, "comment", &begin, &iter);
			gtk_text_buffer_apply_tag(text_buffer, comment_tag, &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		if (c1 == '/' && c2 == '/') {
			GtkTextIter begin = iter;
			gtk_text_iter_forward_line(&iter);
			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
//			gtk_text_buffer_apply_tag_by_name(text_buffer, "comment", &begin, &iter);
			gtk_text_buffer_apply_tag(text_buffer, comment_tag, &begin, &iter);
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
//			gtk_text_buffer_apply_tag_by_name(text_buffer, "string-literal", &begin, &iter);
			gtk_text_buffer_apply_tag(text_buffer, string_tag, &begin, &iter);
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
//			gtk_text_buffer_apply_tag_by_name(text_buffer, "string-literal", &begin, &iter);
			gtk_text_buffer_apply_tag(text_buffer, string_tag, &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

//		if (c1 == '#') { // @ something wrong here?
//			GtkTextIter begin = iter;
//			while (gtk_text_iter_forward_line(&iter)) { // Is there a next line?
//				gtk_text_iter_backward_chars(&iter, 2);
//				if (gtk_text_iter_get_char(&iter) == 92) {
//					gtk_text_iter_forward_chars(&iter, 2);
//					continue;
//					//gtk_text_iter_forward_line(&iter);
//				} else {
//					gtk_text_iter_forward_chars(&iter, 2);
//					break;
//				}
//			}
//			
//			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
//			gtk_text_buffer_apply_tag_by_name(text_buffer, "preprocessor-directive", &begin, &iter);
//			gtk_text_iter_backward_char(&iter);
//			continue;
//		}

		//@ I dont think preprocessor directives can contain characters other than alphanumeric and underscores?
		if (c1 == '#') {
			GtkTextIter start_tag, end_tag;
			start_tag = iter;
			while (gtk_text_iter_forward_char(&iter)) {
				gunichar ch = gtk_text_iter_get_char(&iter);
				if (ch == '\n') {
					// newlines can be escaped by backslashes
					//@ really, backslashes can be escaped by backslashes, so we should count them?
					gtk_text_iter_backward_char(&iter);
					ch = gtk_text_iter_get_char(&iter);
					gtk_text_iter_forward_char(&iter);
					if (ch == '\\') {
						continue;
					}
					break;
				} else if (ch == '/') {
					gtk_text_iter_forward_char(&iter);
					ch = gtk_text_iter_get_char(&iter);
					if (ch == '/' || ch == '*') {
						// comment
						//@ theres a bug: the tag should end at the first '/', not before
						gtk_text_iter_backward_chars(&iter, 2);
						break;
					}
					gtk_text_iter_backward_char(&iter);
				}
			}
			end_tag = iter;

			gtk_text_buffer_remove_all_tags(text_buffer, &start_tag, &end_tag);
//			gtk_text_buffer_apply_tag_by_name(text_buffer, "preprocessor-directive", &start_tag, &end_tag);
			gtk_text_buffer_apply_tag(text_buffer, preprocessor_tag, &start_tag, &end_tag);
		}

		if (c1 == '+' || c1 == '-' || c1 == '*' || c1 == '/' || c1 == '='
				|| c1 == '(' || c1 == ')' || c1 == '[' || c1 == ']' || c1 == '{' || c1 == '}'
				|| c1 == '<' || c1 == '>' || c1 == '!'  || c1 == '|' || c1 == '~' || c1 == '&'
				|| c1 == ';' || c1 == ',' || c1 == '?' || c1 == ':' || c1 == '.' || c1 == '%') {
			GtkTextIter begin = iter;
			gtk_text_iter_forward_char(&iter);
//			gtk_text_buffer_apply_tag_by_name(text_buffer, "operator", &begin, &iter);
			gtk_text_buffer_apply_tag(text_buffer, operator_tag, &begin, &iter);
			gtk_text_iter_backward_char(&iter);

			// it could also be an arithmetic operator
			// we could look at the specific way its used:
			// if ident *ident then we could tend to suspect a pointer and highlight that way
			// all other cases like ident * ident or ident*ident or ident* ident etc. 
			// we would highlight as a multiplication operator 
			if (c1 == '*' && (g_unichar_isalpha(c2) || c2 == '_' || c2 == '*'))
				continue; // if immediately followed by an identifier well assume pointer

			if (possible_type_identifier[0]) {
				if (c1 == '(') {
//					const char *tag_name = "function";
//					GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_name);
//					if (tag) {
//						gtk_text_buffer_apply_tag(text_buffer, tag, &i1, &i2);
//					} else {
//						printf("*** error ***: text-tag doesnt exist: %s\n", tag_name);
//					}
//					gtk_text_buffer_apply_tag_by_name(text_buffer, "function", &i1, &i2);
					gtk_text_buffer_apply_tag(text_buffer, function_tag, &i1, &i2);
				} else {
//					gtk_text_buffer_apply_tag_by_name(text_buffer, "identifier", &i1, &i2);
					gtk_text_buffer_apply_tag(text_buffer, identifier_tag, &i1, &i2);
				}
				possible_type_identifier[0] = 0;
			} 
			continue;
		}

		if (g_unichar_isdigit(c1)) {
			GtkTextIter begin = iter;
			while(gtk_text_iter_forward_char(&iter) == TRUE) {
				c1 = gtk_text_iter_get_char(&iter);
				if(!g_unichar_isalnum(c1) && c1 != '.') break;
			}
//			gtk_text_buffer_apply_tag_by_name(text_buffer, "number-literal", &begin, &iter);
			gtk_text_buffer_apply_tag(text_buffer, number_tag, &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		//@ identifiers not followed by at least 1 operator?
		if (g_unichar_isalpha(c1) || c1 == '_') {
			GtkTextIter begin = iter;
			while (gtk_text_iter_forward_char(&iter)) {
				c1 = gtk_text_iter_get_char(&iter);
				if (!g_unichar_isalnum(c1) && c1 != '_') break;
			}

			// Maybe our identifier is a keyword?
//			const char *keywords[] = {
//				"if", "else", "return", "for", "while", "break", "continue", "struct", "const", "extern", "static", NULL};

			const char *keywords[] = {
				"auto", "break", "case", /*"char",*/
				"const", "continue", "default", "do",
				/*"double",*/ "else", "enum", "extern",
				/*"float",*/ "for", "goto", "if",
				/*"int",*/ "long", "register", "return",
				"short", "signed", "sizeof", "static",
				"struct", "switch", "typedef", "union",
				"unsigned", /*"void",*/ "volatile", "while", NULL
			};

			bool found = false;
			char *identifier = gtk_text_buffer_get_text(text_buffer, &begin, &iter, FALSE);
//			printf("*** identifier: %s\n", identifier);
			for (int i = 0; keywords[i] != NULL; ++i) {
				if (strcmp(identifier, keywords[i]) == 0) {
//					printf("*** %s is a keyword\n", identifier);
//					gtk_text_buffer_apply_tag_by_name(text_buffer, "keyword", &begin, &iter);
					gtk_text_buffer_apply_tag(text_buffer, keyword_tag, &begin, &iter);
					found = true;
					break;
				}
			}
			if (found) {
				gtk_text_iter_backward_char(&iter);
				free(identifier);
				continue;
			}
//			printf("? identifier: %s\n", identifier);

// identifier could also identify a type... lets see if we can recognize that...
// lets just assume for now, that if we have an identifier followed by an identifier,
// then the first identifier identifies a type.

			if (possible_type_identifier[0] != 0) {
				// if no1 has zeroed the previous identifier, we take it to be the type of the current identifier
//				gtk_text_buffer_apply_tag_by_name(text_buffer, "type", &i1, &i2);
				gtk_text_buffer_apply_tag(text_buffer, type_tag, &i1, &i2);
//				printf("*** %s is a type\n", possible_type_identifier);
				possible_type_identifier[0] = 0;
//				gtk_text_buffer_apply_tag_by_name(text_buffer, "identifier", &begin, &iter);
				gtk_text_buffer_apply_tag(text_buffer, identifier_tag, &begin, &iter);
//				printf("*** %s is an identifier\n", identifier);
			} else {
//				gtk_text_buffer_apply_tag_by_name(text_buffer, "identifier", &begin, &iter);
//				printf("********* applying tags to: %s\n", identifier);
				strncpy(possible_type_identifier, identifier, 100);
//				suspect_identifier = true;
				i1 = begin; i2 = iter;
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
//			gtk_text_buffer_apply_tag_by_name(text_buffer, "unknown", &begin, &iter);
			gtk_text_buffer_apply_tag(text_buffer, unknown_tag, &begin, &iter);
			gtk_text_iter_backward_char(&iter);
		}
	}

	TIME_END
	printf("it took %ld milliseconds\n", TIME_ELAPSED);
}