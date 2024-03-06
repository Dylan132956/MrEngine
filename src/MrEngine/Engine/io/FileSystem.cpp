#include "FileSystem.h"
#include "stdio.h"
#include <memory>
#ifdef VR_ANDROID
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),  (mode)))==NULL
#endif

namespace moonriver 
{
    void Error(const char* message, const char* detail)
    {
        if (detail != nullptr)
            fprintf(stderr, "%s: ", detail);
        fprintf(stderr, "%s (use -h for usage)\n", message);
    }

    FileSystem::FileSystem()
    {

    }
    FileSystem::~FileSystem()
    {

    }

    char* FileSystem::ReadFileData(const char* fileName)
    {
        FILE* in = nullptr;
        int errorCode = fopen_s(&in, fileName, "r");
        if (errorCode || in == nullptr)
            Error("unable to open input file");

        int count = 0;
        while (fgetc(in) != EOF)
            count++;

        fseek(in, 0, SEEK_SET);

        char* return_data = (char*)malloc(count + 1);  // freed in FreeFileData()
        if ((int)fread(return_data, 1, count, in) != count) {
            free(return_data);
            Error("can't read input file");
        }

        return_data[count] = '\0';
        fclose(in);

        return return_data;
    }

    void FileSystem::FreeFileData(char* data)
    {
        free(data);
    }
}