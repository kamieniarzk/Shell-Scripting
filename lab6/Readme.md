# Copy
This program's function is to copy files. It copies the content of the file with name specified in 
`[input_file]` argument into the file with name `[output_file]`
Two methods of copying are available:
- using read and write functions from `<unistd.h>` - no option needed, default,
- using mmap and memcpy - option `[-m]` required. <br> <br>
Usage: 
- `./copy [-m] [input_file] [output_file]`
- `./copy [-h]`
## Functions
### Main
The main function starts with a loop which acquires the command line arguments from the `argc` and `*argv[]` variables. The loop is 
followed by a check of number of supplied arguments. If the number is incorrect, help is displayed and the program returns 1 (fail).
On success, the program execution continues and there is an attempt on opening the files with the names specified in the arguments.
If the `input_file` opens, it's status is then acquired using `fstat` so that it can be suplemented for the `output_file`. 
If all of the above steps succeeded, one of the copy functions (depending on the `-m` option flag) is now called using the file descriptors returned from `open()`. 
### copy_read_write
This function takes two parameters - `fd_from` and `fd_to` which are file descriptors of source and destination files of copy. The function
runs a loop in which at first the input file content is read into a buffer after which it is written from this buffer into the output file.
The functions used are `read()` and `write()` fron `<unistd.h>`.
### copy_mmap
Similarly as `copy_read_write` it takes two file descriptors as its arguments. The function at first tries to acquire the status of the input 
file using `fstat` which will be then used in the `mmap` function. On success it tries to map the content of the input file to the program memory
and acquire a pointer to that memory. Then the output file's size needs to be truncated using `ftruncate` so that it mathes the size of the input.
If that succeeds, the content of output file is mapped to the memory using `mmap` (size suplemented from the status of input). If both files
are mapped successfuly, the memory mapped to input file is copied into the memory mapped to output_file using `memcpy`.
