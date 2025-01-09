#include <string.h>
#include <ctype.h>
#include "declarations.h"
#include "lib/lib.hpp"

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
//	"true",
	"break",
	"export",
	"protected",
	"try",
	"case",
	"extern",
	"public",
	"typedef",
	"catch",
//	"false",
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
//	"void",
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

Lib::HashTable keywords_table;
bool keywords_table_initialized = false;

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
#define TOKEN_FUNCTION 10
#define TOKEN_TYPE 11
#define TOKEN_POINTER_STAR 12

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
	"function",
	"type",
	"operator", // TOKEN_POINTER_STAR should be highlighted like any old operator
};

// slightly faster to store iterators directly
struct Token {
	int type;
//	int start;
//	int end;
	GtkTextIter start;
	GtkTextIter end;
//	char *text;
};

void create_token(Token *token, int type, GtkTextIter *start, GtkTextIter *end){
	token->type = type;
	token->start = *start;
	token->end = *end;
}

// Creates 1 character token
void create_token(Token *token, int type, GtkTextIter *start){
	GtkTextIter i = *start;
	gtk_text_iter_forward_char(&i);
	token->type = type;
	token->start = *start;
	token->end = i;
}

//// initializes 'start' and 'end'
//void get_range(GtkTextBuffer *text_buffer, GtkTextIter *location, GtkTextIter *start, GtkTextIter *end){
//	printf("get_range()\n");
//
//	GtkTextIter token_start, token_end;
//
//	token_start = *location;
////	gtk_text_iter_backward_char(start);
//	while (gtk_text_iter_backward_char(&token_start)) {
//		if (gtk_text_iter_begins_tag(&token_start, NULL)) {
//			break;
//		}
//	}
//
//	token_end = *location;
//	while (gtk_text_iter_forward_char(&token_end)) {
//		if (gtk_text_iter_ends_tag(&token_end, NULL)) {
//			break;
//		}
//	}
//
//	GtkTextIter semicolon_start, semicolon_end;
//
//	semicolon_start = *location;
////	gtk_text_iter_backward_char(start);
//	while (gtk_text_iter_backward_char(&semicolon_start)) {
//		if (gtk_text_iter_get_char(&semicolon_start) == ';') {
//			break;
//		}
//	}
//
//	semicolon_end = *location;
//	while (gtk_text_iter_forward_char(&semicolon_end)) {
//		if (gtk_text_iter_get_char(&semicolon_end) == ';') {
//			break;
//		}
//	}
//
//	*start = (gtk_text_iter_compare(&token_start, &semicolon_start) < 0) ? token_start : semicolon_start;
//	*end = (gtk_text_iter_compare(&token_end, &semicolon_end) > 0) ? token_end : semicolon_end;
//}

//bool is_inside_comment_or_string_or_charliteral(GtkTextIter *iter)
//{
//	bool result = false;
//	GSList *tags = gtk_text_iter_get_tags(iter);
//	for (GSList *ptr = tags; ptr; ptr = ptr->next) {
//		char *name;
//		GtkTextTag * tag = (GtkTextTag *) ptr->data;
//		g_object_get(tag, "name", &name, NULL);
//		if (strcmp("comment", name) == 0 || strcmp("string", name) == 0 || strcmp("char", name) == 0) {
//			result = true;
//			break;
//		}
//	}
////@	free(tags);
//	return result;
//}

bool is_end_of_statement(GtkTextIter *iter) {
	bool is_end_of_statement = false;
	GSList *tags = gtk_text_iter_get_tags(iter);
	for (GSList *ptr = tags; ptr; ptr = ptr->next) {
		char *name;
		GtkTextTag * tag = (GtkTextTag *) ptr->data;
		g_object_get(tag, "name", &name, NULL);
		if (strcmp("operator", name) == 0) {
			is_end_of_statement = true;
			break;
		}
	}
//@	free(tags);
	return is_end_of_statement;
}

