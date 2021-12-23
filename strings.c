#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
	int -> number of bytes copied 
	copy_string(
	const char *src, -> source buffer memory address
	char *dst, -> destination buffer memory address
	int src_i, -> index into the source buffer
	int dst_i, -> index into the destination buffer
	int n); -> maximum number of bytes to copy (size of the destination buffer)
*/
/*@ make indexes etc optional */
/*@ if we want to copy n characters we have to say n+1 because last character is overwritten by 0 */
int copy_string(
	const char *src,
	char *dst,
	int src_i,
	int dst_i,
	int n)
{
	for (int i = 0; i < n; ++i) {
		dst[dst_i + i] = src[src_i + i];
		if (dst[dst_i + i] == 0) return i + 1;
	}
	dst[dst_i + n - 1] = 0; // just overwrite the last character we copied
	//dst[n - 1] = 0;
	return n;
}

/*
	void
	copy_bytes(
	const char *src, -> source buffer memory address
	char *dst, -> destination buffer memory address
	int src_i, -> index into the source buffer
	int dst_i, -> index into the destination buffer
	int n); -> number of bytes to copy
*/
void copy_bytes(
	const char *src,
	char *dst,
	int src_i,
	int dst_i,
	int n)
{
	for (int i = 0; i < n; ++i) {
		dst[dst_i + i] = src[src_i + i];
	}
}

char *str_replace(const char *h, const char *n, const char *r)
{
	assert(h != NULL && n != NULL && r != NULL);
	assert(strlen(h) > 0);

	int matching = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	int m = 0;

	int r_len = strlen(r);
	char *result_buffer = malloc(100);

	for (int i = 0; h[i] != 0; ++i) {
		if (matching) {
			++j;
			if (n[j] == 0) {

				//found!
				matching = 0;
				j = 0;

				//strncpy(&result_buffer[m], &h[k], l - k);
				copy_bytes(h, result_buffer, k, m, l - k);
				m += l - k;
				//strncpy(&result_buffer[m], r, r_len);
				copy_bytes(r, result_buffer, 0, m, r_len);
				m += r_len;
				k = l = i;

			} else if (n[j] != h[i]) {
				matching = 0;
				j = 0;

				l = i;
				++l;
				continue;
			}
		}
		
		if (h[i] == n[j]) {
			matching = 1;
			//continue;
		} else {
			++l;
		}
	}

	if (matching && n[++j] == 0) {
		//strncpy(&result_buffer[m], &h[k], l - k);
		copy_bytes(h, result_buffer, k, m, l - k);
		m += l - k;
		//strncpy(&result_buffer[m], r, r_len);
		copy_bytes(r, result_buffer, 0, m, r_len);
		result_buffer[m + r_len] = 0;
	} else {
		/*
		strcpy(&result_buffer[m], &h[k]);
		result_buffer[m + strlen(&h[k])] = 0;
		*/
		//printf("*** in else block ***\n");
		//copy_bytes(h, result_buffer, k, m, l - k); 
		//m += l - k;
		int copied = copy_string(h, result_buffer, k, m, 9999); //@ we want to copy from k until the 0-character but its a terrible hack
		m += copied;
		result_buffer[m] = 0;
	}

	return result_buffer;
}

void test_str_replace_for(
	const char *h,
	const char *n,
	const char *r,
	const char *expected)
{
	char *result = str_replace(h, n, r);
	printf("\"%s\" -> \"%s\"\n", h, result);
	if (strcmp(result, expected) != 0) {
		printf("= Failed\n");
		exit(EXIT_SUCCESS);
	} else {
		printf("= OK\n");
	}
	free(result);
}

void test_str_replace(void)
{
	/* str_replace() doesnt deal with NULL's */
	//str_replace(NULL, NULL, NULL); // assertion failure
	//str_replace("abc", NULL, NULL); // assertion failure
	//str_replace("abc", "abc", NULL); // assertion failure

	/* first argument cant be an empty string */
	//str_replace("abc", "abc", NULL); // assertion failure

	test_str_replace_for("Hello world! Hello world!", "world", "universe", "Hello universe! Hello universe!");
	test_str_replace_for("Hello world! Hello world!", "World", "universe", "Hello world! Hello world!");
	test_str_replace_for(".one.two..three...", ".", "\\.", "\\.one\\.two\\.\\.three\\.\\.\\.");
	test_str_replace_for("abc", "abc", "x", "x");
	test_str_replace_for("abc", "abcd", "x", "abc");
	test_str_replace_for("Hello world! Hello world!", " world", "", "Hello! Hello!");
	test_str_replace_for("tab.", ".", "\\.", "tab\\.");
	//test_str_replace_for("", "", "not empty", "not empty");

	//test_str_replace_for("", "", "", ""); // assertion failure

	//test_str_replace_for("abc", NULL, NULL, NULL); // assertion failure
}


