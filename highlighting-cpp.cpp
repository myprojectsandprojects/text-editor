#include <string.h>
#include <ctype.h>
#include "declarations.h"

const char *cpp_keywords[] = {
	"asm",
	"else",
	"new",
	"this",
	"auto",
	"enum",
	"operator",
	"throw",
//	"bool",
	"explicit",
	"private",
	"true",
	"break",
	"export",
	"protected",
	"try",
	"case",
	"extern",
	"public",
	"typedef",
	"catch",
	"false",
	"register",
	"typeid",
//	"char",
//	"float",
	"reinterpret_cast",
	"typename",
	"class",
	"for",
	"return",
	"union",
	"const",
	"friend",
//	"short",
	"unsigned",
	"const_cast",
	"goto",
	"signed",
	"using",
	"continue",
	"if",
	"sizeof",
	"virtual",
	"default",
	"inline",
	"static",
	"void",
	"delete",
//	"int",
	"static_cast",
	"volatile",
	"do",
//	"long",
	"struct",
	"wchar_t",
//	"double",
	"mutable",
	"switch",
	"while",
	"dynamic_cast",
	"namespace",
	"template",
};

// Order is important here because we use these values as indices into the token_names array to get the name of the tag.
#define TOKEN_END 0
#define TOKEN_UNKNOWN 1
#define TOKEN_COMMENT 2
#define TOKEN_STRING_LITERAL 3
#define TOKEN_IDENTIFIER 4
#define TOKEN_KEYWORD 5
#define TOKEN_OPERATOR 6
#define TOKEN_NUMBER 7
#define TOKEN_PREPROCESSOR 8
#define TOKEN_CHAR_LITERAL 9

const char *token_names[] = {
	NULL,
	"unknown",
	"comment",
	"string",
	"identifier",
	"keyword",
	"operator",
	"number",
	"preprocessor",
	"char",
};

// slightly faster to store iterators directly
typedef struct {
	int type;
//	int start;
//	int end;
	GtkTextIter start;
	GtkTextIter end;
//	char *text;
} Token;

// initializes 'start' and 'end'
void get_range(GtkTextBuffer *text_buffer, GtkTextIter *location, GtkTextIter *start, GtkTextIter *end){
	printf("get_range()\n");

	GtkTextIter token_start, token_end;

	token_start = *location;
//	gtk_text_iter_backward_char(start);
	while (gtk_text_iter_backward_char(&token_start)) {
		if (gtk_text_iter_begins_tag(&token_start, NULL)) {
			break;
		}
	}

	token_end = *location;
	while (gtk_text_iter_forward_char(&token_end)) {
		if (gtk_text_iter_ends_tag(&token_end, NULL)) {
			break;
		}
	}

	GtkTextIter semicolon_start, semicolon_end;

	semicolon_start = *location;
//	gtk_text_iter_backward_char(start);
	while (gtk_text_iter_backward_char(&semicolon_start)) {
		if (gtk_text_iter_get_char(&semicolon_start) == ';') {
			break;
		}
	}

	semicolon_end = *location;
	while (gtk_text_iter_forward_char(&semicolon_end)) {
		if (gtk_text_iter_get_char(&semicolon_end) == ';') {
			break;
		}
	}

	*start = (gtk_text_iter_compare(&token_start, &semicolon_start) < 0) ? token_start : semicolon_start;
	*end = (gtk_text_iter_compare(&token_end, &semicolon_end) > 0) ? token_end : semicolon_end;
}

