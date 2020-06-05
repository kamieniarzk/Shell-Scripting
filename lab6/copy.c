#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define BUFFER 4096

void help();
int wrong_args();
int copy_read_write(int fd_from, int fd_to);
int copy_mmap(int fd_from, int fd_to);

int main(int argc, char *argv[])
{
    int fd_from, fd_to; // file descriptors
    int option;         // temp for storing getopt result
    bool m = false;     // -m option

    // Getting option arguments
    while ((option = getopt(argc, argv, "mh")) != -1)
    {
        switch (option)
        {
        case 'm':
            m = true;
            break;
        case 'h':
            help();
            return 0;
        case '?':
            printf("Wrong option provided.\n");
            help();
            return 1;
        }
    }

    if (m && argc != 4)
        return wrong_args();
    else if (!m && argc != 3)
        return wrong_args();

    char *input_file = argv[1];
    char *output_file = argv[2];

    if (m)
    {
        input_file = argv[2];
        output_file = argv[3];
    }

    // input_file open
    fd_from = open(input_file, O_RDONLY);
    if (fd_from < 0)
    {
        perror("error while opening the input_file ");
        return 1;
    }

    // acquiring the input_file status
    struct stat input_status;
    if (fstat(fd_from, &input_status) < 0)
    {
        perror("error while acquiring the status of input_file ");
        return 1;
    }

    // output_file open
    fd_to = open(output_file, O_RDWR | O_CREAT, input_status.st_mode);
    if (fd_to < 0)
    {
        perror("error while opening the output_file ");
        return 1;
    }

    int retv = m ? copy_mmap(fd_from, fd_to) : copy_read_write(fd_from, fd_to);
    close(fd_from);
    close(fd_to);
    return retv;
}

void help()
{
    printf("This program's function is to copy files.\n");
    printf("It copies the content of the file with name specified\n");
    printf("in [input_file] argument into the file with name [output_file]\n");
    printf("Two methods of copying are available:\n");
    printf("- using read and write functions from <unistd.h> - no option needed, default,\n");
    printf("- using mmap and memcpy - option [-m] required.\n\n");
    printf("Usage:\n");
    printf("\t./copy [-m] [input_file] [output_file]\n");
    printf("\t./copy [-h]\n");
}

int wrong_args()
{
    printf("Incorrect number of arguments provided.\n\n");
    help();
    return (1);
}

int copy_read_write(int fd_from, int fd_to)
{
    char buffer[BUFFER];
    int count_from, count_to;

    // reading from input_file into buffer until end of file or error
    while ((count_from = read(fd_from, buffer, BUFFER)) > 0)
    {   
        // writing into output_file from the buffer
        count_to = write(fd_to, buffer, count_from);
        if (count_to == -1 || count_from == -1)
        {
            perror("error in copy_read_write() ");
            return 1;
        }
    }

    return 0;
}

int copy_mmap(int fd_from, int fd_to)
{
    // acquiring input_file status
    struct stat input_status;
    if (fstat(fd_from, &input_status) == -1)
    {
        perror("error while acquiring the status of input_file ");
        return 1;
    }

    // acquiring pointer to mapped input_file memory
    char *buff_from = mmap(NULL, input_status.st_size, PROT_READ, MAP_SHARED, fd_from, 0);
    if (buff_from == (void *)-1)
    {
        perror("error while mapping input_file memory ");
        return 1;
    }

    // setting the output_file size to match input_file with ftruncate
    if (ftruncate(fd_to, input_status.st_size) < 0)
    {
        perror("error while setting the output file size ");
        return 1;
    }

    // acquiring pointer to mapped input_file memory
    char *buff_to = mmap(NULL, input_status.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_to, 0);
    if (buff_to == (void *)-1)
    {
        perror("error while mapping output_file memory");
        return 1;
    }

    // copying the memories from the mapped locations
    if (memcpy(buff_to, buff_from, input_status.st_size) == (void *)-1)
    {
        perror("error while copying the mapped memory");
        return 1;
    }

    return 0;
}