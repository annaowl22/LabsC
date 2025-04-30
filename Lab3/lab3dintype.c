#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>


static inline size_t ht_str_hash(const void *key) {
    const char* s = key;
	size_t h = (size_t) *s;
	return h;
}

bool ht_str_eq(const void*a, const void* b){
    return strcmp(a,b) == 0;
}

static inline size_t ht_char_hash(const void *key){
    const char s = *(const char*)key;
    int h = (int) s;
	return h;
}

bool ht_char_eq(const void* a, const void* b){
    return *(const char*)a == *(const char*)b;
}

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
    void* *keys;
    void* *values;
    size_t* key_sizes;
    size_t(*hash)(const void*);
    bool(*equals)(const void*, const void*);
} HashTab;

void ht_init(HashTab* h, size_t(*hash)(const void*), bool(*equals)(const void*, const void*)){
    h->size = 0;
    h->max_size = 0; 
    h->capacity = 0;
	h->flags = NULL;
    h->key_sizes = NULL;
	h->keys = NULL; 
	h->values = NULL;
    h->hash = hash;
    h->equals = equals;
}

void ht_clear(HashTab* h){
    h->size = 0;
    if(h->flags){
        memset(h->flags, 0, h->capacity);
    }
}

size_t ht_get(HashTab h, const void* key){
    size_t result;
	if (!(h).size) {
		return 0;
	}
	size_t ht_mask = (h).capacity - 1;
	result = h.hash(key) & ht_mask;
	size_t ht_step = 0;
	while ((h).flags[(result)] == 2 || ((h).flags[(result)] == 1 && !h.equals((h).keys[(result)], (key)))) {
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
	void* *ht_new_keys = malloc(ht_new_capacity * sizeof(void*));
	if (!ht_new_keys) {
		free(ht_new_flags);
		return false;
	}
	void* *ht_new_values = malloc(ht_new_capacity * sizeof(void*));
	if (!ht_new_values) {
		free(ht_new_keys);
		free(ht_new_flags);
		return false;
	}
	memset(ht_new_flags, 0, ht_new_capacity);
	size_t ht_mask = ht_new_capacity - 1;
	for (size_t ht_i = 0; ht_i < (h)->capacity; ht_i++) {
		if ((h)->flags[ht_i] != 1) continue;
		size_t ht_j = h->hash((h)->keys[ht_i]) & ht_mask;
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

int ht_put(HashTab* h, const void* key, size_t key_size, const void* value, size_t value_size){
	bool ht_success;
	size_t ht_new_size = (h)->size ? (h)->size + 1 : 2;
	ht_success = ht_reserve(h, ht_new_size);
	if (!ht_success) {
		return -1;
	}
	size_t ht_mask = (h)->capacity - 1;
	size_t index = h->hash(key) & ht_mask;
	size_t ht_step = 0;
	while ((h)->flags[(index)] == 2 || ((h)->flags[(index)] == 1 && !h->equals((h)->keys[(index)], (key)))) {
		(index) = ((index) + ++ht_step) & ht_mask;
	}
	if ((h)->flags[(index)] == 1) {
		free(h->values[index]);
        void* new_value = malloc(value_size);
        memcpy(new_value, value, value_size);
        h->values[index] = new_value;
        return 0;
	} else {
        void* key_copy = malloc(key_size);
        void* value_copy = malloc(value_size);
        if(!key_copy || !value_copy){
            free(key_copy);
            free(value_copy);
            return -1;
        }
        memcpy(key_copy, key, key_size);
        memcpy(value_copy, value, value_size);

		(h)->flags[(index)] = 1;
		(h)->keys[(index)] = key_copy;
        (h)->values[(index)] = value_copy;
        (h)->key_sizes[(index)] = key_size; //Здесь возникает ошибка сегментации. Я исправлю позже
		(h)->size++;
		return 1;
	}
}



void ht_delete(HashTab* h, size_t index){
    h->flags[index] = 2;
    h->size--;
}

void ht_destroy(HashTab* h){
    for (size_t i = 0; i < h->capacity; i++){
        if(h->flags[i] == 1){
            free(h->values[i]);
            free(h->keys[i]);
        }
    }
    free(h->key_sizes);
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
    ht_init(&Word_Hash_Tab,ht_str_hash,ht_str_eq);
    char buff;
    char* word_buff = NULL;
    size_t capacity = 0;
    size_t size = 0;
    size_t i = 0;

    while(fread(&buff,1,1,input_file)==1){
        if(!isalpha(buff)){
            if(word_buff && size!=0){
                word_buff[size] = '\0';

                char* word_copy = malloc(strlen(word_buff)+1);
                strcpy(word_copy,word_buff);
                i = ht_get(Word_Hash_Tab,word_copy);
                if(!ht_valid(Word_Hash_Tab,i)){
                    int val = 1;
                    ht_put(&Word_Hash_Tab,word_copy,strlen(word_copy)+1,&val,sizeof(int));
                }else{
                    int val = *(int*)Word_Hash_Tab.values[i] + 1;
                    ht_put(&Word_Hash_Tab, word_copy, strlen(word_copy)+1,&val,sizeof(int));
                    free(word_copy);
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

        char* word_copy = malloc(strlen(word_buff)+1);
        strcpy(word_copy,word_buff);
        i = ht_get(Word_Hash_Tab,&word_copy);
        if(!ht_valid(Word_Hash_Tab,i)){
            int val = 1;
            ht_put(&Word_Hash_Tab,&word_buff,sizeof(char*),&val,sizeof(int));
        }else{
            int val = *(int*)Word_Hash_Tab.values[i] + 1;
            ht_put(&Word_Hash_Tab, &word_buff, sizeof(char*),&val,sizeof(int));
            free(word_copy);
        }
        word_buff = NULL;
        capacity = 0;
        size = 0;
    }
    free(word_buff);
    fclose(input_file);

    for(size_t ind = 0; ind < Word_Hash_Tab.capacity;ind++){
        if(!ht_valid(Word_Hash_Tab,ind)||strlen(Word_Hash_Tab.keys[ind])==0) continue;
        printf("Слово: %s\n", *(const char**)Word_Hash_Tab.keys[ind]);
        i = ht_get(Word_Hash_Tab, *(const char**)Word_Hash_Tab.keys[ind]);
        printf("В тексте раз: %d\n", *(int*)Word_Hash_Tab.values[i]);
    }
    ht_destroy(&Word_Hash_Tab);
    
    return 0;
}
