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

int current_indent = -1;

/* Receives a dirname (has to be a dir), assumes that cwd is this dir, and calculates its size */
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

    chdir(dirname);
    current_indent++;

    for (int i = 1; i < count + 1; ++i)
    {
        struct stat file_stat;
        char *filename = files[i - 1]->d_name;

        // inicio codigo duvidoso pra pular soft links

        struct stat link_stat;

        lstat(filename, &link_stat);
        if (S_ISLNK(link_stat.st_mode))
        {
            continue;
        }

        // fim codigo duvidoso

        stat(filename, &file_stat);

        printf("%*s[%s]\n", current_indent * 2, "", filename);

        if (S_ISDIR(file_stat.st_mode))
        {

            size += size_dir(filename);
        }
        else
        {
            size += file_stat.st_size;
        }
    }

    current_indent--;
    chdir("..");

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