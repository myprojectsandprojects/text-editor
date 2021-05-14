#include <stdlib.h>
#include <string.h>

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

	if (s[i] == 0) {
		*p_s = NULL;
	} else {
		*p_s = &(s[i]);
	}

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


