#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define WORD 256
#define PRIME 103

//Структура Хэш-таблицы
#define HT(key_type, value_type) struct { \
	size_t size, max_size, capacity; \
	char *flags; \
	key_type *keys; \
	value_type *values; \
}

//Инициализация хэш-таблицы
#define ht_init(h) do { \
	(h).size = (h).max_size = (h).capacity = 0; \
	(h).flags = NULL; \
	(h).keys = NULL; \
	(h).values = NULL; \
} while (0)

//Простые макросы
#define ht_size(h) ((h).size)
#define ht_max_size(h) ((h).max_size)
#define ht_capacity(h) ((h).capacity)
#define ht_key(h, index) ((h).keys[(index)])
#define ht_value(h, index) ((h).values[(index)])

//Очистка таблицы (в данном задании не так нужна, но пусть будет)
#define ht_clear(h) do { \
    (h).size = 0; \
    if ((h).flags) { memset((h).flags, 0, (h).capacity); } \
} while (0)

//Взятие по ключу
#define ht_get(h, key, result, hash_func, eq_func) do { \
	if (!(h).size) { \
		(result) = 0; \
		break; \
	} \
	size_t ht_mask = (h).capacity - 1; \
	(result) = hash_func(key) & ht_mask; \
	size_t ht_step = 0; \
	while ((h).flags[(result)] == 2 || ((h).flags[(result)] == 1 && !eq_func((h).keys[(result)], (key)))) { \
		(result) = ((result) + ++ht_step) & ht_mask; \
	} \
} while (0)

//Проверка существования элемента, так как взятие по ключу может вернуть свободный индекс
#define ht_valid(h, index) ((h).flags && (h).flags[(index)] == 1)


//Добавление элемента в хэш-таблицу
#define ht_put(h, key_type, value_type, key, index, absent, hash_func, eq_func) do { \
	bool ht_success; \
	size_t ht_new_size = (h).size ? (h).size + 1 : 2; \
	ht_reserve((h), key_type, value_type, ht_new_size, ht_success, hash_func); \
	if (!ht_success) { \
		(absent) = -1; \
		break; \
	} \
	size_t ht_mask = (h).capacity - 1; \
	(index) = hash_func(key) & ht_mask; \
	size_t ht_step = 0; \
	while ((h).flags[(index)] == 2 || ((h).flags[(index)] == 1 && !eq_func((h).keys[(index)], (key)))) { \
		(index) = ((index) + ++ht_step) & ht_mask; \
	} \
	if ((h).flags[(index)] == 1) { \
		(absent) = 0; \
	} else { \
		(h).flags[(index)] = 1; \
		(h).keys[(index)] = (key); \
		(h).size++; \
		(absent) = 1; \
	} \
} while (0)


//Проверка, достаточно ли в таблице места для добавления, и расширение, если нет
#define ht_reserve(h, key_type, value_type, new_capacity, success, hash_func) do { \
	if (new_capacity <= (h).max_size) { \
		(success) = true; \
		break; \
	} \
	size_t ht_new_capacity = (new_capacity); \
	roundupsize(ht_new_capacity); \
	if (ht_new_capacity <= (h).capacity) { \
		ht_new_capacity <<= 1; \
	} \
	char *ht_new_flags = malloc(ht_new_capacity); \
	if (!ht_new_flags) { \
		(success) = false; \
		break; \
	} \
	key_type *ht_new_keys = malloc(ht_new_capacity * sizeof(key_type)); \
	if (!ht_new_keys) { \
		free(ht_new_flags); \
		(success) = false; \
		break; \
	} \
	value_type *ht_new_values = malloc(ht_new_capacity * sizeof(value_type)); \
	if (!ht_new_values) { \
		free(ht_new_keys); \
		free(ht_new_flags); \
		(success) = false; \
		break; \
	} \
	memset(ht_new_flags, 0, ht_new_capacity); \
	size_t ht_mask = ht_new_capacity - 1; \
	for (size_t ht_i = 0; ht_i < (h).capacity; ht_i++) { \
		if ((h).flags[ht_i] != 1) continue; \
		size_t ht_j = hash_func((h).keys[ht_i]) & ht_mask; \
		size_t ht_step = 0; \
		while (ht_new_flags[ht_j]) { \
			ht_j = (ht_j + ++ht_step) & ht_mask; \
		} \
		ht_new_flags[ht_j] = 1; \
		ht_new_keys[ht_j] = (h).keys[ht_i]; \
		ht_new_values[ht_j] = (h).values[ht_i]; \
	} \
	free((h).values); \
	free((h).keys); \
	free((h).flags); \
	(h).flags = ht_new_flags; \
	(h).keys = ht_new_keys; \
	(h).values = ht_new_values; \
	(h).capacity = ht_new_capacity; \
	(h).max_size = (ht_new_capacity >> 1) + (ht_new_capacity >> 2); \
	(success) = true; \
} while (0)

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

//Удаление элемента
#define ht_delete(h, index) do { \
	(h).flags[(index)] = 2; \
	(h).size--; \
} while (0)

//Флаги для перебора значений
#define ht_begin(h) (0)
#define ht_end(h) ((h).capacity)

#define ht_delete(h) do {\
    free((h).flags);\
    free((h).keys);\
    free((h).values);\
} while (0)



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



    
    return 0;
}