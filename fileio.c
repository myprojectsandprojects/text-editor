#include <stdio.h>
#include <stdlib.h>

char *read_file(const char *filename) {
	char *contents;
	FILE *file;
	long int size;
	int i = 0;


	file = fopen(filename, "r");
	if(file == NULL) {
		printf("read_file: couldnt open file \"%s\"\n", filename);
		return NULL;
	}

	// Determine file size:

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET); // Back to the beginning...

	//g_print("read_file: file size: %d\n", size);

	contents = (char *) malloc(size+1);
	if (contents == NULL) {
		printf("read_file: malloc() failed!\n");
		return NULL;
	}

	while((contents[i] = fgetc(file)) != -1) {
		i++;
	}
	contents[i] = 0;
	fclose(file);

	return contents;
}

void write_file(const char *filename, const char *contents) {
	FILE *file;
	int i = 0;


	file = fopen(filename, "w");
	if(file == NULL) {
		printf("write_file: couldnt open file \"%s\"\n", filename);
		return;
	}

	while(contents[i] != 0) {
		fputc(contents[i], file);
		i++;
	}

	fclose(file);
}