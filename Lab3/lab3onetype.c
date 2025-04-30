#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static inline size_t ht_str_hash(const char *s) {
	size_t h = (size_t) *s;
	if (h) {
		for(++s; *s; ++s) {
			h = (h << 5) - h + (size_t) *s;
		}
	}
	return h;
}

#define ht_str_eq(a, b) (strcmp((a), (b)) == 0)

/** Round up a 8-bit integer variable `x` to a nearest power of 2  */
#define roundup8(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, ++(x))

/** Round up a 16-bit integer variable `x` to a nearest power of 2  */
#define roundup16(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, ++(x))

/** Round up a 32-bit integer variable `x` to a nearest power of 2  */
#define roundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

/** Round up a 64-bit integer variable `x` to a nearest power of 2  */
#define roundup64(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, (x)|=(x)>>32, ++(x))

#if SIZE_MAX == UINT64_MAX
/** Round up a size_t variable `x` to a nearest power of 2  */
#define roundupsize(x) roundup64(x)
#elif SIZE_MAX == UINT32_MAX
/** Round up a size_t variable `x` to a nearest power of 2  */
#define roundupsize(x) roundup32(x)
#elif SIZE_MAX == UINT16_MAX
/** Round up a size_t variable `x` to a nearest power of 2  */
#define roundupsize(x) roundup16(x)
#elif SIZE_MAX == UINT8_MAX
/** Round up a size_t variable `x` to a nearest power of 2  */
#define roundupsize(x) roundup8(x)
#endif

//Структура Хэш-таблицы
typedef struct HashTab
{
    size_t size, max_size, capacity;
    char *flags;
    const char* *keys;
    int *values;
} HashTab;

void ht_init(HashTab* h){
    h->size = 0;
    h->max_size = 0; 
    h->capacity = 0;
	h->flags = NULL; 
	h->keys = NULL; 
	h->values = NULL; 
}

void ht_clear(HashTab* h){
    h->size = 0;
    if(h->flags){
        memset(h->flags, 0, h->capacity);
    }
}

size_t ht_get(HashTab h, const char* key){
    size_t result;
	if (!(h).size) {
		return 0;
	}
	size_t ht_mask = (h).capacity - 1;
	result = ht_str_hash(key) & ht_mask;
	size_t ht_step = 0;
	while ((h).flags[(result)] == 2 || ((h).flags[(result)] == 1 && !ht_str_eq((h).keys[(result)], (key)))) {
		(result) = ((result) + ++ht_step) & ht_mask;
	}
    return result;
} 

bool ht_reserve(HashTab* h, size_t new_capacity){
	if (new_capacity <= (h)->max_size) {
		return true;
    }
	size_t ht_new_capacity = (new_capacity);
	roundupsize(ht_new_capacity);
	if (ht_new_capacity <= (h)->capacity) {
		ht_new_capacity <<= 1;
	}
	char *ht_new_flags = malloc(ht_new_capacity);
	if (!ht_new_flags) {
		return false;
	}
	const char* *ht_new_keys = malloc(ht_new_capacity * sizeof(const char*));
	if (!ht_new_keys) {
		free(ht_new_flags);
		return false;
	}
	int *ht_new_values = malloc(ht_new_capacity * sizeof(int));
	if (!ht_new_values) {
		free(ht_new_keys);
		free(ht_new_flags);
		return false;
	}
	memset(ht_new_flags, 0, ht_new_capacity);
	size_t ht_mask = ht_new_capacity - 1;
	for (size_t ht_i = 0; ht_i < (h)->capacity; ht_i++) {
		if ((h)->flags[ht_i] != 1) continue;
		size_t ht_j = ht_str_hash((h)->keys[ht_i]) & ht_mask;
		size_t ht_step = 0;
		while (ht_new_flags[ht_j]) {
			ht_j = (ht_j + ++ht_step) & ht_mask;
		}
		ht_new_flags[ht_j] = 1;
		ht_new_keys[ht_j] = (h)->keys[ht_i];
		ht_new_values[ht_j] = (h)->values[ht_i];
	}
	free((h)->values);
	free((h)->keys);
	free((h)->flags);
	(h)->flags = ht_new_flags;
	(h)->keys = ht_new_keys;
	(h)->values = ht_new_values;
	(h)->capacity = ht_new_capacity;
	(h)->max_size = (ht_new_capacity >> 1) + (ht_new_capacity >> 2);
	return true;
}

