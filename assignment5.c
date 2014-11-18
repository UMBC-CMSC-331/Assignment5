/* Assignment 5
Authors: Eric Hebert, Tyler Dallwig, Kendal Reed, Zain Syed
Usage:
    make
    ./assignment5 filename
*/

/*
TODO:
    Remove debugging code
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define HEADER_SIZE 4

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
#define PRINT_DATA(data, data_length, type, format) { \
        size_t num_elements = data_length / sizeof(type); \
        for (size_t i = 0; i < num_elements; ++i) { \
            printf(format, *((type*) data + i)); \
        } \
    }

// Holds any version of a 32-bit datagram
typedef struct {
    uint8_t version: 4;
    uint8_t type: 4;
    uint8_t length;

    union {
        // The skip bit exists in every version
        uint16_t skip_bit: 1;

        // Datagram version 2
        struct {
            // Including skip bit to make sure alignment is correct
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
void test_datagram();

// Reads a binary file, and prints its data
int read_file(const char *filename);

// Reads the data of a datagram, or handles the control instruction
int handle_datagram(datagram *dptr, FILE *fp, uint32_t *skips);

// Skips to the next datagram in the file, and returns the new status
int skip_datagram(FILE *fp, size_t data_length);

// Returns true if checksum is valid
bool valid_checksum(void *data);

// Prints data based on the numeric type value from the binary file
void print_data(uint8_t type, void *data, uint8_t data_length);

// BURN instruction
int handle_burn();

int main(int argc, char **argv)
{
    // test_sizes();
    // test_datagram();

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
    // printf("sizeof(my_data.data.version1) = %lu\n", sizeof(my_data.data.version1));
    printf("sizeof(my_data.data.version2) = %lu\n", sizeof(my_data.data.version2));
    printf("sizeof(my_data.data.version3) = %lu\n\n", sizeof(my_data.data.version3));
}

void test_datagram()
{
    datagram test_data = {0};
    test_data.data.skip_bit = 0;
    printf("Skip bit: %u\n", test_data.data.version3.skip_bit);
    test_data.data.skip_bit = 1;
    printf("Skip bit: %u\n", test_data.data.version3.skip_bit);
    test_data.data.version2.checksum = 97;
    printf("Checksum: %u\n", test_data.data.version3.checksum);
    test_data.data.version2.checksum = 0;
    printf("Checksum: %u\n", test_data.data.version3.checksum);
    test_data.data.version2.dupe_bit = 1;
    test_data.data.version2.skip_bit = 0;
    printf("Dupe bit: %u\n", test_data.data.version2.dupe_bit);
    printf("Skip bit: %u\n", test_data.data.version2.skip_bit);
    printf("\n");
}

int read_file(const char *filename)
{
    int file_status = -1;
    FILE *fp = fopen(filename, "rb");
    // FILE *fp = fopen(filename, "r+b"); // Used for fixing checksums
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
            else if (feof(fp)) {
                printf("Reached the end of the file.\n");
                status = STATUS_STOP;
            }
            else {
                printf("There was an error reading the file.\n");
                status = STATUS_FAIL;
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
    // printf("Version = %u, Type = %u, Length = %u\n", dptr->version, dptr->type, dptr->length);

    // Get the length of the data portion in bytes (not including the header)
    uint8_t data_length = dptr->length - sizeof(datagram);
    
    // Get the datagram type
    uint8_t type = dptr->type;

    if (*skips > 0 || dptr->data.skip_bit) {
        // Handle skipping N datagrams and the skip bit
        // printf("Skipping %u datagrams...\n", *skips);
        if (*skips > 0) {
            --(*skips);
        }
        return skip_datagram(fp, data_length);
    }

    // Make sure the version is valid
    if (dptr->version < 1 || dptr->version > 3) {
        printf("Invalid version: %u\n", dptr->version);
        return status;
    }

    // Normally, we only run things once
    int run_count = 1;

    // When the dupe bit is set, run things twice
    if (dptr->version == 2 && dptr->data.version2.dupe_bit) {
        run_count = 2;
    }

    if (dptr->version == 2 || dptr->version == 3) {
        // Skip the datagram if the checksum is invalid (ignoring the length)
        if (!valid_checksum(dptr)) {
            while (run_count--) {
                printf("Checksum is invalid!!!\n");
            }
            return status;
        }
    }

    if (dptr->length < sizeof(datagram)) {
        // Make sure the length is valid
        while (run_count--) {
            printf("Length is invalid: %u\n", dptr->length);
        }
        status = STATUS_FAIL;
    }
    else if ((type >= 0 && type <= 3) || type == 7) {
        // Handle datagram containing data
        
        // Allocate memory to read the data
        void *data = malloc(data_length);

        // Read the data from the file into the buffer
        size_t bytes_read = fread(data, 1, data_length, fp);

        if (bytes_read == data_length) {
            // Print the data read as the correct type
            while (run_count--) {
                print_data(type, data, data_length);
            }
        }
        else {
            while (run_count--) {
                printf("Read %lu/%u bytes of the datagram.\n", bytes_read, data_length);
            }
            status = STATUS_FAIL;
        }

        // Free the memory allocated for the data
        free(data);
    }
    else if (type == CONTROL_SKIP) {
        // Handle datagram containing SKIP instruction
        // Read the number of datagrams to skip from the file (store this number in skips)
        size_t bytes_read = fread(skips, 1, sizeof(*skips), fp);

        // Modify the skip value according to the run count.
        // (This doubles the skips if the dupe bit is set.)
        (*skips) *= run_count;

        // If the number could not be read, then fail
        if (bytes_read != sizeof(*skips)) {
            status = STATUS_FAIL;
        }
    }
    else if (type == CONTROL_BURN) {
        // Handle datagram containing BURN instruction
        while (run_count--) {
            status = handle_burn();
        }
    }
    else if (type == CONTROL_STOP) {
        // Handle datagram containing STOP instruction
        // Stop reading the file
        status = STATUS_STOP;
    }
    else if (type == TYPE_JUNK) {
        // Handle datagram containing junk data
        // Skip junk data
        status = skip_datagram(fp, data_length);
    }
    else {
        // Handle datagram with unrecognized type value
        printf("Unknown datagram type: %u\n", type);
        status = STATUS_FAIL;
    }

    return status;
}

int skip_datagram(FILE *fp, size_t data_length)
{
    // Go to the next datagram without reading any data
    if (fseek(fp, data_length, SEEK_CUR)) {
        return STATUS_FAIL;
    }
    return STATUS_CONTINUE;
}

bool valid_checksum(void *data)
{
    uint8_t total = 0;

    // Add up all of the bytes of the header
    for (unsigned i = 0; i < sizeof(datagram); ++i) {
        total += ((uint8_t *) data)[i];
    }

    // Total needs to be 0 for the checksum to be valid
    return (total == 0);
}

void print_data(uint8_t type, void *data, uint8_t data_length)
{
    // Depending on the type ID stored, print the data as a different type
    switch (type) {
        case TYPE_INT16:
            PRINT_DATA(data, data_length, int16_t, "%d");
            break;
        case TYPE_INT32:
            PRINT_DATA(data, data_length, int32_t, "%d");
            break;
        case TYPE_FLOAT32:
            PRINT_DATA(data, data_length, float, "%.7f");
            break;
        case TYPE_FLOAT64:
            PRINT_DATA(data, data_length, double, "%.15f");
            break;
        case TYPE_ASCII:
            PRINT_DATA(data, data_length, char, "%c");
            break;
    }
}

int handle_burn()
{
    printf("All system fans have been disabled, your CPU is now melting...\n");
    return STATUS_STOP;
}
