#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Determines the length of an array at compile time.
#define COUNT(a) (sizeof(a) / sizeof(a[0]))

// These collide with GTK's names:
//#define MIN(a, b) (a) < (b) ? (a) : (b)
//#define MAX(a, b) (a) > (b) ? (a) : (b)

double get_time_secs();
long get_time_us();

char *read_file(const char *filename);
void write_file(const char *filename, const char *contents);
char *byte_in_binary(char byte, char *buffer);
char *int_in_binary(int four_bytes, char *buffer);
int get_utf8_char(const char *str, int i, int *num_bytes);
const char *filepath_get_basename(const char *filepath);
const char *basename_get_extension(const char *basename);


//#define INITIALLY_ALLOCATED 100
//
//struct Array {
//	int count;
//	int allocated;
//	void **data;
//};
//
//void array_init(Array *array);
//Array *array_new();
////void array_free(Array *array); //@ need something along these lines
//void array_add(Array *array, void *element);


#define INITIALLY_ALLOCATED 100

template <typename T>
struct Array {
	int count;
	int allocated;
	T *data;
};

template <typename T>
void array_init(Array<T> *array){
	array->count = 0;
	array->allocated = INITIALLY_ALLOCATED;
	array->data = (T *) malloc(array->allocated * sizeof(T)); //@
}

template <typename T>
Array<T> *array_new(){
	Array<T> *array = (Array<T> *) malloc(sizeof(Array<T>));
	array_init(array);
	return array;
}

template <typename T>
void array_add(Array<T> *array, T element){
	assert(array->count <= array->allocated);

	if(array->count == array->allocated){
		array->allocated *= 2;
		T *p = (T *) malloc(array->allocated * sizeof(T)); //@
		for(int i = 0; i < array->count; ++i){
			p[i] = array->data[i];
		}
		free(array->data);
		array->data = p;
	}

	array->data[array->count] = element;
	array->count += 1;
}

//@ todo (things that could be improved probably):
// dynamically grow at some point (when?)
// improved hash
// improved probing
// right now we just store strings so that someone can check if we have a particular string or not, but we might want to store other things as well (?)

#define HASH_TABLE_SIZE 512

struct HashTable
{
	const char *data[HASH_TABLE_SIZE];
	int num_occupied_slots;
};

void hash_table_init(HashTable *table);
void hash_table_store(HashTable *table, const char *str);
bool hash_table_has(HashTable *table, const char *str);
void hash_table_print(HashTable *table);
