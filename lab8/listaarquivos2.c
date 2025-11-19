#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FALSE 0
#define TRUE 1

int file_select(struct direct *entry)
{
    if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        return (FALSE);
    else
        return (TRUE);
}

extern int alphasort();
char pathname[MAXPATHLEN];

int main()
{
    int count, i;
    struct direct **files;
    int file_select();

    if (getcwd(pathname, MAXPATHLEN) == NULL)
    {
        printf("Error getting path\n");
        exit(0);
    }
    printf("Current Working Directory = %s\n", pathname);
    count = scandir(pathname, &files, file_select, alphasort);

    /* If no files found, make a non-selectable menu item */
    if (count <= 0)
    {
        printf("No files in this directory\n");
        exit(0);
    }
    printf("Number of files = %d\n", count);

    time_t current_time = time(NULL);
    for (i = 1; i < count + 1; ++i) {
        struct stat file_stat;
        stat(files[i - 1]->d_name, &file_stat);
        printf("%s \t", files[i - 1]->d_name);
        printf("inode: %lu \t", file_stat.st_ino);
        printf("size: %ld \t", file_stat.st_size);
        printf("age: %ld days \t", (current_time - file_stat.st_mtime) / (60 * 60 * 24));
        printf("links: %lu", file_stat.st_nlink);
        printf("\n");
    }

    return 0;
}