void cpp_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end){
	printf("cpp_highlight()\n");

//	print_tags(text_buffer);

//	TIME_START;

	// determine range
	// do at least one statement, but when tokens are larger, then be bounded by tokens.
	// editing a large block-comment becomes slow if the range is bounded by token bounds.

	GtkTextIter a, b;

	if(end == NULL)
		get_range(text_buffer, start, &a, &b);
	else
		a = *start, b = *end;

	char *text = gtk_text_buffer_get_text(text_buffer, &a, &b, FALSE);
	printf("range:\n---\n%s\n---\n", text);
	free(text);

//	gtk_text_buffer_remove_all_tags(text_buffer, &a, &b);
//	print_tags(text_buffer);

//	struct CList *tokens = new_list();
//	struct List<Token *> *tokens = list_create<Token *>();
//	struct List<Token> *tokens = list_create<Token>();
//	Array<Token *> tokens;
	Array<Token> tokens;
	array_init(&tokens);

	gunichar c;
	GtkTextIter i = a;

	while(gtk_text_iter_compare(&i, &b) < 0){
		for(c = gtk_text_iter_get_char(&i); isspace(c); gtk_text_iter_forward_char(&i), c = gtk_text_iter_get_char(&i));
//		printf("first non-whitespace index: %d\n", gtk_text_iter_get_offset(&i)); // offset jumps from 0 immediately to 2???
//		printf("first non-whitespace character: %lc\n", c);
//		break;

		if(gtk_text_iter_is_end(&i)) break; // is this necessary?

//		Token *token = (Token *) malloc(sizeof(Token));
		Token token;

		switch(c){
		case '/':
		{
			GtkTextIter start = i;
			gtk_text_iter_forward_char(&i);
			c = gtk_text_iter_get_char(&i);
			if(c == '/'){
				// LINE-COMMENT
				while(gtk_text_iter_forward_char(&i)){
					if(gtk_text_iter_get_char(&i) == '\n') break;
				};
				token.type = TOKEN_COMMENT;
				token.start = start;
				token.end = i;
				array_add(&tokens, token);
			}else if(c == '*'){
				// BLOCK-COMMENT
				while(gtk_text_iter_forward_char(&i)){
					if(gtk_text_iter_get_char(&i) == '*'){
						gtk_text_iter_forward_char(&i);
						if(gtk_text_iter_get_char(&i) == '/') break;
						gtk_text_iter_backward_char(&i);
					}
				}
				gtk_text_iter_forward_char(&i);
				token.type = TOKEN_COMMENT;
				token.start = start;
				token.end = i;
				array_add(&tokens, token);
			}else{
				// DIVISION-OPERATOR
				token.type = TOKEN_OPERATOR;
				token.start = start;
				token.end = i;
				array_add(&tokens, token);
			}
			break;
		}

		case '#':
		{
			GtkTextIter t = i;
			bool is_directive = true;
			const char *directive = "include";
			for(int j = 0; directive[j]; ++j){
				gtk_text_iter_forward_char(&t);
				gunichar tc = gtk_text_iter_get_char(&t);
				if(tc != directive[j]){
					is_directive = false;
					break;
				}
			}
			if(is_directive){
				// PREPROCCESSOR-DIRECTIVE
				GtkTextIter start, end;
				start = i;
				i = t;
				gtk_text_iter_forward_char(&i);
				end = i;

				token.type = TOKEN_PREPROCESSOR;
				token.start = start;
				token.end = end;

				array_add(&tokens, token);
			}else{
				token.type = TOKEN_UNKNOWN;
				token.start = i;
				gtk_text_iter_forward_char(&i);
				token.end = i;
	
				array_add(&tokens, token);
			}

			break;
		}
			
		case '+': case '-': case '*': case '%':
		case '&': case '|': case '^': case '~':
		case '(': case ')': case '{': case '}': case '[': case ']':
		case '=': case '<': case '>': case '!': case '?': case ':': case '.': case ',': case ';':
//			printf("operator: %lc\n", c);
			token.type = TOKEN_OPERATOR;
			token.start = i;
			gtk_text_iter_forward_char(&i);
			token.end = i;

			array_add(&tokens, token);
			break;
		case '"':
			// STRING-LITERAL
			GtkTextIter start, end;
			start = i;
			while(gtk_text_iter_forward_char(&i)){
				if(gtk_text_iter_get_char(&i) == '"') break; //@ need to check for backslash
			}
			gtk_text_iter_forward_char(&i);
			end = i;
			token.type = TOKEN_STRING_LITERAL;
			token.start = start;
			token.end = end;
			array_add(&tokens, token);
			break;

		// '[any character]'
		// escape sequances like '\n' '\t' etc. are fine
		// '' is not a valid char-literal
		// ''' is wrong
		// '\'' -- single quote
		// '\\' -- backslash
		case '\'':
		{
			bool valid = false;

			GtkTextIter t = i;
			gtk_text_iter_forward_char(&t);
			gunichar tc = gtk_text_iter_get_char(&t);
			if(tc == '\\'){
				// possible escape sequence
				gtk_text_iter_forward_chars(&t, 2);
				if(gtk_text_iter_get_char(&t) == '\'')
					valid = true;
			}else if(!(tc == '\'' || tc == '\\')){
				// any character, not a singequote or backslash
				gtk_text_iter_forward_char(&t);
				if(gtk_text_iter_get_char(&t) == '\'')
					valid = true;
			}

			if(valid){
				// CHAR-LITERAL
				GtkTextIter start, end;
				start = i;
				i = t;
				gtk_text_iter_forward_char(&i);
				end = i;

				token.type = TOKEN_CHAR_LITERAL;
				token.start = start;
				token.end = end;

				array_add(&tokens, token);
			}else{
				token.type = TOKEN_UNKNOWN;
				token.start = i;
				gtk_text_iter_forward_char(&i);
				token.end = i;
	
				array_add(&tokens, token);
			}
		}
			break;
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
		{
			// NUMBER-LITERAL
			GtkTextIter start, end;
			start = i;
			while(gtk_text_iter_forward_char(&i)){
				c = gtk_text_iter_get_char(&i);
				if(!(c == '.' || isalnum(c))) break;
			}
			end = i;
			token.type = TOKEN_NUMBER;
			token.start = start;
			token.end = end;

			array_add(&tokens, token);

			break;
		}
		default:
			if(isalpha(c) || c == '_'){
				GtkTextIter start, end;
				start = i;
				while(gtk_text_iter_forward_char(&i)){
					c = gtk_text_iter_get_char(&i);
					if(!(c == '_' || isalnum(c))) break;
				};
				end = i;

				//@ Maybe storing keywords in some other data structure (Trie or Hashtable?) would be more reasonable, because at fileloadtime we might crunch through a lot of characters?
				char *identifier = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
//				printf("identifier: %s\n", identifier);

				bool is_keyword = false;
				for(int i = 0; i < COUNT(cpp_keywords); ++i){
					if(strcmp(cpp_keywords[i], identifier) == 0){
						is_keyword = true;
						break;
					}
				}
				if(is_keyword){
					token.type = TOKEN_KEYWORD;
				}else{
					token.type = TOKEN_IDENTIFIER;
				}
				token.start = start;
				token.end = end;

				array_add(&tokens, token);

				free(identifier);
				break;
			}

			token.type = TOKEN_UNKNOWN;
			token.start = i;
			gtk_text_iter_forward_char(&i);
			token.end = i;

			array_add(&tokens, token);
		}
	}

	if(tokens.count > 0){
		Token *first = &(tokens.data[0]);
		Token *last = &(tokens.data[tokens.count-1]);
		text = gtk_text_buffer_get_text(text_buffer, &(first->start), &(last->end), FALSE);
		printf("range2:\n---\n%s\n---\n", text);
		free(text);
		gtk_text_buffer_remove_all_tags(text_buffer, &(first->start), &(last->end));
	}

	for(int i = 0; i < tokens.count; ++i){
		gtk_text_buffer_apply_tag_by_name(text_buffer, token_names[tokens.data[i].type], &(tokens.data[i].start), &(tokens.data[i].end));
	}

//	TIME_END;
//	printf("time elapsed: %ld\n", TIME_ELAPSED);

//	print_tags(text_buffer);
}