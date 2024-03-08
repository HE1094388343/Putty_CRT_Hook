// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include"detours.h"
#include<iostream>
#include<Windows.h>
#include <fstream>
#include<vector>
#include <sstream>
#include <regex>
#include <Psapi.h>
#include <cstring>
#pragma comment(lib, "detours.lib")


typedef void* (__cdecl* MEMMOVE)(void*, const void*, size_t);
typedef void* (__cdecl* MEMCPY)(void*, const void*, size_t);
typedef void* (__cdecl* Remove)(char src);
typedef void* (__cdecl* Check)(void* src);





typedef void (*Putty_Check)(void*,  void*, size_t);
typedef void* (__cdecl* Putty_Remove)(char src);
// 定义函数指针，用于保存原始的 memmove 和 memcpy 函数地址
MEMMOVE OriginalMemmove = nullptr;

//CRT
Check OriginalCheck = nullptr;
Remove OriginalRemove = nullptr;

//Putty软件hook点位
Putty_Check Putty_OriginalCheck = nullptr;
Putty_Remove Putty_OriginalRemove = nullptr;

static std::string accumulatedString="";
void* __cdecl MyMemmove(void* dest, const void* src, size_t count) {
    
    if (count == 1) {
    const char* srcChar = static_cast<const char*>(src);

    // 定义一个静态字符串用于累积字符
    static std::string accumulatedChars;

    // 将当前字符追加到累积字符串
    accumulatedChars += *srcChar;

    // 定义目标字符串列表
    std::vector<std::string> targetStrings = { "ki", "rm"};

    // 遍历目标字符串列表
    for (const auto& targetString : targetStrings) {
        for (size_t i = 0; i < accumulatedChars.length(); ++i) {
            // 判断是否匹配目标字符串的开头
            if (accumulatedChars.compare(i, targetString.length(), targetString) == 0) {
                // 执行相应的操作，比如触发相应逻辑
                if(targetString.compare("ki")==0)
                    std::cout << "检测到危险命令: " << targetString<<"ll" << std::endl;
                else
                {
                    std::cout << "检测到危险命令: " << targetString << std::endl;
                }
                // 清空累积字符串
                accumulatedChars.clear();
                
                void* result = OriginalMemmove(dest, "", count);
                return result;
                break;
            }
        }
    }

    // 判断是否累积达到 1000 个字符
    if (accumulatedChars.length() >= 1000) {
        // 执行相应的操作，比如处理累积的字符
        std::cout << "累积的字符达到 1000 个：" << accumulatedChars << std::endl;
        // 清空累积字符串
        accumulatedChars.clear();
    }
  
}
    // 需要查找的命令数组
    //const char* commands[] = { "kill", "rm"};

    //const char* srcChar = static_cast<const char*>(src);

    //// 遍历所有命令
    //for (const char* command : commands) {
    //    size_t commandLength = strlen(command);
    //    for (size_t i = 0; i <= count - commandLength; ++i) {
    //        if (memcmp(srcChar + i, command, commandLength) == 0) {
    //            std::ofstream outputFile("D:\\example.txt", std::ios::app | std::ios::out);

    //            if (outputFile.is_open()) {
    //                // 追加内容到文件
    //                outputFile << "找到：" << command << "\n";

    //                // 关闭文件
    //                outputFile.close();
    //            }
    //            else {
    //                std::cerr << "无法打开或创建文件" << std::endl;
    //            }
    //        }
    //    }
    //}
    return OriginalMemmove(dest, src, count);
}

uintptr_t FindPattern(uintptr_t start, size_t length, const unsigned char* pattern, const char* mask)
{
    size_t pos = 0;
    auto maskLength = std::strlen(mask) - 1;

    auto startAdress = start;
    for (auto it = startAdress; it < startAdress + length; ++it)
    {
        if (*reinterpret_cast<unsigned char*>(it) == pattern[pos] || mask[pos] == '?')
        {
            if (mask[pos + 1] == '\0')
            {
                return it - maskLength;
            }

            pos++;
        }
        else
        {
            pos = 0;
        }
    }

    return -1;
}
uintptr_t FindPatter_n(HMODULE module, const unsigned char* pattern, const char* mask)
{
    MODULEINFO info = { };
    GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

    return FindPattern(reinterpret_cast<uintptr_t>(module), info.SizeOfImage, pattern, mask);
}
std::vector<std::string> ReadCommandsFromFile(const std::string& fileName) {
    std::vector<std::string> commands;
    std::ifstream file(fileName);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            commands.push_back(line);
        }
        file.close();
    }
    else {
        std::cerr << "Failed to open file: " << fileName << std::endl;
    }
    return commands;
}
HHOOK g_hHook = NULL;
UINT_PTR g_nHotkeyID = 1; // 热键ID







LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        DWORD currentProcessId;
     
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        GetWindowThreadProcessId(GetForegroundWindow(), &currentProcessId);
        DWORD g_targetProcessId = GetCurrentProcessId();
        if (currentProcessId == g_targetProcessId) {
            if (wParam == WM_KEYDOWN) {
                if (kbdStruct->vkCode == VK_RETURN) {
                    std::vector<std::string> highRiskCommands = ReadCommandsFromFile("C:\\Comment.txt");
                    for (const auto& command : highRiskCommands) {
                        if (accumulatedString.find(command) != std::string::npos) {

                            return 1;   //判断是否有危险字符 有的话禁止按回车
                        }

                    }
                  
                }
            }
        }

    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}



//检测函数
void*  CheckFun(void* src)
{
   // std::vector<std::string> highRiskCommands = { "rm /root", "rm /home"};
    std::vector<std::string> highRiskCommands=  ReadCommandsFromFile("C:\\Comment.txt");


    char* srcChar = static_cast< char*>(src);
    accumulatedString += srcChar;

    for (const auto& command : highRiskCommands) {
        if (accumulatedString.find(command) != std::string::npos) {
            // 如果发现，则触发警报并将第一个字符替换为 '-'
            std::cout << "发现高危操作" << std::endl;
            *srcChar = '-';
            return srcChar;
        }
    }
    std::cout << "命令检测：" << accumulatedString << std::endl;
    return src; 
}

void __cdecl Putty_CheckFun(void* dest,  void* src, size_t count)
{
    
    std::vector<std::string> highRiskCommands = ReadCommandsFromFile("C:\\Comment.txt");
    char* srcChar = static_cast<char*>(src);
    if (srcChar != nullptr) 
    {
  
            for (size_t i = 0; i < count; ++i) 
            {
               
                if ((int)srcChar[i] != 8 && (int)srcChar[i] != 27 && (int)srcChar[i] != 91 && (int)srcChar[i] != 75)
                {
                    accumulatedString += srcChar[i];
                }
                   
                if ((int)srcChar[i] == 7 || (int)srcChar[i] == 8)
                {
                    if (!accumulatedString.empty())
                    {
                        accumulatedString.erase(accumulatedString.size() - 1);

                    }
                }
                else if ((int)srcChar[i] == 13)
                {
                    if (!accumulatedString.empty())
                    {
                        accumulatedString.clear();
                    }
                }
            }
    }
    std::cout << "命令检测：" << accumulatedString << std::endl;
    for (const auto& command : highRiskCommands) {
        if (accumulatedString.find(command) != std::string::npos) {
            // 如果发现，则触发警报并将第一个字符替换为 '-'
            std::cout << "发现高危操作" << std::endl;
            *srcChar = '-';
    
        }

    }

    Putty_OriginalCheck(dest, src, count);
   
}
//调用检测函数
__declspec(naked) void* __cdecl MyCheckFun() {

    _asm {
        pushad
        push edx
        call CheckFun
        add esp,4
        popad
        jmp OriginalCheck
    }
}

