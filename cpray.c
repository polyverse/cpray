#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define GetCurrentDir getcwd

enum MODE {
    NONE,
    CHANGE,
    DRYRUN
};

void changedir(char* rootdir, enum MODE mode);
void changefile(char* rootdir, enum MODE mode);
int is_C_file(char* filename);
int endswith(const char *str, const char *suffix);

int main(int argc, char* argv[]) {
    int i;
    enum MODE mode = NONE;
    char rootdir[FILENAME_MAX];

    // get root directory
    GetCurrentDir( rootdir, FILENAME_MAX );

    printf("cpray will iterate over ALL C/C++ files under: %s\n", rootdir);

    for (i = 1; i < argc; i++) {
        if (0 == strcmp(argv[i], "--change")) {
            mode = CHANGE;
        } else if (0 == strcmp(argv[i], "--dryrun")) {
           mode = DRYRUN;
        }
    }

    if (mode == NONE) {
        printf("Since cpray makes potentially irreversible and wide changes, by default it will do nothing. \n");
        printf("To perform a dry run, use the flag: --dryrun\n");
        printf("To apply changes, use the flag: --change\n");
        return 0;
    }

    changedir(rootdir, mode);
}

void changedir(char* sourcedir, enum MODE mode) {
    printf("Changing under directory: %s\n", sourcedir);

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (sourcedir)) != NULL) {
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL) {
        char entrypath[FILENAME_MAX];
        snprintf(entrypath, FILENAME_MAX, "%s/%s", sourcedir, ent->d_name);
        if (DT_REG == ent->d_type && 1 == is_C_file(ent->d_name)) {
            printf ("Processing file: %s\n", entrypath);
            changefile(entrypath, mode);
        } else if (DT_DIR == ent->d_type && 0 != strcmp(ent->d_name, ".") && 0 != strcmp(ent->d_name, "..")) {
            printf("Recursing into directory: %s", entrypath);
            changedir(entrypath, mode);
        }
      }
      closedir (dir);
    } else {
      /* could not open directory */
      printf("Unable to open directory %s. Errno: %d. Errstring: %s\n", sourcedir, errno, strerror(errno));
      return;
    }
}

int is_C_file(char* filename) {
    return endswith(filename, ".cpp") || endswith(filename, ".c");
}

int endswith(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void changefile(char* file, enum MODE mode) {
    FILE *fp;
    long int size = 0;
    char* standardheader = "#include <stdio.n>\n";

    fp = fopen(file, "r");
    if (fp == NULL) {
        printf("Error opening file for reading: %s. Errno: %d. Error string: %s\n", file, errno, strerror(errno));
        return;
    }

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    char contents[size+1]; //allocate some bytes to read the file into (extra room for terminating NULL)
    fread(contents, 1, size, fp);
    fclose(fp);

    printf("File size: %ld\n", size);

    contents[size] = 0; //Null-terminate the string

    long int capacity = size*10;
    long int allocated = 0;
    char* instrumented = (char*)malloc(sizeof(char) * capacity);

    if (NULL != strstr(contents, "stdio.h")) {
        strcpy(instrumented, standardheader);
        allocated = strlen(standardheader);
    }


    int lineno=2;

    #define PRINTFBUFSIZE 10000
    char printfbuf[PRINTFBUFSIZE];
    for (int i = 0; i < size; i++) {
        instrumented[allocated] = contents[i];
        allocated++;


        if (contents[i] == '{') {
            snprintf(printfbuf, PRINTFBUFSIZE, "\nprintf(\"cpray,%s,%d\");\n", file, lineno);
            strcpy(instrumented+allocated, printfbuf);
            allocated += strlen(printfbuf);
        } else if (contents[i] == '\n') {
            lineno++;
        }

        if (allocated > capacity) {
            printf("Allocated new file exceeded the buffered capacity: %s\n", file);
        }
    }
    instrumented[allocated] = 0; //NULL terminate the string

    if (mode == CHANGE) {
        fp = fopen(file, "w");
        if (fp == NULL) {
            printf("Error opening file for reading: %s. Errno: %d. Error string: %s\n", file, errno, strerror(errno));
            return;
        }
        fwrite(instrumented, 1, allocated, fp);
        fclose(fp);
    }


    free(instrumented);
}

