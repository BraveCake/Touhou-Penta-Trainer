#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <chrono>
#include <atomic>

DWORD getProcessIdByName(const char* processName) {
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create snapshot." << std::endl;
        return 0;
    }

    if (Process32First(snapshot, &processEntry)) {
        do {
            if (strcmp(processEntry.szExeFile, processName) == 0) {
                CloseHandle(snapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
    std::cerr << "Process not found!" << std::endl;
    return 0;
}

DWORD_PTR getBaseAddress(DWORD processId) {
    MODULEENTRY32 moduleEntry;
    moduleEntry.dwSize = sizeof(MODULEENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create snapshot of modules." << std::endl;
        return 0;
    }

    if (Module32First(snapshot, &moduleEntry)) {
        do {
            if (strcmp(moduleEntry.szModule, "np21nt.exe") == 0) {
                CloseHandle(snapshot);
                return (DWORD_PTR)moduleEntry.modBaseAddr;
            }
        } while (Module32Next(snapshot, &moduleEntry));
    }

    CloseHandle(snapshot);
    std::cerr << "Base address not found!" << std::endl;
    return 0;
}

void promptForValue(const char* name, int& value) {
    std::cout << "Enter " << name << " value (or -1 to skip): ";
    std::cin >> value;
    if (value == -1) {
        std::cout << name << " value unchanged." << std::endl;
    }
}
int getTypeSize(const char* name){ //return number of bytes required by these types
    if(!strcmp("HP",name)||!strcmp("Bombs",name)){
        return 1;}
    if(!strcmp("Power",name)){
        return 2;}
    else {
        return 4;}

}
bool writeValue(HANDLE hProcess, DWORD_PTR address, int value, const char* name) {
    if (value == -1) return true; // Skip writing if user chose not to change
    if(address== NULL){
      std::cout<<"Unsupported feature for this touhou version"<<std::endl;
      return true;
    }
    int typeSize= getTypeSize(name);
    if (!WriteProcessMemory(hProcess, (LPVOID)(address), &value,typeSize, NULL)) {
        std::cerr << "Failed to write " << name << "." << std::endl;
        std::cerr<<"address: "<<std::hex<<address<<std::endl;
        return false;
    }
    std::cout << name << " updated successfully." << std::endl;
    return true;
}


int main() {
    const char* targetProcess = "np21nt.exe";
    DWORD processId = getProcessIdByName(targetProcess);
    unsigned short int version;

    if (processId == 0) {
        std::cerr << "Target process not found." << std::endl;
        return 1;
    }

    std::cout << "Process found with PID: " << processId << std::endl;

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processId);
    if (hProcess == NULL) {
        std::cerr << "Failed to open process." << std::endl;
        return 1;
    }

    DWORD_PTR baseAddress = getBaseAddress(processId);
    if (baseAddress == 0) {
        CloseHandle(hProcess);
        return 1;
    }

    std::cout << "Base address: " << std::hex << baseAddress << std::dec << std::endl;
    std::cout<<"Please choose which touhou version you wanna operate on (1v:5v): ";
    std::cin>>version;
    --version; // 0-4 indexes I might add touhou 1 later

    DWORD_PTR hpAddress[] = {baseAddress+0x4CD030,baseAddress + 0x4B893C,baseAddress+0x4C6DAD,baseAddress+0x51C33B,baseAddress+0x4BF610};
    DWORD_PTR bombsAddress[] = {baseAddress+0x4CCFE2,baseAddress + 0x4B893D,baseAddress+0x4C6DC3,baseAddress+0x51C33D,baseAddress+0x4BF611};
    DWORD_PTR PowerAddress[] = {NULL,baseAddress+0x4B88B6 ,NULL, baseAddress + 0x4B2074 ,baseAddress+0x4C717E };
    DWORD_PTR scoreAddress[] = {NULL,baseAddress + 0x4B8938,baseAddress+0x4C5364,baseAddress+0x4B1D5C,baseAddress+0x4C70C8};

    int hp, bombs, power = -1, score;

    while (true) {
        // Prompt for values
        std::cout<<"Touhou "<<version+1<<" trainer"<<std::endl;
        promptForValue("HP", hp);
        promptForValue("Bombs", bombs);
        promptForValue("Score", score);
        promptForValue("Power", power);

        // Write the values
        writeValue(hProcess, hpAddress[version], hp, "HP");
        writeValue(hProcess, bombsAddress[version], bombs, "Bombs");
        writeValue(hProcess, scoreAddress[version], score, "Score");
        writeValue(hProcess,PowerAddress[version],power,"Power");

        // Ask the user if they want to continue
        std::cout << "Do you want to re-enter values? (y/n): ";
        char choice;
        std::cin >> choice;

        system("cls");//Clear screen
    }


    CloseHandle(hProcess);
    std::cout << "Program terminated." << std::endl;
    return 0;
}
