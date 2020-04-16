#pragma comment(lib, "Dbghelp.lib")

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <imagehlp.h>

int main(int argc, char **argv) {

    if(argc < 3) {
        std::cout << "Usage: " << argv[0] << " <path to dll> <renamed original>\n";
        return 1;
    }

    std::string dllPath(argv[1]);
    std::string renamed(argv[2]);

    auto fileHandle = CreateFileA(dllPath.c_str(), GENERIC_ALL, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(fileHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Unable to open " << dllPath << "\n";
        return 1;
    }

    auto fileSize = GetFileSize(fileHandle, 0);
    auto fileData = HeapAlloc(GetProcessHeap(), 0, fileSize);

    DWORD bytesRead = 0;
    ReadFile(fileHandle, fileData, fileSize, &bytesRead, 0);
    CloseHandle(fileHandle);

    auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(fileData);
    if(dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        std::cerr << "File does not seem to be valid PE file\n";
        return 1;
    }

    auto ntHeader = ImageNtHeader(fileData);
    
    ULONG exportDirectorySize = 0;
    auto exportDirectory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(ImageDirectoryEntryToData(fileData, FALSE, IMAGE_DIRECTORY_ENTRY_EXPORT, &exportDirectorySize));

    if(exportDirectory == 0) {
        std::cerr << "Unable to parse PE file\n";
        return 1;
    }

    auto exportNamesVA = reinterpret_cast<DWORD*>(ImageRvaToVa(ntHeader, fileData, exportDirectory->AddressOfNames, 0));

    for(int i = 0; i < exportDirectory->NumberOfNames; i++) {
        char* exportName = reinterpret_cast<char *>(ImageRvaToVa(ntHeader, fileData, exportNamesVA[i], 0));
        printf(R"(#pragma comment(linker, "/export:%s=%s.%s"))", exportName, renamed.c_str(), exportName);
        std::cout << "\n";
    }

    return 0;
}