void RemoveStr(BYTE src)
{
    if (static_cast<int>(src) == 8) 
    {
        if (!accumulatedString.empty())
        {
            accumulatedString.erase(accumulatedString.size() - 1);
            std::cout << "命令检测：" << accumulatedString << std::endl;
        }

    }
    else if (static_cast<int>(src) == 13)
    {
        if (!accumulatedString.empty())
        {
            accumulatedString.clear();
        }
           
 
    }
}
//调用清空函数
__declspec(naked) void* __cdecl  MyRemove() {


    _asm {
        pushad
        push ax
        call RemoveStr
        pop ax
        popad
        jmp OriginalRemove
    }
}
std::wstring GetProcessName()
{
    TCHAR szFileName[MAX_PATH];
    if (GetModuleFileName(NULL, szFileName, MAX_PATH) == 0)
    {
        std::cerr << "Failed to get module file name." << std::endl;

    }

    std::wstring path(szFileName);
    std::size_t found = path.find_last_of(L"\\");

    std::wstring processName = path.substr(found + 1);

    return processName;
}
void init()
{
 
    //AllocConsole();
    //ShowWindow(GetConsoleWindow(), SW_SHOW);
    //FILE* fp;
    //freopen_s(&fp, "CONOIN$", "r", stdin);
    //freopen_s(&fp, "CONOUT$", "w", stdout);
    //freopen_s(&fp, "CONOUT$", "w", stderr);
    uint32_t BaseAddress = (uint32_t)GetModuleHandle(NULL);
    auto name =GetProcessName();
    std::wcout << "Process Name: " << name << std::endl;

    if (name.compare(L"putty.exe") != std::string::npos)
    {
        auto OriginalCheckOff = FindPatter_n((HMODULE)BaseAddress, (unsigned char*)"\x57\x56\x8B\x74\x24\x0C\x8D\xBE\x00\x00\x00\x00\xFF\x74\x24\x14\xFF\x74\x24\x14\x57", "xxxxxxxx????xxxxxxxxx");
        std::wcout << "OriginalCheckOff: " <<std::hex<< OriginalCheckOff << std::endl;
  

        auto CheckOffset = (((uint32_t)OriginalCheckOff) - BaseAddress);


        Putty_OriginalCheck= (Putty_Check)(((DWORD)GetModuleHandle(L"putty.exe")) + CheckOffset);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)Putty_OriginalCheck, Putty_CheckFun);

        DetourTransactionCommit();
    }
    else
    {
        auto OriginalRemoveOff = FindPatter_n((HMODULE)BaseAddress, (unsigned char*)"\x0F\xB7\xD8\x8B\xC7\x89\x5D\xEC\xE8\x00\x00\x00\x00\x85\xC0\x7E\x48\x83\xFB\x03", "xxxxxxxxx????xxxxxxx");
        auto OriginalCheckOff = FindPatter_n((HMODULE)BaseAddress, (unsigned char*)"\xE8\x00\x00\x00\x00\x5F\x5E\xB0\x01\x5B\x5D\xC2\x04\x00\xCC\xCC\x55", "x????xxxxxxxxxxxx");
  
        auto RemoveOffset = (((uint32_t)OriginalRemoveOff) - BaseAddress);
        auto CheckOffset = (((uint32_t)OriginalCheckOff) - BaseAddress);
        OriginalRemove = (Remove)(((DWORD)GetModuleHandle(L"securecrt.exe")) + RemoveOffset);
        OriginalCheck = (Check)(((DWORD)GetModuleHandle(L"securecrt.exe")) + CheckOffset);
        // 在进程附加时进行 hook，确保只执行一次
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)OriginalRemove, MyRemove);
        DetourAttach(&(PVOID&)OriginalCheck, MyCheckFun);

        DetourTransactionCommit();
    }
   

 

    
 

}

void cleanup(HINSTANCE dll)
{
    MSG msg;
    while (!GetAsyncKeyState(VK_END))
    {
        Sleep(20);
    }
    // 取消挂钩

    auto name = GetProcessName();

    if (name.compare(L"putty.exe") != std::string::npos)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)Putty_OriginalCheck, Putty_CheckFun);
        DetourTransactionCommit();
   
    }
    else
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)OriginalRemove, MyRemove);
        DetourDetach(&(PVOID&)OriginalCheck, MyCheckFun);

        DetourTransactionCommit();

    }
    UnhookWindowsHookEx(g_hHook);
  //  FreeConsole();
    FreeLibraryAndExitThread(dll, 0);
  
}
DWORD WINAPI KeyboardThread(LPVOID lpParam) {

    g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (g_hHook == NULL) {
        std::cerr << "Failed to set keyboard hook" << std::endl;
        return 1;
    }



    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
         
            init();
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)KeyboardThread, NULL, 0, NULL);
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cleanup, hModule, 0, NULL);
            break;
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:

 
            break;
    }
    return TRUE;
}

