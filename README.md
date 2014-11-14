Assignment 5
============

Instructions from Blackboard:

Due Nov 18
----------

Thirty years ago, I wrote a program that stored data in a compact binary format. I now wish to retrieve that data. Sadly, I no longer have the codec program that I wrote to work with this data format. I do, however, have the spec.

Write a program that will take a filename as an argument. It will then open that file and start reading data in a specific format. It should then read all valid "data" fields and print them to STDOUT.

File Format
-----------

This file is a binary format, made up of datagrams. A datagram is variable in size, but will always be aligned to bytes. Extract datagrams in order until the end of the file is reached, or a STOP control instruction is encountered. Skip all datagrams that have a checksum that is invalid.

Each Data field in a valid, unskipped datagram should be printed to STDOUT. Integers and floats should be printed as such.

![alt tag](https://dl.dropboxusercontent.com/u/10528991/assignment5_images/file_format.png)

Data Definition
---------------

Version

* unsigned integer indicating datagram version (1, 2, etc.)

Type

![alt tag](https://dl.dropboxusercontent.com/u/10528991/assignment5_images/type_of_data.png)

Size

* unsigned integer indicating total size of datagram, in bytes (including header)

S

* SKIP bit. Do not process this datagram if set.

D

* DUPE bit. Process this datagram twice.

ID

* Unique ID of the datagram.

Checksum

* unsigned 8-bit checksum of the header

Data

* The actual data

### Control Instruction

If the datagram type indicates a control instruction, the Data section encodes the special control instruction. Suppress normal output for that data section; instead do:

SKIP

* The next N datagrams should be skipped, where N is the 32-bit unsigned integer value encoded in the Data field.

BURN

* The computer should halt and catch fire.

STOP

* All data have been read, stop extraction.

Suggested Languages
-------------------

Excellent Choice

* C, C++, Perl

Good Choice

* Java, C++

Marginal Choice

* FORTRAN, COBOL
