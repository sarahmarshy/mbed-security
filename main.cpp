#include "CameraOV528.h"
#include "mbed.h"
#include "FATFileSystem.h"
#include "SDBlockDevice.h"
#include <stdio.h>
#include <errno.h>

CameraOV528 camera(D1, D0);
SDBlockDevice bd(PTE3, PTE1, PTE2, PTE4);
FATFileSystem fs("fs");
EventQueue queue;
int num;
char dirname[16];
DigitalOut led2(LED2);

void return_error(int ret_val) {
  if (ret_val)
    printf("Failure. %d\r\n", ret_val);
  else
    printf("done.\r\n");
}

void errno_error(void* ret_val) {
  if (ret_val == NULL)
    printf(" Failure. %d \r\n", errno);
  else
    printf(" done.\r\n");
}

int format_fs(){
    int error = 0;
    printf("Welcome to the filesystem example.\r\n"
           "Formatting a FAT, RAM-backed filesystem. ");
    error = FATFileSystem::format(&bd);
    return_error(error);
    return error;
}

int mount_fs() {
    printf("Mounting the filesystem on \"/fs\". ");
    int error = fs.mount(&bd);
    return_error(error);
    return error;
}

void init_fs() {
  if(mount_fs()) {
      format_fs();
      mount_fs();
  }
}

void take_and_store_photo() {
    printf("Taking photo\r\n");
    if(camera.take_picture() == 0) {
        printf("Took photo\r\n");
        uint32_t size = camera.get_picture_size();
        uint8_t* data_buff = (uint8_t*)malloc(size);
        uint32_t bytes_read = camera.read_picture_data(data_buff, size);

        char filename[30];
        sprintf(filename, "/fs/%s/img%d.jpg", dirname, num++);
        printf("Opening a new file, %s\r\n", filename);
        FILE* fd = fopen(filename, "wb");
        errno_error(fd);
        fwrite(data_buff, sizeof(uint8_t), bytes_read, fd);
        fclose(fd);
        printf("file written\r\n");
        free(data_buff);
    }
    else {
        printf("Failed to take photo\r\n");
    }
}

// Create a new directory with each power cycle
void create_dir() {
    int dir_num = 0;
    char temp_dir[7];
    sprintf(temp_dir, "dir%d", dir_num);
    printf("Trying to open %s\r\n", temp_dir);
    DIR* dir = fs.opendir(temp_dir);
    return_error(errno);
    if (dir == NULL) {
        printf("Creating %s\r\n", temp_dir);
        fs.mkdir(temp_dir, O_RDWR);
        return_error(errno);
        strcpy(dirname, temp_dir);
    }
    else {
        while(1) {
            memset(temp_dir, 0, sizeof temp_dir);
            sprintf(temp_dir, "dir%d", ++dir_num);
            printf("Trying to open %s\r\n", temp_dir);
            DIR* dir = fs.opendir(temp_dir);
            if (dir == NULL) {
                break;
            }
        }
        printf("Creating %s\r\n", temp_dir);
        int err  = fs.mkdir(temp_dir, O_RDWR);
        return_error(err);
        strcpy(dirname, temp_dir);
    }
}

void blink() {
    led2 = !led2;
}

int main() {
    camera.powerup();
    init_fs();
    create_dir();
    num = 0;
    take_and_store_photo();
    queue.call_every(600000, take_and_store_photo);
    queue.call_every(500, blink);
    queue.dispatch();
    while(1) {
        wait(0.2);
    }
}
