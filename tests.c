#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "declarations.h"


#define OK 0
#define FAILED 1


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