int is_beginning_of(const char *needle, const char *haystack)
{
	return strstr(haystack, needle) == haystack;
}

/* The input-string must be writable! */
char *get_slice_by(char **p_s, char ch)
{
	char *s = *p_s;
	int i = 0;

	if (s == NULL || s[i] == 0) return NULL;

	for(; s[i] != 0; ++i) if (s[i] == ch) break;

	if (s[i] != 0) {
		s[i] = 0;
		++i;
	}
/*
	if (s[i] == 0) {
//		*p_s = NULL; // Do we really need to do this?
	} else {
		*p_s = &(s[i]);
	}
*/
	*p_s = &(s[i]);

	return s;
}

char **slice_by(const char *s, char c)
{
	char **list_of_ss, **p3;
	p3 = list_of_ss = malloc(100);

	const char *p1, *p2;
	p1 = p2 = s;
	while (*p2 != 0) {
		if (*p2 == c) {
			long unsigned n_characters = p2 - p1;
			/*if (n_characters == 0) {
				printf("nothing before space..\n");
			} else {
				char *until_space = malloc(n_characters + 1);
				strncpy(until_space, p1, n_characters);
				until_space[n_characters] = 0;
				printf("until space: %s\n", until_space);
			}*/
			char *until_space = malloc(n_characters + 1);
			strncpy(until_space, p1, n_characters);
			until_space[n_characters] = 0;
			//printf("until space: %s\n", until_space);

			*p3 = until_space;
			p3 += 1; // ?
			//printf("p3: %p\n", p3);
			
			p1 = p2 + 1;
		}
		//printf("character: %c\n", *p2);
		p2 += 1;
	}

	if (p2 > p1) {
		//printf("last chunk: %s\n", p1);
		*p3 = malloc(p2 - p1 + 1);
		strcpy(*p3, p1);
		//*p3[p2 - p1] = 0;
		(*p3)[p2 - p1] = 0;
		p3 += 1;
	}

	*p3 = NULL;

	return list_of_ss;
}

char * get_until(const char *s, char c)
{
	int i;
	static const char *current_s;

	if (s != NULL) {
		current_s = s;
	}
	
	if (current_s[0] == 0) {
		return NULL;
	}

	for (i = 0; current_s[i] != 0; ++i) {
		//printf("ch: %c\n", current_s[i]);
		if (current_s[i] == c) {
			break;
		}
	}

	char *slice = malloc(i + 1);
	strncpy(slice, current_s, i);
	slice[i] = 0;

	if (current_s[i] != 0) ++i;

	current_s = current_s + i;

	return slice;
}

/* 
Its not excactly clear to me right now what should be returned for all kinds of exotic possibilities.
*/
char *get_parent_path(const char *path)
{
	char *parent_path;

	if (path == NULL) return NULL;

	int i = strlen(path) - 1;
	if (i < 0) return NULL; // if empty string

	if (path[i] == '/') i -= 1; // we'll ignore a trailing '/'

	for (; i >= 0; --i) {
		if (path[i] == '/') break;
	}

	// either '/' at the beginning or no '/':
	if (i == 0 || i < 0) {
		parent_path = malloc(1 + 1);
		parent_path[0] = '/';
		parent_path[1] = 0;
	} else {
		//int n_chars = i + 1; // count chars including the 1 i points at
		int n_chars = i; // count chars excluding the 1 i points at
		parent_path = malloc(n_chars + 1);
		strncpy(parent_path, path, n_chars);
		parent_path[n_chars] = 0; // assume strncpy didnt 0-terminate
	}

	return parent_path;
}

void test_get_parent_path()
{
	const char *path;

	path = NULL; printf("%s -> %s\n", path, get_parent_path(path));
	path = ""; printf("%s -> %s\n", path, get_parent_path(path));
	path = "/"; printf("%s -> %s\n", path, get_parent_path(path));
	path = "/one"; printf("%s -> %s\n", path, get_parent_path(path));
	path = "/one/two"; printf("%s -> %s\n", path, get_parent_path(path));
	path = "/one/two/three"; printf("%s -> %s\n", path, get_parent_path(path));

	path = "one"; printf("%s -> %s\n", path, get_parent_path(path));
	path = "one/two"; printf("%s -> %s\n", path, get_parent_path(path));
	path = "one/two/three"; printf("%s -> %s\n", path, get_parent_path(path));
	path = "one/"; printf("%s -> %s\n", path, get_parent_path(path));
	path = "one/two/"; printf("%s -> %s\n", path, get_parent_path(path));
	path = "one/two/three/"; printf("%s -> %s\n", path, get_parent_path(path));
	path = "//"; printf("%s -> %s\n", path, get_parent_path(path));
	path = "///"; printf("%s -> %s\n", path, get_parent_path(path));
}


