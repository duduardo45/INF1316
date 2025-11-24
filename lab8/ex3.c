#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FALSE 0
#define TRUE 1

int file_select(const struct direct *entry)
{
    if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        return (FALSE);
    else
        return (TRUE);
}

extern int alphasort();
char pathname[MAXPATHLEN];

/* Receives a dirname (has to be a dir), assumes that dirname is accessible from cwd, and calculates its size */
long size_dir(char *dirname)
{
    long size = 0;

    struct direct **files;
    int count = scandir(dirname, &files, file_select, alphasort);

    if (count < 0)
    {
        printf("Não consegui dar scandir\n");
        exit(EXIT_FAILURE);
    }

    if (chdir(dirname) < 0)
    {
        printf("Não consegui dar chdir\n");
        exit(EXIT_FAILURE);
    };

    for (int i = 1; i < count + 1; ++i)
    {
        struct stat file_stat;
        char *filename = files[i - 1]->d_name;

        stat(filename, &file_stat);

        if (S_ISDIR(file_stat.st_mode))
        {

            size += size_dir(filename);
        }
        else
        {
            size += file_stat.st_size;
        }
    }

    if (chdir("..") < 0)
    {
        printf("Não consegui dar chdir\n");
        exit(EXIT_FAILURE);
    };

    return size;
}

int main()
{
    if (getcwd(pathname, MAXPATHLEN) == NULL)
    {
        printf("Error getting path\n");
        exit(0);
    }
    printf("Current Working Directory = %s\n", pathname);

    long total_size = size_dir(pathname);

    printf("Size is: %ld\n", total_size);

    return 0;
}