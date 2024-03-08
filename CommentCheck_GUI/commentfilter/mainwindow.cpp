#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include<thread>
#include<QThread>
#include <Psapi.h>
#include <shlwapi.h>

#pragma execution_character_set("utf-8")
bool CRT=false;
bool tty=false;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(970, 550);
    loadCommentsFromFile(fileName);
    std::thread t(std::bind(&MainWindow::jianceCRT, this)); // 将成员函数绑定到实例上
    t.detach();
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    ui->listWidget->addItem(ui->lineEdit->text());
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << ui->lineEdit->text() << endl;
        file.close();
    }

}




void MainWindow::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{



    QString textToBeDeleted = item->text();
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList lines;
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line != textToBeDeleted) {
                lines.append(line);
            }
        }
        file.close();

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream out(&file);
            foreach(const QString &line, lines) {
                out << line << endl;
            }
            file.close();
        } else {
            // 处理文件打开失败的情况
        }
    } else {
        // 处理文件打开失败的情况
    }
    int row = ui->listWidget->row(item);
    delete ui->listWidget->takeItem(row);
}
void MainWindow::loadCommentsFromFile(const QString &fileName) {
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            ui->listWidget->addItem(line);
        }
        file.close();
    } else {
        // 处理文件打开失败的情况
    }
}

bool MainWindow::isDllLoaded(const std::wstring& dllName, DWORD PID) {
    HMODULE hModules[1024];
    DWORD cbNeeded;
    HANDLE hProcessSnap = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, (DWORD)PID);
    // 获取进程模块列表的大小
    if (!EnumProcessModules(hProcessSnap, hModules, sizeof(hModules), &cbNeeded)) {

        return false;
    }

    // 检查指定的DLL是否在进程模块列表中
    for (DWORD i = 0; i < cbNeeded; i++) {
        std::wstring moduleName(MAX_PATH, L'\0'); // 初始化为足够大的空间

        DWORD length = GetModuleFileNameEx(hProcessSnap, hModules[i], &moduleName[0], moduleName.size());
        if (length == 0) {
            // 获取文件名失败
            // 处理错误情况
        } else {
            moduleName.resize(length); // 裁剪字符串，以适应实际的文件名长度
            if (moduleName.find(dllName) != std::wstring::npos) {
                return true;
            }
        }
    }

    return false;
}
bool MainWindow::isProcess64Bit(DWORD processId) {
    BOOL isWow64 = FALSE;
    IsWow64Process(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId), &isWow64);
    return isWow64 == TRUE;
}
// 获取指定进程名对应的进程ID
DWORD MainWindow::GetProcessIdByName(const wchar_t* processName) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snapshot, &pe)) {
            do {
                if (wcscmp(pe.szExeFile, processName) == 0) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

// 在指定进程中注入 DLL
bool MainWindow::InjectDll(DWORD processId, const wchar_t* dllPath) {
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (processHandle == NULL) {
        return false;
    }

    LPVOID remoteString = VirtualAllocEx(processHandle, NULL, wcslen(dllPath) * sizeof(wchar_t) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (remoteString == NULL) {
        CloseHandle(processHandle);
        return false;
    }

    if (!WriteProcessMemory(processHandle, remoteString, dllPath, wcslen(dllPath) * sizeof(wchar_t) + 1, NULL)) {
        VirtualFreeEx(processHandle, remoteString, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return false;
    }

    HMODULE kernel32Module = GetModuleHandleW(L"kernel32.dll");
    if (kernel32Module == NULL) {
        VirtualFreeEx(processHandle, remoteString, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return false;
    }

    LPVOID loadLibraryAddr = GetProcAddress(kernel32Module, "LoadLibraryW");
    if (loadLibraryAddr == NULL) {
        VirtualFreeEx(processHandle, remoteString, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return false;
    }

    HANDLE remoteThread = CreateRemoteThread(processHandle, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, remoteString, 0, NULL);
    if (remoteThread == NULL) {
        VirtualFreeEx(processHandle, remoteString, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return false;
    }

    WaitForSingleObject(remoteThread, INFINITE);

    CloseHandle(remoteThread);
    VirtualFreeEx(processHandle, remoteString, 0, MEM_RELEASE);
    CloseHandle(processHandle);

    return true;
}

bool MainWindow::IsProcessRunning(const wchar_t* processName)
{
    DWORD pid = 0;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);

    return pid != 0;
}

void MainWindow::jianceCRT()
{
    // 获取程序运行路径
    QString programPath = QCoreApplication::applicationDirPath();
    // 构造 DLL 的完整路径
    QString dllPath = programPath + "/smc.dll";
    const wchar_t* targetProcessName_SecureCRT = L"SecureCRT.exe";
    const wchar_t* targetProcessName_Putty = L"putty.exe";
    QString puttyStatus = "未监控";
    QString crtStatus =   "未监控";
    while (true)
    {
        // 将 QString 转换为 wchar_t*
        wchar_t* dllPathWchar = new wchar_t[dllPath.length() + 1];
        dllPath.toWCharArray(dllPathWchar);
        dllPathWchar[dllPath.length()] = L'\0';
        DWORD targetProcessIdputty = GetProcessIdByName(targetProcessName_Putty);
        bool isloadmoduleCRT = isDllLoaded(L"smc.dll",targetProcessIdputty);

        if (targetProcessIdputty == 0 || isloadmoduleCRT==false)
        {
            puttyStatus="未监控";
        }
        if (targetProcessIdputty != 0&& isloadmoduleCRT==false)
        {
             bool isWow64  = isProcess64Bit(targetProcessIdputty);
            if(isWow64==true)
            {
                if (InjectDll(targetProcessIdputty, dllPathWchar)) {
                    puttyStatus="正在监控";
                }
                else {
                    puttyStatus="监控失败请尝试管理员运行";
                }
            }
            else
            {
                puttyStatus="请用32位的putty";
            }

        }

        DWORD targetProcessIdCRT = GetProcessIdByName(targetProcessName_SecureCRT);
        bool isloadmodule = isDllLoaded(L"smc.dll",targetProcessIdCRT);
        if (targetProcessIdCRT == 0 || isloadmodule==false)
        {
            crtStatus="未监控";
        }
        if (targetProcessIdCRT != 0&& isloadmodule==false)
        {
            bool isWow64  = isProcess64Bit(targetProcessIdCRT);
            if(isWow64==true)
            {

                if (InjectDll(targetProcessIdCRT, dllPathWchar)) {
                      crtStatus="正在监控";
                }
                else {
                       crtStatus="监控失败请尝试管理员运行";

                }
            }
            else
            {
                  crtStatus="请用32位的CRT";

            }
        }
        QString formattedString = QString("putty状态:%1   CRT状态:%2").arg(puttyStatus).arg(crtStatus);
        ui->label_3->setText(formattedString);
        delete[] dllPathWchar; // 释放内存
        QThread::msleep(1000);
    }




}
void MainWindow::on_pushButton_2_clicked()
{
ui->listWidget->clear();
  loadCommentsFromFile(fileName);

}

