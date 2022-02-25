#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "declarations.h"


#define OK 0
#define FAILED 1


/* main.c */

//void test_table(void)
//{
//	printf("test_table()\n");
//
//	struct Table *t = table_create();
//	int *p = (int *) malloc(sizeof(int));
//	*p = 123;
//	table_store(t, "the number", (void *) p);
//	void *d = table_get(t, "the number");
//	int the_number = *((int *) d);
//	printf("test_table: the number: %d\n", the_number);
//	table_store(t, "the string", (void *) "forty two");
//	d = table_get(t, "the string");
//	const char *the_str = (const char *) d;
//	printf("test_table: the string: %s\n", the_str);
//}


/* strings.c */

void test_case_trim_whitespace(const char *original)
{
	char *s = strdup(original);
	printf("\"%s\" -> \"%s\"\n", original, trim_whitespace(s));
	free(s);
}

void test_trim_whitespace(void)
{
	test_case_trim_whitespace("abc");
	test_case_trim_whitespace(" abc ");
	test_case_trim_whitespace("	abc	");
	test_case_trim_whitespace("	 	abc");
	test_case_trim_whitespace(" ");
	test_case_trim_whitespace("");
	//trim_whitespace(NULL); // assertion fail
}

// not automated, just a sensibility-check for time being
void test_get_word_with_allocate(void)
{
	char s[] = "This is a sentence!";
	//char s[] = "abc  123	xyz 	\n abc-+*/123";
	//char s[] = "";
	//char s[] = "¹@£$½¬{[]}\`]}";
	//char s[] = "\n¹@£$½¬\n\n{[]}\n\`]}\n"; // unknown espace sequence warning
	char *p = (char *) s;

	printf("%s\n", s);
	while (char *w = get_word_with_allocate(&p)) {
		printf("word: %s\n", w);
		free(w);
	}
}


/* search-replace.c: */


int test_case_parse_str(const char *str2parse,
	int expected_action,
	int expected_line_num,
	const char *expected_search_str,
	const char *expected_replace_with_str)
{
	LOG_MSG("test_case_parse_str()\n");

	int line_num;
	char *search_str, *replace_with_str;
	int action = parse_str(str2parse,
		&line_num, &search_str, &replace_with_str);

	int result = FAILED;

	if (action != expected_action) {
		goto _RETURN_FROM_FUNCTION;
	}

	if (line_num != expected_line_num) {
		goto _RETURN_FROM_FUNCTION;
	}

	if (expected_search_str == NULL && search_str != NULL) {
		// failure (should be NULL)
		goto _RETURN_FROM_FUNCTION;
	} else if (expected_search_str != NULL && search_str == NULL) {
		// failure (shouldnt be NULL)
		goto _RETURN_FROM_FUNCTION;
	} else if (expected_search_str != NULL && search_str != NULL
		&& strcmp(search_str, expected_search_str) != 0) {
		// failure (neither one is NULL, but strings dont match)
		goto _RETURN_FROM_FUNCTION;
	}

	if (expected_replace_with_str == NULL && replace_with_str != NULL) {
		// failure (should be NULL)
		goto _RETURN_FROM_FUNCTION;
	} else if (expected_replace_with_str != NULL && replace_with_str == NULL) {
		// failure (shouldnt be NULL)
		goto _RETURN_FROM_FUNCTION;
	} else if (expected_replace_with_str != NULL && replace_with_str != NULL
		&& strcmp(replace_with_str, expected_replace_with_str) != 0) {
		// failure (neither one is NULL, but strings dont match)
		goto _RETURN_FROM_FUNCTION;
	}

	result = OK;

_RETURN_FROM_FUNCTION:
	if (result == OK) {
		printf("\"%s\" ... OK\n", str2parse);
	} else {
		printf("\"%s\" ... FAILED\n", str2parse);
	}
	return result;
}


void test_parse_str(void)
{
	LOG_MSG("test_parse_str()\n");

	printf("testing function \"parse_str()\" for following cases:\n");

	int r = test_case_parse_str(
		"abc", 	// string to parse
		SEARCH, 	// expected action to take
		-1, 		// expected line number
		"abc", 	// expected string to search
		NULL); 	// expected replacement string for replace
	if (r == FAILED) {
		exit(0); // we exit at 1st failure
	}

	//if (test_case_parse_str("abc", SEARCH, -1, "123", NULL) == FAILED)	exit(0);
	if (test_case_parse_str(":123", GO_TO_LINE, 123, NULL, NULL) == FAILED) exit(0);
	if (test_case_parse_str("abc/def", REPLACE, -1, "abc", "def") == FAILED) exit(0);
	if (test_case_parse_str("\t/\n", REPLACE, -1, "\t", "\n") == FAILED) exit(0);
	if (test_case_parse_str("\\:/ =", REPLACE, -1, ":", " =") == FAILED) exit(0);
}