void get_next_token(Token *token, GtkTextBuffer *text_buffer, GtkTextIter *iter) {
	gunichar c = gtk_text_iter_get_char(iter);
	switch(c) {
		case '/':
		{
			GtkTextIter start = *iter;
			gtk_text_iter_forward_char(iter);
			c = gtk_text_iter_get_char(iter);
			if(c == '/'){
				// LINE-COMMENT
				while(gtk_text_iter_forward_char(iter)){
					if(gtk_text_iter_get_char(iter) == '\n') break;
				};
				create_token(token, TOKEN_COMMENT, &start, iter);
			}else if(c == '*'){
				// BLOCK-COMMENT
				while(gtk_text_iter_forward_char(iter)){
					if(gtk_text_iter_get_char(iter) == '*'){
						gtk_text_iter_forward_char(iter);
						if(gtk_text_iter_get_char(iter) == '/') break;
						gtk_text_iter_backward_char(iter);
					}
				}
				gtk_text_iter_forward_char(iter);
				create_token(token, TOKEN_COMMENT, &start, iter);
			}else{
				// DIVISION-OPERATOR
				create_token(token, TOKEN_OPERATOR, &start, iter);
			}
			break;
		}

		case '#':
		{
			GtkTextIter t = *iter;
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
				start = *iter;
				*iter = t;
				gtk_text_iter_forward_char(iter);
				end = *iter;

				create_token(token, TOKEN_PREPROCESSOR, &start, &end);
			}else{
				create_token(token, TOKEN_UNKNOWN, iter);
				gtk_text_iter_forward_char(iter);
			}

			break;
		}
			
		case '+': case '-': case '*': case '%':
		case '&': case '|': case '^': case '~':
		case '(': case ')': case '{': case '}': case '[': case ']':
		case '=': case '<': case '>': case '!': case '?': case ':': case '.': case ',': case ';':
			//@ what about char **argv for example?
			if (c == '*') {
				GtkTextIter j, k;
				j = *iter; k = *iter;
				gtk_text_iter_forward_char(&j);
				gtk_text_iter_backward_char(&k);
				if (!isspace(gtk_text_iter_get_char(&j)) && isspace(gtk_text_iter_get_char(&k))) {
					create_token(token, TOKEN_POINTER_STAR, iter);
					gtk_text_iter_forward_char(iter);
					break;
				}
			}
			create_token(token, TOKEN_OPERATOR, iter);
			gtk_text_iter_forward_char(iter);
			break;

		case '"':
			// STRING-LITERAL
			GtkTextIter start, end;
			start = *iter;
			while(gtk_text_iter_forward_char(iter)){
				gunichar tc = gtk_text_iter_get_char(iter);
				if(tc == '\\'){
					// ignore char's preceded by a backslash
					gtk_text_iter_forward_char(iter);
					continue;
				}
				if(tc == '"') break;
			}
			gtk_text_iter_forward_char(iter);
			end = *iter;
			create_token(token, TOKEN_STRING_LITERAL, &start, &end);
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

			GtkTextIter t = *iter;
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
				start = *iter;
				*iter = t;
				gtk_text_iter_forward_char(iter);
				end = *iter;

				create_token(token, TOKEN_CHAR_LITERAL, &start, &end);
			}else{
				create_token(token, TOKEN_UNKNOWN, iter);
				gtk_text_iter_forward_char(iter);
			}
		}
		break;

		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
		{
			// NUMBER-LITERAL @is this a valid float-literal: ".123"?
			GtkTextIter start, end;
			start = *iter;
			while(gtk_text_iter_forward_char(iter)){
				c = gtk_text_iter_get_char(iter);
				if(!(c == '.' || isalnum(c))) break;
			}
			end = *iter;
			create_token(token, TOKEN_NUMBER, &start, &end);

			break;
		}

		default:
			if (isalpha(c) || c == '_') {
				GtkTextIter start, end;
				start = *iter;
				while(gtk_text_iter_forward_char(iter)){
					c = gtk_text_iter_get_char(iter);
					if(!(c == '_' || isalnum(c))) break;
				};
				end = *iter;

				//@ Maybe storing keywords in some other data structure (Trie or Hashtable?) would be more reasonable, because at fileloadtime we might crunch through a lot of characters?
				char *identifier = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
//				printf("identifier: %s\n", identifier);

				token->type = TOKEN_IDENTIFIER;
//				for(int i = 0; i < COUNT(cpp_keywords); ++i){
//					if (strcmp(cpp_keywords[i], identifier) == 0){
//						token->type = TOKEN_KEYWORD;
//						break;
//					}
//				}
				// it is a bit faster to use a hash-table lookup, but in the greater scheme of things the difference is insignificant
				if (Lib::hash_table_has(&keywords_table, identifier)) {
					token->type = TOKEN_KEYWORD;
				} else if (strcmp(identifier, "true") == 0|| strcmp(identifier, "false") == 0) {
					token->type = TOKEN_NUMBER; // Let boolean-values receive the same highlighting numbers do
				}
				token->start = start;
				token->end = end;

				free(identifier);
				break;
			}

//			token->type = TOKEN_UNKNOWN;
//			token->start = i;
//			gtk_text_iter_forward_char(iter);
//			token->end = i;
			create_token(token, TOKEN_UNKNOWN, iter);
			gtk_text_iter_forward_char(iter);
		}
}

