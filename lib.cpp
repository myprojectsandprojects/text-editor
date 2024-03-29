#include "lib.h"

// nanosecond accuracy
double get_time_secs()
{
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
//	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
	return (t.tv_sec + (t.tv_nsec / 1000000000.0));
}

long get_time_us()
{
	timeval time;
	gettimeofday(&time, NULL);
	return time.tv_sec * 1000000 + time.tv_usec;
}

char *read_file(const char *filename) {
	char *contents;
	FILE *file;
	long int size;
	int i = 0;


	file = fopen(filename, "r");
	if(file == NULL) {
//		printf("read_file: couldnt open file \"%s\"\n", filename);
		return NULL;
	}

	// Determine file size:

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET); // Back to the beginning...

	contents = (char *) malloc(size+1);
	if (contents == NULL) {
//		printf("read_file: malloc() failed!\n");
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

// byte_in_binary() -- takes a byte-value and returns it's binary representation
// we expect buffer to point to at least 9 bytes (8 for each bit and one extra for the terminating zero character)
char *byte_in_binary(char byte, char *buffer){
	for(int i = 0; i < 8; ++i)
		buffer[i] = (byte & (1 << (7 - i))) ? '1' : '0';
	buffer[8] = '\0';
	return buffer;
}

// we expect buffer to point to at least 33 bytes (32 for each bit and one extra for the terminating zero character)
char *int_in_binary(int four_bytes, char *buffer){
	for(int i = 0; i < 32; ++i)
		buffer[i] = (four_bytes & (1 << (31 - i))) ? '1' : '0';
	buffer[32] = '\0';
	return buffer;
}

int get_utf8_char(const char *str, int i, int *num_bytes){
	int utf8_char = 0;

	unsigned char mask = 1 << 7; // right-shifting a signed variable is weird
//	char buffer[8+1];
//	printf("mask: %s\n", byte_in_binary(mask, buffer));
//	if(str[i] & mask) printf("first bit is one\n");
//	else printf("first bit is zero\n");
	int count = 0;
	while(count < 10){
		if(str[i] & mask) count += 1;
		else break;
		mask = mask >> 1;
	}
//	printf("# of ones at the beginning: %d\n", count);

	int _0011_1111 = 63;
	int _0001_1111 = 31;
	int _0000_1111 = 15;
	int _0000_0111 = 7;

	//@ do it in a single codepath?
	if(count == 0){
//		printf("byte of a one-byte utf8 character\n");
		utf8_char = str[i];
		*num_bytes = 1;
	}/*else if(count == 1){
		printf("not the first byte of a utf8 character\n");
	}*/else if(count == 2){
//		printf("first byte of a two-byte utf8 character\n");
		utf8_char |= (str[i] & _0001_1111) << 6;
		utf8_char |= (str[i+1] & _0011_1111);
		*num_bytes = 2;
	}else if(count == 3){
//		printf("first byte of a three-byte utf8 character\n");
		utf8_char |= (str[i] & _0000_1111) << 12;
		utf8_char |= (str[i+1] & _0011_1111) << 6;
		utf8_char |= (str[i+2] & _0011_1111);
		*num_bytes = 3;
	}else if(count == 4){
//		printf("first byte of a four-byte utf8 character\n");
		utf8_char |= (str[i] & _0000_0111) << 18;
		utf8_char |= (str[i+1] & _0011_1111) << 12;
		utf8_char |= (str[i+2] & _0011_1111) << 6;
		utf8_char |= (str[i+3] & _0011_1111);
		*num_bytes = 4;
	}else{
//		printf("not a valid byte of a utf8 character\n");
		*num_bytes = -1;
	}

	return utf8_char;
}

//void array_init(Array *array){
//	array->count = 0;
//	array->allocated = INITIALLY_ALLOCATED;
//	array->data = (void **) malloc(array->allocated * sizeof(void *)); //@
//}
//
//Array *array_new(){
//	Array *array = (Array *) malloc(sizeof(Array));
//	array_init(array);
//	return array;
//}
//
//void array_add(Array *array, void *element){
//	assert(array->count <= array->allocated);
//
//	if(array->count == array->allocated){
//		array->allocated *= 2;
//		void **p = (void **) malloc(array->allocated * sizeof(void *)); //@
//		for(int i = 0; i < array->count; ++i){
//			p[i] = array->data[i];
//		}
//		free(array->data);
//		array->data = p;
//	}
//
//	array->data[array->count] = element;
//	array->count += 1;
//}

const char *filepath_get_basename(const char *filepath){
	assert(filepath);

	int length = strlen(filepath);
	assert(length > 0); // ?
	const char *p = filepath + length - 1;
	for(; p > filepath; --p){
		if(*p == '/'){
			p += 1;
			break;
		}
	}

	return p;
}

const char *basename_get_extension(const char *basename){
	assert(basename);

	int length = strlen(basename);
	assert(length > 0); // ?
	const char *p = basename + length - 1;
	for(; p > basename; --p){
		if(*p == '.'){
			p += 1;
			break;
		}
	}

	if(p == basename){ // ".file" and "file" do not have basenames
		return "";
	}

	return p;
}

void hash_table_init(HashTable *table)
{
	for (int i = 0; i < HASH_TABLE_SIZE; ++i)
	{
		table->data[i] = 0;
	}
	table->num_occupied_slots = 0;
}

void hash_table_store(HashTable *table, const char *str)
{
	unsigned int hash = 0;
	for (const char *p = str; *p; ++p)
	{
		hash += *p;
	}
	unsigned int index = hash % HASH_TABLE_SIZE;

	// we want to keep 1 empty slot, because otherwise when the table is full the lookup code will loop forever.
	assert(table->num_occupied_slots < HASH_TABLE_SIZE-1);

	while (table->data[index])
	{
		if (strcmp(table->data[index], str) == 0)
		{
			// string already there
			return;
		}
		index += 1;
		index %= HASH_TABLE_SIZE;
	}
	table->data[index] = str;
	table->num_occupied_slots += 1;
}

bool hash_table_has(HashTable *table, const char *str)
{
	unsigned int hash = 0;
	for (const char *p = str; *p; ++p)
	{
		hash += *p;
	}
	unsigned int index = hash % HASH_TABLE_SIZE;

//	int num_comparsions = 0;
	bool found = false;
	while (table->data[index])
	{
//		num_comparsions += 1;
		if (strcmp(table->data[index], str) == 0)
		{
			found = true;
			break;
		}
		index += 1;
		index %= HASH_TABLE_SIZE;
	}
//	printf("num_comparsions for \"%s\": %d\n", str, num_comparsions);
	return found;
}

void hash_table_print(HashTable *table)
{
	for (int i = 0; i < HASH_TABLE_SIZE; ++i)
	{
		if (table->data[i])
		{
			unsigned int hash = 0;
			for (const char *p = table->data[i]; *p; ++p)
			{
				hash += *p;
			}
			unsigned int index = hash % HASH_TABLE_SIZE;
			printf("%d: %s (%d)\n", i, table->data[i], index);
		}
		else
		{
			printf("%d: -\n", i);
		}
	}
}
