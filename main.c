#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include "declarations.h"

#define err_message printf("error\n");
#define err_continue printf("error\n");continue;
#define err_return printf("error\n");return;
#define err_neg printf("error\n");return -1;
#define ldisk_size 64 * 64 * sizeof(char)
#define bitmap_size 64
#define data_start 64 * 7

void put_number(void* pointer, int value){
    memcpy(pointer, &value, 4);
}
int get_number(char* pointer){

}
void microwave_disk(){
    for(int i = 0; i < 64*64; i++){
        ldisk[i] = '\0';
    }
}
void wipe_descriptor(int index){
    char descriptions[64];
    read_block(0, descriptions);
    file_descriptor* fd = (void*) descriptions + sizeof(file_descriptor)*index;
    fd->length = -1;
    fd->content_0 = -4;
    fd->content_1 = -4;
    fd->content_2 = -4;
    write_block(0, descriptions);
}
int get_free_descriptor(){
    char descriptions[64];
    read_block(1, descriptions);
    for(int i = 0; i < 24; i++){
        char block[64];
        read_block( 1 + i / 4, block);
        if ( ((file_descriptor*) block + i % 4)->length == -1 ) return i;
    }

    return -1;
}
void allocate_bitmap(int index){
    char cc[64];
    read_block(0, cc);
    cc[index] = true;
    write_block(0, cc);
}
void deallocate_bitmap(int index){
    char cc[64];
    read_block(0, cc);
    cc[index] = false;
    write_block(0, cc);
}
bool check_bitmap(int index){
    char cc[64];
    read_block(0, cc);
    return cc[index] == false;
}
void empty_bitmap(){
    for(int i = 0; i < 64; i++)deallocate_bitmap(i);
}
//void allocate_bitmap(int index){
//    int byte_index = index / 8;
//    char bitmap[64];
//    read_block(0, bitmap);
//    bitmap[byte_index] != (1 << (index % 8));
//    write_block(0, bitmap);
//}
//void deallocate_bitmap(int index){
//    int byte_index = index / 8;
//    char bitmap[64];
//    read_block(0, bitmap);
//    bitmap[byte_index] &= ~(1 << (index % 8));
//    write_block(0, bitmap);
//}
//bool check_bitmap(int index){
//    int byte_index = index / 8;
//    char bitmap[64];
//    read_block(0, bitmap);
//    return (bitmap[byte_index] >> (index % 8)) & 1;
//}
//void allocate_bitmap(int index){
//    char bitmap[64];
//    read_block(0, bitmap);
//    int64_t* b = bitmap;
//    *b |= (1 << index);
//    write_block(0, b);
//}
//void deallocate_bitmap(int index){
//    char bitmap[64];
//    read_block(0, bitmap);
//    int64_t* b = bitmap;
//    *b &= ~(1 << index);
//    write_block(0, b);
//}
//bool check_bitmap(int index){
//    char bitmap[64];
//    read_block(0, bitmap);
//    int64_t* b = bitmap;
//    return (*b >> index) & 1;
//}
int count_available_blocks(){
    int result = 0;
    for (int i = 7; i < 64; i++) result += check_bitmap(i) ? 1 : 0;
    return result;
}
int get_available_block(){
    int result = 0;
    for(int i = 7; i < 64; i++) if(check_bitmap(i)) return i;
    return -1;
}
directoy_entry* get_directory_entry(char* symbolic_file_name){
    char block[64];
    read_block(1, block);
    int directory_length = ((file_descriptor*) block)->length;
    for(int j = 0; j <= 128; j = j + 64){
        if(directory_length > j) {
            read_block(get_fd_block_index(oft[0].index, j / 3), block);
            for (int i = 0; i < 8; i++) {
                directoy_entry* entry = ((directoy_entry *) (char *) block + i);
                entry->name[3] = '\0';
                if (strcmp(entry->name, symbolic_file_name) == 0) {
                    return entry;
                }
            }
        }
    }
    return NULL;
}
int clear_directory_entry(char* symbolic_file_name){
    char block[64];
    read_block(1, block);
    int directory_length = ((file_descriptor*) block)->length;
    for(int j = 0; j <= 128; j = j + 64){
        if(directory_length > j) {
            read_block(get_fd_block_index(oft[0].index, j / 3), block);
            for (int i = 0; i < 8; i++) {
                directoy_entry* entry = ((directoy_entry *) (char *) block + i);
                entry->name[3] = '\0';
                if (strcmp(entry->name, symbolic_file_name) == 0) {
                    char blank[] = {0, 0, 0, 0, 0, 0, 0, 0};
                    memcpy(entry, blank, 8);
                    write_block(get_fd_block_index(oft[0].index, j / 3), block);
                    return 1;
                }
            }
        }
    }
    return 0;
}
int create(char* symbolic_file_name) {
    directoy_entry* entry = get_directory_entry(symbolic_file_name);
    if(entry != NULL){
        return -1;
    }
    int free_descriptor_index = get_free_descriptor();
    if(free_descriptor_index == -1) return -1;
    lseek(0, (free_descriptor_index - 1) * 4 * 2);
    char bytes[4];
    memcpy(bytes, symbolic_file_name, 4);
    symbolic_file_name[3] = '\0';
    if(write(0, symbolic_file_name, 4) == -1) return -1;
    bytes[4];
    memcpy(bytes, &free_descriptor_index, 4);
    if(write(0, bytes, 4) == -1) return -1;
    set_fd_size(free_descriptor_index, 0);
}
int is_open(int fd_index){
    for(int i = 0; i < 4; i++){
        if(oft[i].index == fd_index)
            return i;
    }
    return -1;
}
int delete(char* symbolic_file_name) {
    directoy_entry* entry = get_directory_entry(symbolic_file_name);
    if(entry == NULL || is_open(entry->index) == -1){
        return -1;
    }
    int fd_index = entry->index;
    set_fd_size(fd_index, -1);
    int index;
    if (index = get_fd_block_index(fd_index, 0) > -1) {
        deallocate_bitmap(index);
    }
    if (index = get_fd_block_index(fd_index, 1) > -1) {
        deallocate_bitmap(index);
    }
    if (index = get_fd_block_index(fd_index, 2) > -1) {
        deallocate_bitmap(index);
    }
    clear_directory_entry(symbolic_file_name);
    set_fd_block_index(fd_index, 0, -4);
    set_fd_block_index(fd_index, 1, -4);
    set_fd_block_index(fd_index, 2, -4);
    set_fd_size(fd_index, -1);
    return 0;
}
int open(char* symbolic_file_name) {
    directoy_entry* entry = get_directory_entry(symbolic_file_name);
    if(!entry){
        return -1;
    }
    int i = 1;
    for(; i < 5; i++) {
        if (oft[i].index == entry->index) return -1;
        if (oft[i].index == -1) break;
        if (i == 4) return -1;
    }
    oft[i].pos = 0;
    oft[i].index = entry->index;
    oft[i].length = get_fd_size(oft[i].index);
    if(oft[i].length > 0)
        fill_oft_buffer(i, 0);
}
int close(int index) {
    if (oft[index].index == -1) {
        return -1;
    }
    set_fd_size(oft[index].index, oft[index].length);
    oft[index].index = -1;
    return 0;
}
int read(int index, char* mem_area, int count) {
    int left_to_read = count;
    int pos_in_buffer = oft[index].pos % 64;
    int current_block = oft[index].pos / 64;
    int spot = 0;
    int current_pos = oft[index].pos;
    while(left_to_read > 0 && 
current_pos < oft[index].length && 
current_pos < 64 * 3){
        if(pos_in_buffer > 63){
            update_oft_buffer(index, current_block, current_block+1);
            pos_in_buffer = 0;
        }
        mem_area[spot] = oft->buffer[spot];
        left_to_read--;
        current_pos++;
        pos_in_buffer++;
        spot++;
    }
    if (spot == 0) return -1;
    oft[index].pos = current_pos;
    return spot;
}
int write(int index, char* mem_area, int count) {
    int left_to_read = count;
    int pos_in_buffer = oft[index].pos % 64;
    int current_block = oft[index].pos / 64;
    int spot = pos_in_buffer;
    int point = 0;
    int current_pos = oft[index].pos;
    int owned_blocks = 0;
    if (get_fd_block_index(oft[index].index, 0) != -4) owned_blocks++;
    if (get_fd_block_index(oft[index].index, 1) != -4) owned_blocks++;
    if (get_fd_block_index(oft[index].index, 2) != -4) owned_blocks++;
    int needed_blocks = 1 + ((current_pos + count) / 64);

    if((owned_blocks + count_available_blocks()) < needed_blocks || current_pos + count > 64*3) {
        return -1;
    }
    if(needed_blocks > 0 && 
get_fd_block_index(oft[index].index, 0) == -4) {
        set_fd_block_index(oft[index].index, 0, get_available_block());
    }
    if(needed_blocks > 1 && 
get_fd_block_index(oft[index].index, 1) == -4){
        set_fd_block_index(oft[index].index, 1, get_available_block());
    }
    if(needed_blocks > 2 && 
get_fd_block_index(oft[index].index, 2) == -4){
        set_fd_block_index(oft[index].index, 2, get_available_block());
    }

    while(left_to_read > 0 && 
current_pos < 64 * 3){
        if(pos_in_buffer > 63){
            update_oft_buffer(index, current_block, current_block+1);
            pos_in_buffer = 0;
        }
        oft->buffer[spot] = mem_area[point];

        left_to_read--;
        current_pos++;
        pos_in_buffer++;
        spot++;
        point++;
    }
    oft[index].pos = current_pos;
    oft[index].length = current_pos;
    set_fd_size(oft[index].index, oft[index].length);
    pull_oft_buffer(index, current_block);
    return spot;
}
int get_fd_size(int fd_index){
    char block[64];
    read_block(1 + fd_index / 4, block);
    file_descriptor* description = ((file_descriptor*)block) + fd_index % 4;
    return description->length;
}
void set_fd_size(int fd_index, int new_size){
    char block[64];
    read_block(1 + fd_index / 4, block);
    file_descriptor* description = ((file_descriptor*)block) + fd_index % 4;
    put_number( &description->length, new_size );
    write_block(1 + fd_index / 4, block);
}
int get_fd_block_index(int fd_index, int block_index){
    char block[64];
    read_block(1 + fd_index / 4, block);
    file_descriptor* description = ((file_descriptor*)block) + fd_index % 4;
    if(block_index == 0)
        return description->content_0;
    if(block_index == 1)
        return description->content_1;
    if(block_index == 2)
        return description->content_2;
}
void set_fd_block_index(int fd_index, int block_index, int value){
    char block[64];
    read_block(1 + fd_index / 4, block);
    file_descriptor* description = ((file_descriptor*)block) + fd_index % 4;
    if(block_index == 0)
        description->content_0 = value;
    if(block_index == 1)
        description->content_1 = value;
    if(block_index == 2)
        description->content_2 = value;
    allocate_bitmap(value);
    write_block(1 + fd_index / 4, block);
}
bool upgrade_oft_buffer(int oft_index, int old_block_index){
    for(int i = 7; i < 64; i++){
        if(check_bitmap(i)){
            set_fd_block_index(oft_index, old_block_index + 1, i);
        }
        return true;
    }
    return false;
}
void update_oft_buffer(int oft_index, int old_block_index, int new_block_index){
    write_block(get_fd_block_index(oft[oft_index].index, old_block_index), oft[oft_index].buffer);
    read_block(get_fd_block_index(oft[oft_index].index, new_block_index), oft[oft_index].buffer);
}
void fill_oft_buffer(int oft_index, int new_block_index){
    read_block(get_fd_block_index(oft[oft_index].index, new_block_index), oft[oft_index].buffer);
}
void pull_oft_buffer(int oft_index, int new_block_index){
    int block_num = get_fd_block_index(oft[oft_index].index, new_block_index);
    write_block(block_num, oft[oft_index].buffer);
}
int lseek(int index, int new_pos){
    if(new_pos >= oft[index].length && 
oft[index].length > 0) return -1;
    oft_slot* slot = oft + index;
    int old_pos = slot->pos;
    int old_block = old_pos / 64;
    int new_block = new_pos / 64;
    if(new_block != old_block){
        update_oft_buffer(index, old_block, new_block);
    }
    oft[index].pos = new_pos;
    return new_pos;
}
int directory(){
    char block[64];
    read_block(1, block);
    int directory_length = ((file_descriptor*) block)->length;
    int entry_count = directory_length / 8;
    for(int j = 0; j <= 128; j = j + 64){
        if(directory_length > j) {
            read_block(get_fd_block_index(oft[0].index, j / 3), block);
            for (int i = 0; i < 8; i++) {
                directoy_entry* entry = ((directoy_entry *) (char *) block + i);
                entry->name[3] = '\0';
                if(!entry->name[0] == 0) {
                    printf("%s ", entry->name);
                    entry_count--;
                }
                if(entry_count == 0){
                    printf("\n");
                    return 0;
                }
            }
        }
    }
    printf("\n");
    return 0;
}
int disk_exist (char *filename){
    struct stat stat_holder;
    return (stat(filename, &stat_holder) == 0);
}
int init(char* filename){
    ldisk;
    microwave_disk();
    bitmap = ldisk;
    descriptors = ldisk + 64;
    int directory_length = 0;
    if(strcmp(filename, "") == 0){
        for(int i = 0; i < 24; i++){
            set_fd_size(i, -1);
            set_fd_block_index(i, 0, -4);
            set_fd_block_index(i, 1, -4);
            set_fd_block_index(i, 2, -4);
        }
        set_fd_size(0, 0);
        empty_bitmap();
        for(int i = 0; i < 4; i++){
            oft_slot* slot = oft + i;
            slot->index = -1;
        }
        oft->index = 0;
        oft->pos = 0;
        oft->length = 0;
        fill_oft_buffer(0, 0);
        return 1;
    }else{
        if(!disk_exist(filename)){ return -1; }
        FILE *file;
        file = fopen(filename, "r");
        for(int i = 0; i < (16 * 64); i++) ldisk[i] = fgetc( file );
        fclose(file);
    }
    for(int i = 0; i < 4; i++){
        oft_slot* slot = oft + i;
        slot->index = -1;
    }
    oft->index = 0;
    oft->pos = 0;
    oft->length = 0;
    fill_oft_buffer(0, 0);
    return 0;
}
int save(char* filename){
    FILE *file;
    file = fopen(filename, "w");
    for(int i = 0; i < (64 * 64); i++) fputc( ldisk[i], file );
    fclose(file);
    return -1;
}
void read_block(int i, char *p){
    for(int j = 0; j < 64; j++) p[j] = ldisk[(64 * i) + j];
    memcpy(datadisk, ldisk + (7 * 64), (64*64) - (7 * 64));
}
void write_block(int i, char *p){
    for(int j = 0; j < 64; j++) ldisk[(64 * i) + j] = p[j];
    memcpy(datadisk, ldisk + (7 * 64), (64*64) - (7 * 64));
}
char **split_line(char *line, int* arg_count){
    int bufsize = 4, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, " \n");
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += 4;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, " \n");
        arg_count[0]++;
    }
    tokens[position] = NULL;
    return tokens;
}
void print_ldisk(){
    char* start = ldisk;
    printf("block 0\n");
    for(int i = 0; i < 64; i++){
        printf(" %2d", (i));
    }
    printf("\n");
    for(int i = 0; i < 64; i++){
        printf(" %2d", !check_bitmap(i));
    }
    printf("\n");
    for(int i = 1; i < 7; i++){
        printf("block %d / ", i);
        for(int j = 0; j < 4; j++){
            file_descriptor* description = start + 64 * i + j * sizeof(file_descriptor);
            printf("%2d %2d %2d %2d, ", description->length, description->content_0, description->content_1, description->content_2);
        }
        printf("\n");
    }
    for(int h = 7; h < 64; h++){

        if(h == get_fd_block_index(0, 0) || h == get_fd_block_index(0, 1) || h == get_fd_block_index(0, 2)){
            printf("block %d\n", h);
            for(int k = 0; k < 64; k += 8){
                ((directoy_entry*) (start + h * 64 + k))->name[3] = '\0';

                printf("%3s %2d, ", ((directoy_entry*) (start + h * 64 + k))->name, ((directoy_entry*) (start + h * 64 + k))->index);
            }
            printf("\n");
        }else{
            char blank[64];
            memset(blank, 0, 64);
            if(memcmp(blank, start + h * 64, 64)){
                printf("block %d\n", h);
                for(int i = 0; i < 64; i++){
                    printf(" %2d", i);
                }
                printf("\n");
                for(int i = 0; i < 64; i++){
                    printf(" %2c", *(start + h * 64 + i));

                }
                printf("\n");
            }
        }

    }

}
int charToInt(char* str){
    char *endptr;
    errno = 0;
    long result = strtol(str, &endptr, 10);
    if (endptr == str)
    {
        // nothing parsed from the string, handle errors or exit
        return -1;
    }
    if ((result == LONG_MAX || result == LONG_MIN) && 
errno == ERANGE)
    {
        // out of range, handle or exit
        return -1;
    }
    return result;
}
int main() {
    bool initialized = false;
    while(true) {
        char *line = malloc(128);
        memset(line, 0, 128);
        char **args;
        int arg_count = 0;

        fgets(line, 128 , stdin);
        args = split_line(line, &arg_count);
        for(int i = 0; i < arg_count; i++){

        }
//        if(!(strcmp("cr", args[0]) == 0 || strcmp("de", args[0]) == 0 || strcmp("op", args[0]) == 0 || strcmp("cl", args[0]) == 0 || strcmp("rd", args[0]) == 0 || strcmp("sk", args[0]) == 0 || strcmp("dr", args[0]) == 0) || strcmp("in", args[0]) == 0 || strcmp("fr", args[0] == 0)){
//            printf("error\n");
//        }
//        else
        if(initialized == true &&
                strcmp("cr", args[0]) == 0 &&
                arg_count==2 &&
                strlen(args[1]) < 4){
            if( create(args[1]) != -1 ){
                printf("%s created\n", args[1]);
            }else{
                printf("error\n");
            }
        }
        else if(initialized == true && 
strcmp("de", args[0]) == 0 && 
arg_count==2 && 
strlen(args[1]) < 4){
            if( delete(args[1]) != -1 ){
                printf("%s deleted\n", args[1]);
            }else{
                printf("error\n");
            }
        }
        else if(initialized == true && 
strcmp("op", args[0]) == 0 && 
arg_count==2 && 
strlen(args[1]) < 4){
            int result = 0;
            if( result = open(args[1]) != -1 ){
                printf("%s opened %d\n", args[1], result);
            }else{
                printf("error\n");
            }
        }
        else if(initialized == true && 
strcmp("cl", args[0]) == 0 && 
arg_count==2 && 
(strcmp(args[1], "0") || strcmp(args[1], "1") || strcmp(args[1], "2") || strcmp(args[1], "3"))){
            int result = 0;
            if( result = close(args[1]) != -1 ){
                printf("%d closed\n", args[1], result);
            }else{
                printf("error\n");
            }
        }
        else if(initialized == true && 
strcmp("rd", args[0]) == 0 && 
arg_count==3 && 
(strcmp(args[1], "0") || strcmp(args[1], "1") || strcmp(args[1], "2") || strcmp(args[1], "3")) && 
charToInt(args[2]) > -1){
            char str[charToInt(args[3])];
            memset(str, 0, charToInt(args[3]));
            if (read(charToInt(args[1]), str, charToInt(args[3])) != -1)
                initialized == true &&("%s\n", str);
            else
                initialized == true &&("error\n");
        }
        else if(initialized == true && 
strcmp("wr", args[0]) == 0 && 
arg_count==4 && 
(strcmp(args[1], "0") || strcmp(args[1], "1") || strcmp(args[1], "2") || strcmp(args[1], "3")) && 
strlen(args[2]) == 1 && 
charToInt(args[3]) > -1){
            char c = args[2][0];
            char str[charToInt(args[3])];
            memset(str, c, charToInt(args[3]));
            int result = 0;
            if ( result = write(charToInt(args[1]), str, charToInt(args[3])) != -1 ){
                printf("%d bytes written\n", result);
            }else{
                printf("error\n");
            }
        }
        else if(initialized == true && 

strcmp("sk", args[0]) == 0 && 
arg_count==3 && 
(strcmp(args[1], "0") || strcmp(args[1], "1") || strcmp(args[1], "2") || strcmp(args[1], "3")) && 
charToInt(args[2]) > -1){
            int result = 0;
            if ( result = lseek(atoi(args[1]), charToInt(args[2])) != -1 ){
                printf("position is %d\n", result);
            }else{
                printf("error\n");
            }
        }
        else if(initialized == true && 
strcmp("dr", args[0]) == 0 && 
arg_count==1){
            directory();
        }
        else if(initialized == true && 
strcmp("sv", args[0]) == 0 && 
arg_count==2 && 
!strcmp(args[1], "")){
            close(0);
            close(1);
            close(2);
            close(3);
            int result = 0;
            if ( (result = save(args[1])) != -1 ){
                printf("%d bytes written\n", result);
            }else{
                printf("error\n");
            }
        }
        else if(strcmp("in", args[0]) == 0 && 
arg_count==2 && 
strcmp("", args[0]) != 0 ){
            int result = init(args[1]);
            if(result == -1)
                printf("error\n");
            if(result == 0) {
                printf("disk restored\n");
                initialized = true;
            }if(result == 1) {
                printf("disk initialized\n");
                initialized = true;
            }
        }
        else if(strcmp("in", args[0]) == 0 && 
arg_count==1){
            int result = init("");
            if(result == -1)
                printf("error\n");
            if(result == 0) {
                printf("disk restored\n");
                initialized = true;
            }if(result == 1) {
                printf("disk initialized\n");
                initialized = true;
            }
        }
        else if(strcmp("print", args[0]) == 0){
            print_ldisk();
        }
        else if(strcmp("exit", args[0]) == 0){
            break;
        }
        else{
            printf("error\n");
        }
        free(line);
        free(args);
    }

    return 0;
}