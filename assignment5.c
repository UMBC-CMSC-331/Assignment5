#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

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

int read_file(char *filename);

int main(int argc, char **argv)
{
	// Make sure sizes are good
	datagram my_data;
	printf("sizeof(my_data) = %lu\n", sizeof(my_data));
	printf("sizeof(my_data.data) = %lu\n", sizeof(my_data.data));
	printf("sizeof(my_data.data.version1) = %lu\n", sizeof(my_data.data.version1));
	printf("sizeof(my_data.data.version2) = %lu\n", sizeof(my_data.data.version2));
	printf("sizeof(my_data.data.version3) = %lu\n", sizeof(my_data.data.version3));

	int status = 0;
	if (argc == 2) {
		status = read_file(argv[1]);
	}
	else {
		printf("Usage: ./%s filename\n", argv[0]);
		status = 1;
	}
	return status;
}

int read_file(char *filename)
{
	return 1;
}
