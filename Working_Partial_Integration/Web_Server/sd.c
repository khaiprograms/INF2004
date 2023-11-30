#include <stdio.h>
#include "pico/stdlib.h"
#include "sd_card.h"
#include "ff.h"
#include <string.h>
#include <stdlib.h>

void sd_card_init() 
{
    // Initialize SD card
    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
        while (true);
    }
}

void write_sd_card()
{
    FRESULT fr;
    FIL fil;
    int ret;
    char filename[] = "test02.txt";
    // Open file for writing ()
    fr = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        while (true);
    }

    // Write something to file
    ret = f_printf(&fil, "This is another test\r\n");
    if (ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        while (true);
    }
    ret = f_printf(&fil, "of writing to an SD card.\r\n");
    if (ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        while (true);
    }

    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        while (true);
    }
}

char *read_sd_card(char *filename)
{
    FRESULT fr;
    FATFS fs;
    FIL fil;

    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
        while (true);
    }

    // Print every line in file over serial
    printf("This is file '%s':\r\n", filename);
    // Open file for reading
    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        while (true);
    }

    // Get the size of the file
    UINT fileSize = f_size(&fil);

    // Allocate memory for the file content
    char *file_content = malloc(fileSize + 1); // +1 for the null terminator
    if (file_content == NULL) {
        printf("ERROR: Out of memory\r\n");
        f_close(&fil);
        return NULL;
    }

    // Read the entire file content
    fr = f_read(&fil, file_content, fileSize, NULL);
    if (fr != FR_OK) {
        printf("ERROR: Could not read file (%d)\r\n", fr);
        free(file_content); // Free memory before returning
        f_close(&fil);
        return NULL;
    }

    // Null-terminate the file content
    file_content[fileSize] = '\0';

    // Print the file content
    printf("%s\n", file_content);

    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        free(file_content); // Free memory before returning
        while (true);
    }

    return file_content;
}

void remove_sd_card()
{
    // Unmount drive
    f_unmount("0:");
}