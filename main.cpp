#include "CameraOV528.h"
#include "mbed.h"
#include "RangeFinder.h"
#include "mbed.h"
#include "FATFileSystem.h"
#include "SDBlockDevice.h"
#include <stdio.h>
#include <errno.h>

CameraOV528 camera(D1, D0);
RangeFinder rf(D5, 10, 5800.0, 100000);
SDBlockDevice bd(PTE3, PTE1, PTE2, PTE4);
FATFileSystem fs("fs");

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

float calibrate_rf(){
    float closest_dist = rf.read_m();
    for(int i = 0 ; i < 5; i++){
        float d = rf.read_m();
        if (d == -1.0 || d > 5.0)  {
            continue;
        } else  {
            closest_dist = d;
        }
        wait_ms(100);
    }
    return closest_dist;
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

void take_and_store_photo(int num) {
    printf("Taking photo\r\n");
    camera.take_picture();
    printf("Took photo\r\n");
    uint32_t size = camera.get_picture_size();
    uint8_t* data_buff = (uint8_t*)malloc(size);
    uint32_t bytes_read = camera.read_picture_data(data_buff, size);

    char filename[15];
    sprintf(filename, "/fs/img%d.jpg", num);
    printf("Opening a new file, %s\r\n", filename);
    FILE* fd = fopen(filename, "wb");
    errno_error(fd);
    fwrite(data_buff, sizeof(uint8_t), bytes_read, fd);
    fclose(fd);
    printf("file written\r\n");
    free(data_buff);
}

int main(){
    printf("Calibrating...\r\n");
    float init_dist = calibrate_rf();
    camera.powerup();
    printf("initial dist: %f\r\n", init_dist);
    init_fs();
    int i = 0;
    while(1) {
        float d = rf.read_m();
        if ((init_dist - d) >= 0.01 && d != -1.0 && d <= 5.0) {
            printf("Object close! %f\r\n", d);
            take_and_store_photo(i++);
        }
        wait_ms(100);
    }
}
