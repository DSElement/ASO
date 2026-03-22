#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 1024

int main() {
    char pathBuffer[256];
    if (fgets(pathBuffer, sizeof(pathBuffer), stdin) == NULL) {
        perror("Error reading input");
        return EXIT_FAILURE;
    }
    pathBuffer[strcspn(pathBuffer, "\r\n")] = '\0';

    if (access(pathBuffer, F_OK) != 0) {
        perror("File does not exist");
        return EXIT_FAILURE;
    }

    FILE *file = fopen(pathBuffer, "r");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    char *fileContent = NULL;
    char chunk[CHUNK_SIZE];
    size_t currentLength = 0;

    while (fgets(chunk, sizeof(chunk), file) != NULL) {
        size_t chunkLength = strlen(chunk);

        char *newContent = realloc(fileContent, currentLength + chunkLength + 1);
        if (newContent == NULL) {
            perror("Memory allocation failed");
            free(fileContent);
            fclose(file);
            return EXIT_FAILURE;
        }

        fileContent = newContent;
        memcpy(fileContent + currentLength, chunk, chunkLength + 1);
        currentLength += chunkLength;
    }

    if (fileContent != NULL) {
        printf("%s", fileContent);
    }

    free(fileContent);
    fclose(file);
    return 0;
}