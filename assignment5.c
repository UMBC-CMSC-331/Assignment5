/* Assignment 5
Authors: Eric Hebert, Tyler Dallwig, Kendal Reed, Insert Other Names here
Usage:
    make
    ./assignment5 filename
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// Valid values for the "type" field
#define TYPE_INT16 0
#define TYPE_INT32 1
#define TYPE_FLOAT32 2
#define TYPE_FLOAT64 3
#define TYPE_ASCII 7
#define TYPE_JUNK 8
#define CONTROL_SKIP 9
#define CONTROL_BURN 10
#define CONTROL_STOP 11

// Return values for handle_datagram
#define STATUS_CONTINUE 0
#define STATUS_STOP -1
#define STATUS_FAIL -2

// Macro for printing different types of data
#define PRINT_DATA(data, data_length, type_str, type, format) { \
        size_t num_elements = data_length / sizeof(type); \
        printf("Printing %lu element%s of type %s:\n", num_elements, (num_elements == 1 ? "" : "s"), type_str); \
        for (size_t i = 0; i < num_elements; ++i) { \
            printf(format, *((type*) data + i)); \
        } \
        printf("\n"); \
    }

// Holds any version of a 32-bit datagram
typedef struct {
    uint8_t type: 4;
    uint8_t version: 4;
    uint8_t length;

    union {
        // Datagram version 1
        struct {
            uint16_t skip_bit: 1;
        } version1;

        // Datagram version 2
        struct {
            uint8_t skip_bit: 1;
            uint8_t dupe_bit: 1;
            uint8_t checksum;
        } version2;

        // Datagram version 3
        struct {
            uint8_t skip_bit: 1;
            uint8_t id: 7;
            uint8_t checksum;
        } version3;
    } data;
} datagram;

// Prints sizeof values of the datagram structure's fields
void test_sizes();

// Reads a binary file, and prints its data
int read_file(const char *filename);

// Reads the data of a datagram, or handles the control instruction
int handle_datagram(datagram *dptr, FILE *fp, uint32_t *skips);

void print_data(uint8_t type, void *data, uint8_t data_length);

int main(int argc, char **argv)
{
    int status = 0;
    if (argc == 2) {
        status = read_file(argv[1]);
        if (status < 0) {
            printf("Error reading file '%s'\n", argv[1]);
        }
    }
    else {
        printf("Usage: %s filename\n", argv[0]);
        status = 1;
    }
    return status;
}

void test_sizes()
{
    // Make sure sizes are good (can remove this later)
    datagram my_data;
    printf("sizeof(my_data) = %lu\n", sizeof(my_data));
    printf("sizeof(my_data.data) = %lu\n", sizeof(my_data.data));
    printf("sizeof(my_data.data.version1) = %lu\n", sizeof(my_data.data.version1));
    printf("sizeof(my_data.data.version2) = %lu\n", sizeof(my_data.data.version2));
    printf("sizeof(my_data.data.version3) = %lu\n", sizeof(my_data.data.version3));
}

int read_file(const char *filename)
{
    int file_status = -1;
    FILE *fp = fopen(filename, "rb");
    if (fp) {
        datagram temp_datagram;
        int status = STATUS_CONTINUE;
        uint32_t skips = 0;
        // Read each datagram in the file, until STOP or EOF is reached
        while (status == STATUS_CONTINUE) {
            size_t bytes_read = fread(&temp_datagram, 1, sizeof(datagram), fp);
            // printf("Read %lu bytes (datagram header).\n", bytes_read);
            if (bytes_read == sizeof(datagram)) {
                status = handle_datagram(&temp_datagram, fp, &skips);
            }
            else {
                status = STATUS_STOP;
            }
        }
        file_status = 0;
        fclose(fp);
        if (status == STATUS_FAIL) {
            printf("An error occurred while processing \"%s\".\n", filename);
        }
    }
    return file_status;
}

int handle_datagram(datagram *dptr, FILE *fp, uint32_t *skips)
{
    int status = STATUS_CONTINUE;
    uint8_t dupe = 0;
    // printf("Version = %u, Type = %u, Length = %u\n", dptr->version, dptr->type, dptr->length);

    /*
    Before reading data:
        if version 2:
            handle dupe bit
        if version 3:
            do something with ID?
    After reading data:
        if version 2 or 3:
            make sure checksum is equal
    */
    
    // handle skip bit
    if (dptr->data.version1.skip_bit) {
        // Do nothing- don't print, don't change status, don't decrement skips.
        return status;
    }
    
    if (dptr->version == 2) {
        // save dupe bit to use in processing
        dupe = dptr->data.version2.dupe_bit;
    }
    
    if (dptr->version == 3) {
        // handle ID
    }
    
    if (dptr->version == 2 || dptr->version == 3) {
        // verify checksum
    }

    // Get the length of the data in bytes, and the type
    uint8_t data_length = dptr->length - sizeof(datagram);
    uint8_t type = dptr->type;

    // Handle skipping datagrams
    if (*skips > 0) {
        printf("Skipping %u datagrams...\n", *skips);
        // Decrement counter for remaining skips
        --(*skips);
        // Go to the next datagram without reading any data
        if (fseek(fp, data_length, SEEK_CUR)) {
            status = STATUS_FAIL;
        }
    }
    else if ((type >= 0 && type <= 3) || type == 7) {
        // Allocate memory to read the data
        void *data = malloc(data_length);

        // Read the data from the file into the buffer
        size_t bytes_read = fread(data, 1, data_length, fp);

        // printf("Read %lu bytes (datagram data).\n", bytes_read);

        if (bytes_read == data_length) {
            print_data(type, data, data_length);
            // If dupe is set to 1, print the data again:
            if (dupe) {
                print_data(type, data, data_length);
            }
        }
        else {
            status = STATUS_FAIL;
        }

        // Free the memory allocated for the data
        free(data);
    }
    else if (type == CONTROL_SKIP) {
        printf("CONTROL_SKIP\n");
        // Read the number of datagrams to skip from the file (store this number in skips)
        size_t bytes_read = fread(skips, 1, sizeof(*skips), fp);
        
        // If dupe is 1, repeat the print statement and double the skip value.
        if (dupe) {
            printf("CONTROL_SKIPS\n");
            (*skips) *= 2;
        }

        // If the number could not be read, then fail
        if (bytes_read != sizeof(*skips)) {
            status = STATUS_FAIL;
        }
    }
    else if (type == CONTROL_BURN) {
        printf("All system fans have been disabled, your CPU is now melting...\n");
        if (dupe) {
            printf("All system fans have been disabled, your CPU is now melting...\n");
        }
        status = STATUS_STOP;
    }
    else if (type == CONTROL_STOP) {
        printf("CONTROL_STOP\n");
        if (dupe) {
            printf("CONTROL_STOP\n");
        }
        status = STATUS_STOP;
    }
    else if (type == TYPE_JUNK) {
        printf("Skipping %u bytes of junk data...\n", data_length);
        if (dupe) {
            printf("Skipping %u bytes of junk data...\n", data_length);
        }
        status = STATUS_CONTINUE;
        // Go to the next datagram without reading any data
        if (fseek(fp, data_length, SEEK_CUR)) {
            status = STATUS_FAIL;
        }
    }
    else {
        printf("Unknown datagram type: %u\n", type);
        status = STATUS_FAIL;
    }

    return status;
}

void print_data(uint8_t type, void *data, uint8_t data_length)
{
    // String names of types
    static const char *TYPE_NAMES[] = {"INT16", "INT32", "FLOAT32", "FLOAT64", "", "", "", "ASCII"};

    // Get the string name of the type
    const char *type_str = TYPE_NAMES[type];

    switch (type) {
        case TYPE_INT16:
            PRINT_DATA(data, data_length, type_str, int16_t, "%d\n");
            break;
        case TYPE_INT32:
            PRINT_DATA(data, data_length, type_str, int32_t, "%d\n");
            break;
        case TYPE_FLOAT32:
            PRINT_DATA(data, data_length, type_str, float, "%.7f\n");
            break;
        case TYPE_FLOAT64:
            PRINT_DATA(data, data_length, type_str, double, "%.15f\n");
            break;
        case TYPE_ASCII:
            PRINT_DATA(data, data_length, type_str, char, "%c");
            printf("\n");
            break;
    }
}