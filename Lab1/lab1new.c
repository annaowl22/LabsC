#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#pragma pack(push, 1)

// Структура локального заголовка файла в ZIP
typedef struct {
    uint32_t signature;        // 0x04034b50
    uint16_t version;
    uint16_t flags;
    uint16_t compression;
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t name_length;
    uint16_t extra_length;
} LocalFileHeader;

#pragma pack(pop)

bool check_jpeg_end(FILE *file) {
    int bytes[2];
    bytes[0] = fgetc(file);
    bytes[1] = fgetc(file);
    
    while (bytes[1] != EOF) {
        if (bytes[0] == 0xFF && bytes[1] == 0xD9) {
            printf("Найден конец файла JPEG\n");
            return true;
        }
        bytes[0] = bytes[1];
        bytes[1] = fgetc(file);
    }
    return false;
}

void process_zip(FILE *file) {
    LocalFileHeader header;
    bool is_zip = false;
    
    while (fread(&header, sizeof(header), 1, file) == 1) {
        if (header.signature == 0x04034b50) {
            if (!is_zip) {
                printf("Файл включает ZIP-архив. Список файлов:\n");
                is_zip = true;
            }
            
            // Читаем имя файла
            if(header.name_length < 256){
                char filename[256];
                fread(filename, header.name_length, 1, file);
                filename[header.name_length] = '\0';
                printf("%s\n", filename);
            }else{
                char filename[1024];
                fread(filename, header.name_length, 1, file);
                filename[header.name_length] = '\0';
                printf("%s\n", filename);
            }
            
            // Пропускаем дополнительные поля и данные файла
            fseek(file, header.extra_length + header.compressed_size, SEEK_CUR);
        } else {
            // Если сигнатура не найдена, отступаем назад для поиска
            fseek(file, -sizeof(header) + 1, SEEK_CUR);
        }
    }
    
    if (is_zip) {
        printf("Архив прочтен\n");
    } else {
        printf("Архив формата ZIP не обнаружен\n");
    }
}

int main(int argc, char *argv[]) {
    
    if(argc > 2){
        printf("Неверное количество аргументов. Должно быть только одно имя файла\n");
        return 1;
    }
    if(argc==1){
        printf("Не выбран файл для проверки\n");
        return 1;
    }

    
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        printf("Не удалось открыть файл\n");
        return 404;
    }
    
    // Поиск конца JPEG
    check_jpeg_end(file);
    
    // Поиск ZIP-архива
    process_zip(file);
    
    fclose(file);
    return 0;
}