/*
	There are two circumstances under which this function is called: 1) when an initial highlighting needs to be done and 2) when an already existing highlighting needs to be updated because the contents of the text-buffer were modified. In the second case the location-argument gives us the location where the change occured. Otherwise we expect it to be NULL.

	It would be much simpler if we could update the highlighting for the whole buffer every time we detect a change in the text-buffer, but this would significantly decrease performance to the point where the editor becomes unusable given sufficient amount of text. So we have to restrict ourselves to update a region whose size doesnt depend on the amount of text in the text-buffer and which is also small enough to provide a pleasant enough experience for the user.

	To recognize things like whether an identifier refers to a function or a type we need to look at tokens around them (for example: we assume an identifier refers to a function if its followed by open-parenthesis). This means that when we are updating a region in the text-buffer we cant just stop at an arbitrary location -- we need to carefully pick a location in such a way that no tokens relevant to us are excluded from the region. So our current solution is to always update from semicolon to semicolon, so that we never "break up" a statement.

	What we do is we "look back" from the location of the change for a semicolon that is not part of a comment, a string or a character-literal to determine where to start parsing the text. We also "look forward" to determine the location from where we could start to consider stopping. Between these two locations is a region we will parse no matter what. The actual region being parsed will depend on the circumstances. Once we have moved past the end of that region and the tokens we are generating begin to match up with pairs of text-tags in the text-buffer, we move on until we find a semicolon and then we stop.
*/

/*
This function is called when either one of the following conditions occurs: 1) contents of the text buffer need to be highlighted for the first time or 2) the contents of the text buffer were modified, so the highlighting needs to be updated. If the contents of the text buffer were modified, then we expect "location" argument to tell us the location of the change. Otherwise we expect "location" argument to be NULL.

We cant parse through the whole text buffer every time contents of the text buffer are modified, because the editor would become untolerably slow if the file was large enough. So we need to pick a smaller region around the location of the change.

Whether an identifier was meant to identify a variable or a type or a function is not possible to determine by "looking at" the identifier alone -- tokens before and after the identifier need to be taken into consideration. So to not "miss out" any tokens important to us, we never begin or end a region we choose to update in the middle of the statement. (Statements in C and C++ end with a ';', so the region we update will always begin and end with ';')

The region we will always parse will begin at the first ';' moving backwards from the location of the change (the ';' must also be legit, which means, for example, in case text was inserted, that it didnt come along with the inserted text etc.) and end at the location where the change took place. How far past the end of the before mentioned region we actually end up parsing depends on the circumstances. Once the tokens we find by parsing begin to match-up with whats already in the text buffer we parse until the first ';' and then stop.
*/

