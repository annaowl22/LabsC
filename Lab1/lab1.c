#include <stdio.h>
#include <stdbool.h>

//Проверка на совпадение с сигнатурой
bool check_signature(int bytes[], int signature[],int size){
    for(int i = 0; i < size;i++){
        if(bytes[i]!=signature[i]){
            return false;
        }
    }
    return true;
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

    FILE *input_file = fopen(argv[1],"r");
    if(input_file == NULL){
        printf("Не удалось открыть файл\n");
        return 404;
    }
    bool iszip = false;//Переменная, хранящая результат проверки на zip

    //Поиск конца файла jpeg
    int signature1[2] = {0xFF,0xD9};
    int seekjpgend[2] = {fgetc(input_file),fgetc(input_file)};
    while(seekjpgend[1]!=EOF){
        if(check_signature(seekjpgend,signature1,2)){
            printf("Найден конец файла jpeg\n");
            break;
        }
        seekjpgend[0] = seekjpgend[1];
        seekjpgend[1] = getc(input_file);
    }

    //Поиск всех локальных заголовков по сигнатуре
    int signature2[4] = {0x50,0x4b,0x03,0x04};
    int seeklocalheader[4] = {fgetc(input_file),fgetc(input_file),fgetc(input_file),fgetc(input_file)};

    int name_length;
    while(seeklocalheader[3]!=EOF){
        if(check_signature(seeklocalheader,signature2,4)){
            if(!iszip){
                printf("Файл включает zip-архив. Список файлов: \n");
                iszip = true;
            }
            fseek(input_file,22,SEEK_CUR);
            name_length=fgetc(input_file)+fgetc(input_file)*256;
            fseek(input_file,2,SEEK_CUR);
            for(int i = 0; i < name_length; i++){
                putchar(fgetc(input_file));
            }
            printf("\n");
        }
        seeklocalheader[0] = seeklocalheader[1];
        seeklocalheader[1] = seeklocalheader[2];
        seeklocalheader[2] = seeklocalheader[3];
        seeklocalheader[3] = fgetc(input_file);
    }

    printf("Конец файла\n");
    if(iszip){
        printf("Архив прочтён\n");
    }else{
        printf("Архив формата zip не обнаружен\n");
    }


    fclose(input_file);
    return 0;
}