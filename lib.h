#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Determines the length of an array at compile time.
#define COUNT(a) (sizeof(a) / sizeof(a[0]))

// These collide with GTK's names:
//#define MIN(a, b) (a) < (b) ? (a) : (b)
//#define MAX(a, b) (a) > (b) ? (a) : (b)

#define TIME_START\
	struct timeval start_time;\
	gettimeofday(&start_time, NULL);

#define TIME_END\
	struct timeval end_time;\
	gettimeofday(&end_time, NULL);

// Returnes time elapsed between TIME_START and TIME_END in microseconds.
#define TIME_ELAPSED\
	(1000000 * (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec))

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