void cpp_highlight(GtkTextBuffer *text_buffer, GtkTextIter *location) {
	LOG_MSG("cpp_highlight()\n");

	if (!keywords_table_initialized)
	{
		Lib::hash_table_init(&keywords_table);
		for (int i = 0; i < COUNT(cpp_keywords); ++i)
		{
			Lib::hash_table_store(&keywords_table, cpp_keywords[i]);
		}
		keywords_table_initialized = true;
	}

	GtkTextIter region_start, region_end;
	if (!location) {
		// We are doing the initial highlighting
		gtk_text_buffer_get_start_iter(text_buffer, &region_start);
		gtk_text_buffer_get_end_iter(text_buffer, &region_end);
	} else {
		// We are updating already existing highlighting
		region_start = *location;
		while (gtk_text_iter_backward_char(&region_start)) {
			if (gtk_text_iter_get_char(&region_start) == ';' && is_end_of_statement(&region_start)) break;
		}
		region_end = *location;
//		while (gtk_text_iter_forward_char(&region_end)) {
//			if (gtk_text_iter_get_char(&region_end) == ';' && is_end_of_statement(&region_end)) break;
//		}
	}
	GtkTextIter i = region_start;

//	char *region_text = gtk_text_buffer_get_text(text_buffer, &region_start, &region_end, FALSE);
//	printf("region: %s\n", region_text);
//	free(region_text);

//	Array<Token *> tokens;
	array<Token> tokens;
	ArrayInit(&tokens);

	bool seen_matching_token = false; // Once the tokens we are generating begin matching up with the text-tags in the text buffer we want to stop parsing at the first semicolon we find. Because GTK merges consecutive one-character same-tagged-regions into one bigger region then in some cases (for example: ');') we wouldnt recognize that for all practical purposes a semicolon actually "matches up" with whats in the text buffer. So we can only stop if we have PREVIOUSLY seen tokens that "match up" with the tags in the text-buffer.

//	long before, after;
//	before = get_time_us();
	while (!gtk_text_iter_is_end(&i)) {
		for
		(
			gunichar c = gtk_text_iter_get_char(&i);
			isspace(c);
			gtk_text_iter_forward_char(&i), c = gtk_text_iter_get_char(&i)
		);
//		printf("first non-whitespace index: %d\n", gtk_text_iter_get_offset(&i)); // offset jumps from 0 immediately to 2???
//		printf("first non-whitespace character: %lc\n", c);
//		break;

		if(gtk_text_iter_is_end(&i)) break; // is this necessary?

//		Token *token = (Token *) malloc(sizeof(Token));
		Token token;
		get_next_token(&token, text_buffer, &i);

		if (location) {
			char *token_text = gtk_text_buffer_get_text(text_buffer, &token.start, &token.end, FALSE);
//			printf("token: %s", token_text);
			if (gtk_text_iter_compare(&token.end, &region_end) > 0) {
//				printf(" -past region end-");
				GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);
				if (token.type == TOKEN_IDENTIFIER) {
					// identifiers could become types and functions later on... so yeah
					const char *tag_names[] = { "identifier", "function", "type" };
					for (int i = 0; i < COUNT(tag_names); ++i) {
						GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_names[i]);
						GtkTextIter end, start;
						start = token.start;
						end = token.end;
						gtk_text_iter_backward_to_tag_toggle(&end, tag);
						gtk_text_iter_forward_to_tag_toggle(&start, tag);
						if (gtk_text_iter_compare(&start, &token.end) == 0 && gtk_text_iter_compare(&end, &token.start) == 0) {
//							printf(" -matches up-");
							seen_matching_token = true;
						}
					}
				} else {
					const char *tag_name = token_names[token.type];
					GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_name);
					GtkTextIter end, start;
					start = token.start;
					end = token.end;
					gtk_text_iter_backward_to_tag_toggle(&end, tag);
					gtk_text_iter_forward_to_tag_toggle(&start, tag);
					if (gtk_text_iter_compare(&start, &token.end) == 0 && gtk_text_iter_compare(&end, &token.start) == 0) {
//						printf(" -matches up-");
						seen_matching_token = true;
					}
				}
				
				if (seen_matching_token && token.type == TOKEN_OPERATOR)
				{
					gunichar c = gtk_text_iter_get_char(&token.start);
					if (c == ';') {
//						printf("\n");
						break;
					}
				}
			}
			free(token_text);
//			printf("\n");
		}

		ArrayAdd(&tokens, token);
	}
//	after = get_time_us();
//	printf("ELAPSED ON GETTING TOKENS: %ld\n", after - before);

//	before = get_time_us();
	if (tokens.Count > 0) {
		Token *first = &(tokens.Data[0]);
		Token *last = &(tokens.Data[tokens.Count-1]);
		gtk_text_buffer_remove_all_tags(text_buffer, &first->start, &last->end);

//		char *text = gtk_text_buffer_get_text(text_buffer, &(first->start), &(last->end), FALSE);
//		printf("highlighted range:\n---\n%s\n---\n", text);
//		free(text);
	}
//	after = get_time_us();
//	printf("ELAPSED ON REMOVING TAGS: %ld\n", after - before);

	for (int i = 0; i < tokens.Count; ++i)
	{
		Token *token = &tokens.Data[i];
		if (token->type == TOKEN_IDENTIFIER)
		{
			Token *next_token = ((i+1) < tokens.Count) ? &tokens.Data[i+1] : 0;
			if (next_token && next_token->type == TOKEN_IDENTIFIER)
			{
				token->type = TOKEN_TYPE;
				continue;
			}
			if (next_token && next_token->type == TOKEN_POINTER_STAR)
			{
				Token *next_next_token = ((i+2) < tokens.Count) ? &tokens.Data[i+2] : 0;
				if (next_next_token && next_next_token->type == TOKEN_IDENTIFIER)
				{
					token->type = TOKEN_TYPE;
					continue;
				}
			}
			if (next_token && next_token->type == TOKEN_OPERATOR)
			{
				Token *next_next_token = ((i+2) < tokens.Count) ? &tokens.Data[i+2] : 0;
				if (gtk_text_iter_get_char(&next_token->start) == '(')
				{
					token->type = TOKEN_FUNCTION;
					continue;
				}
			}
		}
	}

//	before = get_time_us();
	for (int i = 0; i < tokens.Count; ++i)
	{
		Token *token = &tokens.Data[i];
		gtk_text_buffer_apply_tag_by_name(text_buffer, token_names[token->type], &token->start, &token->end);
		//@ could try apply_tag() as opposed to apply_tag_by_name(), maybe we could maintain a list of tags ourselves, worth a try.
	}
//	after = get_time_us();
//	printf("ELAPSED ON APPLYING TAGS: %ld\n", after - before);
}