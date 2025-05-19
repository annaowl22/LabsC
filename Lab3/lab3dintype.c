#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#define UNINITIALIZED 0
#define INITIALAZED 1
#define DELETED 2

static inline size_t ht_str_hash(const void *key, size_t hash_size) {
    const char* s = key;
    if(!s) return 0;
	size_t h = 0;
    while(*s){
        h += (size_t)*s++;
    }
	return h & (hash_size - 1);
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
typedef struct HashTabNode {
    char flag;
    void* key;
    void* value;
}HashTabNode;

typedef struct HashTab
{
    size_t size, max_size, capacity;
    HashTabNode *data;
    size_t(*hash)(const void*, size_t);
    bool(*equals)(const void*, const void*);
} HashTab;

void ht_init(HashTab* h, size_t(*hash)(const void*, size_t), bool(*equals)(const void*, const void*)){
    h->size = 0;
    h->max_size = 0; 
    h->capacity = 0;
	h->data = NULL;
    h->hash = hash;
    h->equals = equals;
}

void ht_clear(HashTab* h){
    h->size = 0;
    if(h->data){
        for(size_t i = 0; i < h->capacity; i++){
            h->data[i].flag = 0;
        }
    }
}

size_t ht_get(HashTab h, const void* key){
    size_t result;
	if (!(h).size || !(h).data) {
		return 0;
	}
	size_t hash_size = (h).capacity;
	result = h.hash(key, hash_size);
	for(size_t ht_step = 0; ht_step < (h).capacity; ht_step++){
        if ((h).data[(result)].flag == UNINITIALIZED){
            return 0;
        }
        if ((h).data[(result)].flag == INITIALAZED && h.equals((h).data[(result)].key, (key))){
            return result;
        }
        (result) = ((result) + ht_step + 1) & (hash_size - 1);
    }
    return 0;
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
	HashTabNode *new_data = calloc(ht_new_capacity,sizeof(HashTabNode));
	if (!new_data) {
		return false;
	}

	size_t hash_size = ht_new_capacity;
	for (size_t ht_i = 0; ht_i < (h)->capacity; ht_i++) {
		if ((h)->data[ht_i].flag != INITIALAZED) continue;
		size_t ht_j = h->hash((h)->data[ht_i].key, hash_size);
		size_t ht_step = 0;
		while (new_data[ht_j].flag) {
			ht_j = (ht_j + ++ht_step) & (hash_size - 1);
		}
		new_data[ht_j].flag = 1;
		new_data[ht_j].key = (h)->data[ht_i].key;
		new_data[ht_j].value = (h)->data[ht_i].value;
	}
	free((h)->data);
	(h)->data = new_data;
	(h)->capacity = ht_new_capacity;
	(h)->max_size = (ht_new_capacity >> 1) + (ht_new_capacity >> 2);
	return true;
}

bool ht_valid(HashTab h, size_t i){
    return h.data && (h).data[i].flag && (h).data[i].flag == INITIALAZED && (i < h.capacity) && h.data[i].key;
}

int ht_put(HashTab* h, const void* key, size_t key_size, const void* value, size_t value_size){
	bool ht_success;
	size_t ht_new_size = (h)->size ? (h)->size + 1 : 2;
	ht_success = ht_reserve(h, ht_new_size);
	if (!ht_success) {
		return -1;
	}
	size_t hash_size = (h)->capacity;
	size_t index = h->hash(key, hash_size);
	size_t ht_step = 0;
	while ((h)->data[(index)].flag == DELETED || index == 0 || ((h)->data[(index)].flag == INITIALAZED && !h->equals((h)->data[(index)].key, (key)))) {
		(index) = ((index) + ++ht_step) & (hash_size - 1);
	}
	if ((h)->data[(index)].flag == INITIALAZED) {
		free(h->data[index].value);
        void* new_value = malloc(value_size);
        memcpy(new_value, value, value_size);
        h->data[index].value = new_value;
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

		(h)->data[(index)].flag = INITIALAZED;
		(h)->data[(index)].key = key_copy;
        (h)->data[(index)].value = value_copy;
		(h)->size++;
		return 1;
	}
}

int ht_update(HashTab* h, const void* key, const void* value, size_t value_size){
    size_t hash_size = (h)->capacity;
	size_t index = h->hash(key, hash_size);
	size_t ht_step = 0;
	while ((h)->data[(index)].flag == DELETED || index == 0 || ((h)->data[(index)].flag == INITIALAZED && !h->equals((h)->data[(index)].key, (key)))) {
		(index) = ((index) + ++ht_step) & (hash_size - 1);
	}
    if((h)->data[index].flag == UNINITIALIZED){
        return 0;
    }

    free(h->data[index].value);
    void* new_value = malloc(value_size);
    if(!new_value){
        return 0;
    }
    memcpy(new_value, value, value_size);
    h->data[index].value = new_value;
    return 1;
}



void ht_delete(HashTab* h, size_t index){
    h->data[index].flag = DELETED;
    h->size--;
}

void ht_destroy(HashTab* h){
    for (size_t i = 0; i < h->capacity; i++){
        if(h->data[i].flag == INITIALAZED){
            free(h->data[i].value);
            free(h->data[i].key);
        }
    }
    free(h->data);
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
                if(!word_copy){
                    printf("Не удалось скопировать слово\n");
                    continue;
                }
                i = ht_get(Word_Hash_Tab,word_copy);
                if(i == 0 || !ht_valid(Word_Hash_Tab,i)){
                    int val = 1;
                    ht_put(&Word_Hash_Tab,word_copy,strlen(word_copy)+1,&val,sizeof(int));
                }else{
                    int val = *(int*)Word_Hash_Tab.data[i].value + 1;
                    ht_update(&Word_Hash_Tab,word_copy,&val,sizeof(int));
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
        i = ht_get(Word_Hash_Tab,word_copy);
        if(!ht_valid(Word_Hash_Tab,i)){
            int val = 1;
            ht_put(&Word_Hash_Tab,&word_copy,sizeof(char*),&val,sizeof(int));
        }else{
            int val = *(int*)Word_Hash_Tab.data[i].value + 1;
            ht_update(&Word_Hash_Tab, &word_copy,&val,sizeof(int));
            free(word_copy);
        }
        word_buff = NULL;
        capacity = 0;
        size = 0;
    }
    free(word_buff);
    fclose(input_file);

    for(size_t ind = 0; ind < Word_Hash_Tab.capacity;ind++){
        if(!ht_valid(Word_Hash_Tab,ind)) continue;
        printf("Слово: %s\n", (const char*)Word_Hash_Tab.data[ind].key);
        i = ht_get(Word_Hash_Tab, (const char*)Word_Hash_Tab.data[ind].key);
        printf("В тексте раз: %d\n", *(int*)Word_Hash_Tab.data[i].value);
    }
    ht_destroy(&Word_Hash_Tab);
    
    return 0;
}