bool ht_valid(HashTab h, size_t i){
    return (h).flags && (h).flags[i] == 1;
}

int ht_put(HashTab* h, const char* key, size_t* index){
	bool ht_success;
	size_t ht_new_size = (h)->size ? (h)->size + 1 : 2;
	ht_success = ht_reserve(h, ht_new_size);
	if (!ht_success) {
		return -1;
	}
	size_t ht_mask = (h)->capacity - 1;
	*(index) = ht_str_hash(key) & ht_mask;
	size_t ht_step = 0;
	while ((h)->flags[*(index)] == 2 || ((h)->flags[*(index)] == 1 && !ht_str_eq((h)->keys[(*index)], (key)))) {
		*(index) = (*(index) + ++ht_step) & ht_mask;
	}
	if ((h)->flags[*(index)] == 1) {
		return 0;
	} else {
		(h)->flags[*(index)] = 1;
		(h)->keys[*(index)] = (key);
		(h)->size++;
		return 1;
	}
}



void ht_delete(HashTab* h, size_t index){
    h->flags[index] = 2;
    h->size--;
}

void ht_destroy(HashTab* h){
    free(h->flags);
    free(h->keys);
    free(h->values);
}

int main(int argc, char *argv[]){

    //Проверка на количество аргументов
    if(argc > 2){
        printf("Неверное количество аргументов. Должно быть только одно имя файла\n");
        return 1;
    }
    if(argc==1){
        printf("Не выбран файл для проверки\n");
        return 1;
    }

    //Проверка на открытие файла
    FILE *input_file = fopen(argv[1],"rb");
    if(input_file == NULL){
        printf("Не удалось открыть файл\n");
        return 404;
    }

    HashTab Word_Hash_Tab;
    ht_init(&Word_Hash_Tab);
    char buff;
    char* word_buff = NULL;
    size_t capacity = 0;
    size_t size = 0;
    size_t i = 0;
    int absent;

    while(fread(&buff,1,1,input_file)==1){
        if(!isalpha(buff)){
            if(word_buff && size!=0){
                word_buff[size] = '\0';
                absent = ht_put(&Word_Hash_Tab,word_buff,&i);
                if(absent){
                    Word_Hash_Tab.values[i] = 1;
                }else{
                    Word_Hash_Tab.values[i] += 1;
                }
                word_buff = NULL;
                capacity = 0;
                size = 0;
            }
        }else {
            if(size + 1 >= capacity){
                capacity = capacity == 0 ? 16 : capacity * 2;
                char *temp = realloc(word_buff, capacity);
                if (temp == NULL){
                    free(word_buff);
                    ht_destroy(&Word_Hash_Tab);
                    fclose(input_file);
                    perror("Memory allocation failed");
                    return 1;
                }
                word_buff = temp;
            }
            word_buff[size++] = buff;
        }
    }
    if(word_buff && size!=0){
        word_buff[size] = '\0';
        ht_put(&Word_Hash_Tab,word_buff,&i);
        if(absent){
            Word_Hash_Tab.values[i] = 1;
        }else{
            Word_Hash_Tab.values[i] += 1;
        }
    }
    free(word_buff);
    fclose(input_file);

    for(size_t ind = 0; ind < Word_Hash_Tab.capacity;ind++){
        if(!ht_valid(Word_Hash_Tab,ind)||strlen(Word_Hash_Tab.keys[ind])==0) continue;
        printf("Слово: %s\n", Word_Hash_Tab.keys[ind]);
        i = ht_get(Word_Hash_Tab, Word_Hash_Tab.keys[ind]);
        printf("В тексте раз: %d\n", Word_Hash_Tab.values[i]);
    }
    ht_destroy(&Word_Hash_Tab);
    
    return 0;
}
