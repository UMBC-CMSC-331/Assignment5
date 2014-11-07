/* Assignment 5
Authors: Eric Hebert, <insert names here>
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

// Holds any version of a 32-bit datagram
typedef struct {
	uint8_t version: 4;
	uint8_t type: 4;
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

// Function prototypes (TODO: comment what these do)
void test_sizes();
int read_file(char *filename);
int handle_datagram(datagram *dptr, FILE *fp);
void print_data(unsigned char *data, unsigned size);

int main(int argc, char **argv)
{
	test_sizes();

	int status = 0;
	if (argc == 2) {
		status = read_file(argv[1]);
		if (status < 0)
			printf("Error reading file '%s'\n", argv[1]);
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

int read_file(char *filename)
{
	int file_status = -1;
	FILE *fp = fopen(filename, "rb");
	if (fp) {
		datagram temp_datagram;
		int status = 0;
		// Read each datagram in the file, until STOP or EOF is reached
		while (!status) {
			unsigned bytes_read = fread(&temp_datagram, sizeof(datagram), 1, fp);
			if (bytes_read == sizeof(datagram))
				status = handle_datagram(&temp_datagram, fp);
			else
				break;
		}
		file_status = 0;
		fclose(fp);
	}
	return file_status;
}

int handle_datagram(datagram *dptr, FILE *fp)
{
	int status = 0;

	// TODO: Handle the extra stuff with each version
	//if (dptr->version == 1)...
	// Skip bit, dupe bit, and checksum

	/*
	TODO: Handle control instructions
		We will only need to read and print data in the first 5 cases
		Sadly, C has no template support, so this will be very redundant
	SKIP: Read the number and return it as the status
	BURN: ...
	STOP: Return -1
	*/
	
	// Get the length of the data in bytes
	uint8_t data_length = dptr->length - sizeof(datagram);

	// Allocate memory to read the data
	void* data_buffer = malloc(data_length);

	// Read the data from the file
	// Note: If the file is in little-endian format, we will need to specify the size
	unsigned bytes_read = fread(data_buffer, 1, data_length, fp);

	// Print the data
	// Note: We will also need to know the type for this
	print_data(data_buffer, bytes_read);

	// Free the memory
	free(data_buffer);

	return status;
}

void print_data(unsigned char *data, unsigned size)
{
	for (unsigned i = 0; i < size; ++i) {
		printf("%u ", (unsigned) data[i]);
	}
}
