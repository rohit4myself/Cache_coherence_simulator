# Cache_coherence_simulator
CONTENTS OF THIS FILE
---------------------

 * Introduction
 * Input File
 * Running the program 
 * Output File
 * Contact Details


INTRODUCTION
------------

Hi!! This is the Assignment #3 submission of course CS622: Advance Computer Architecture
This assignment is coded in C++ 

* Folder contains 1 cpp file and 1 txt file
	* addrtrace.cpp :- Prints the total machine access prepared on the terminal and generates addrtrace.txt

	* assign3.cpp:- It prints the following details on the terminal and also generates "output.csv". 
                       • Number of simulated cycles
                       • Number of L1 cache accesses and misses
                       • Number of L2 cache misses
                       • Names counts of all messages received by the L1 caches.
                       • Names counts of all messages received by the L1 caches.

	

INPUT FILE
-------------
* prog1.cpp, prog2.cpp, prog3.cpp, prog4.cpp 


RUNNING THE PROGRAM
----------------

Compile these programs to generate static binaries. Use the following commands.
gcc -O3 -static -pthread prog1.c -o prog1
gcc -O3 -static -pthread prog2.c -o prog2
gcc -O3 -static -pthread prog3.c -o prog3
gcc -O3 -static -pthread prog4.c -o prog4

For generating total machine access details on terminal:
-> Compile pintool: $ make obj-intel64/addrtrace.so
-> Launch pin: $ ../../../pin -t obj-intel64/addrtrace.so -- ./prog1 8
* Output : Printed on terminal and generates addrtrace.txt

------------------------
File name: assign.cpp
Input required: addrtrace.txt
Output: Printed on terminal and a csv file output.csv will be generated
Command to execute the file:
	$ g++ assign3.cpp -o assign
	$ ./assign



Output File 
-------------------------
An output file "output.csv" will be generated.
