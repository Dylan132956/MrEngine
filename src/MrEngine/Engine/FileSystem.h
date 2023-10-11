#pragma once

namespace moonriver
{
    void Error(const char* message, const char* detail = nullptr);

    class FileSystem
    {
    public:
        FileSystem();
        ~FileSystem();
        static char* ReadFileData(const char* fileName);
        static void FreeFileData(char* data);
    };
}