#include <string.h>
#include <ctype.h>
#include "declarations.h"

const char *rust_keywords[] = {
	"as",
	"use",
	"externcrate",
	"break",
	"const",
	"continue",
	"crate",
	"else",
	"if",
	"iflet",
	"enum",
	"extern",
	"false",
	"fn",
	"for",
	"if",
	"impl",
	"in",
	"for",
	"let",
	"loop",
	"match",
	"mod",
	"move",
	"mut",
	"pub",
	"impl",
	"ref",
	"return",
	"Self",
	"self",
	"static",
	"struct",
	"super",
	"trait",
	"true",
	"type",
	"unsafe",
	"use",
	"where",
	"while",
	"abstract",
	"alignof",
	"become",
	"box",
	"do",
	"final",
	"macro",
	"offsetof",
	"override",
	"priv",
	"proc",
	"pure",
	"sizeof",
	"typeof",
	"unsized",
	"virtual",
	"yield",
	NULL
};

void rust_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end)
{
//	LOG_MSG("rust_highlight()\n");
	printf("rust_highlight()\n");

//	char *text = gtk_text_buffer_get_text(text_buffer, start, end, FALSE);
//	printf("%s\n", text);
//	free(text);

	gtk_text_buffer_remove_all_tags(text_buffer, start, end);

	GtkTextIter i = *start;
	while (gtk_text_iter_compare(&i, end) == -1) {
		gunichar ch = gtk_text_iter_get_char(&i);
		if (ch == '/') {
			gtk_text_iter_forward_char(&i);
//			if (gtk_text_iter_is_end(&i)) break; //@ thats not entirely correct
			ch = gtk_text_iter_get_char(&i);
			if (ch == '/') {
				// line comment
				GtkTextIter start_comment = i;
				gtk_text_iter_backward_char(&start_comment);
				do {
					gtk_text_iter_forward_char(&i);
					ch = gtk_text_iter_get_char(&i);
				} while (ch != '\n' && !gtk_text_iter_is_end(&i));
				gtk_text_buffer_apply_tag_by_name(text_buffer, "comment", &start_comment, &i);
				continue;
			} else if (ch == '*') {
				// '/*' -- begin block-comment
				printf("begin block comment\n");
				GtkTextIter start, end;
//				gtk_text_buffer_get_end_iter(text_buffer, &end);
				start = i;
				gtk_text_iter_backward_char(&start);
				int nested = 0; // current nesting level
				while (true) {
					gtk_text_iter_forward_char(&i);

					if (gtk_text_iter_is_end(&i)) {
						end = i;
						break;
					}

					ch = gtk_text_iter_get_char(&i);
					if (ch == '/') {
						gtk_text_iter_forward_char(&i);
						if (gtk_text_iter_is_end(&i)) {
							end = i;
							break;
						}
						ch = gtk_text_iter_get_char(&i);
						if (ch == '*') {
							// '/*' -- begin nested block-comment
							nested += 1;
							continue;
						}
					} else if (ch == '*') {
						gtk_text_iter_forward_char(&i);
						if (gtk_text_iter_is_end(&i)) {
							end = i;
							break;
						}
						ch = gtk_text_iter_get_char(&i);
						if (ch == '/') {
							// '*/' -- end block-comment
							if (nested) nested -= 1;
							else {
								end = i;
								gtk_text_iter_forward_char(&end);
								break;
							}
						}
					}
				}
				char *comment_text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
				printf("%s\n---\n", comment_text);
				free(comment_text);
				printf("start: %d, end: %d\n", gtk_text_iter_get_offset(&start), gtk_text_iter_get_offset(&end));
				gtk_text_buffer_apply_tag_by_name(text_buffer, "comment", &start, &end);
			} else {
				// operator '/' ?
			}
		} else if (ch == '"') {
			// string literal
			GtkTextIter start_tag = i;
			while (gtk_text_iter_forward_char(&i)) {
				ch = gtk_text_iter_get_char(&i);
				if (ch == '"') {
//					gtk_text_iter_backward_char(&i);
//					ch = gtk_text_iter_get_char(&i)
//					gtk_text_iter_forward_char(&i);
//					if (ch == '\\')
					break;
				}
			}
			gtk_text_iter_forward_char(&i);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "string", &start_tag, &i);
			gtk_text_iter_backward_char(&i);
		} else if (isdigit(ch)) {
			GtkTextIter start, end;
			start = i;
			while (isdigit(ch)) {
				gtk_text_iter_forward_char(&i);
				ch = gtk_text_iter_get_char(&i);
			}
			end = i;
			gtk_text_buffer_apply_tag_by_name(text_buffer, "number", &start, &end);
		} else if (ch == '_' || isalpha(ch) || ch > 127) {
			// Rust supports unicode characters in identifier names.
			// "ch > 127" is not exactly correct, but it doesnt matter right now.
			GtkTextIter start, end;
			start = i;
			while (ch == '_' || isalnum(ch) || ch > 127) {
				gtk_text_iter_forward_char(&i);
				ch = gtk_text_iter_get_char(&i);
			}
			end = i;
			char *identifier = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
			printf("%s\n", identifier);

//			if (strcmp(identifier, "true") == 0 || strcmp(identifier, "false") == 0) {
//				gtk_text_buffer_apply_tag_by_name(text_buffer, "number", &start, &end);
//				goto CONTINUE;
//			}

			for (int i = 0; rust_keywords[i] != NULL; ++i) {
				if (strcmp(identifier, rust_keywords[i]) == 0) {
					gtk_text_buffer_apply_tag_by_name(text_buffer, "keyword", &start, &end);
				}
			}
			free(identifier);
		}

		CONTINUE:
		gtk_text_iter_forward_char(&i);
	}
}