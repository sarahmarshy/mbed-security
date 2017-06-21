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

void return_error(int ret_val){
  if (ret_val)
    printf("Failure. %d\r\n", ret_val);
  else
    printf("done.\r\n");
}

void errno_error(void* ret_val){
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

int mount_fs(){
    printf("Mounting the filesystem on \"/fs\". ");
    int error = fs.mount(&bd);
    return_error(error);
    return error;
}

void init_fs(){
  if(mount_fs()){
      format_fs();
      mount_fs();
  }
}

void take_and_store_photo() {
    printf("Taking photo\r\n");
    camera.take_picture();
    printf("Took photo\r\n");
    uint32_t size = camera.get_picture_size();
    uint8_t* data_buff = (uint8_t*)malloc(size);
    uint32_t bytes_read = camera.read_picture_data(data_buff, size);

    char filename[15];
    sprintf(filename, "/fs/img%d.jpg", num++);
    printf("Opening a new file, %s\r\n", filename);
    FILE* fd = fopen(filename, "wb");
    errno_error(fd);
    fwrite(data_buff, sizeof(uint8_t), bytes_read, fd);
    fclose(fd);
    printf("file written\r\n");
    free(data_buff);
}

int main(){
    camera.powerup();
    init_fs();
    num = 0;
    queue.call_every(10000, take_and_store_photo);
    queue.dispatch();
    while(1) {
        wait(0.2);
    }
}
