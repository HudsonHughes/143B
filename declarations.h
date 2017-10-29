//
// Created by VGDC on 10/26/2017.
//

#ifndef UNTITLED_DECLARATIONS_H
#define UNTITLED_DECLARATIONS_H

typedef struct oft_slot{
    int index;
    int pos;
    char buffer[64];
    int length;
}oft_slot;
typedef struct file_descriptor{
    int length;
    int content_0;
    int content_1;
    int content_2;
} file_descriptor;
typedef struct directoy_entry{
    char name[4];
    int index;
} directoy_entry;
char ldisk[64 * 64];
char datadisk[(64 * 64) - (64 * 7)];
int buffer[16];
char *data = ldisk + 64 * 7;
int *bitmap;
file_descriptor *descriptors;
oft_slot oft[4];
char **split_line(char *line, int* arg_count);
void allocate_bitmap(int index);
bool check_bitmap(int index);
int close(int index);
int create(char* symbolic_file_name);
void deallocate_bitmap(int index);
int delete(char* symbolic_file_name);
int directory();
int disk_exist(char *filename);
void fill_oft_buffer(int oft_index, int new_block_index);
int get_fd_block_index(int fd_index, int block_index);
int get_fd_size(int fd_index);
int get_free_descriptor();
int get_number(char* pointer);
int init(char* filename);
int lseek(int index, int new_pos);
int main();
void microwave_disk();
int open(char* symbolic_file_name);
void print_ldisk();
void printBits(size_t const size, void const * const ptr);
void put_number(void* pointer, int value);
void read_block(int i, char *p);
int read(int index, char* mem_area, int count);
int save(char* filename);
void set_fd_block_index(int fd_index, int block_index, int value);
void set_fd_size(int fd_index, int new_size);
void update_oft_buffer(int oft_index, int old_block_index, int new_block_index);
bool upgrade_oft_buffer(int oft_index, int old_block_index);
void wipe_descriptor(int index);
void write_block(int i, char *p);
int write(int index, char* mem_area, int count);
#endif //UNTITLED_DECLARATIONS_H
