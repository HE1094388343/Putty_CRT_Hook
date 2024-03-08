#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <Windows.h>
#include <TlHelp32.h>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadCommentsFromFile(const QString &fileName);
    DWORD GetProcessIdByName(const wchar_t* processName);
    bool InjectDll(DWORD processId, const wchar_t* dllPath);
    bool IsProcessRunning(const wchar_t* processName);
    bool isProcess64Bit(DWORD processId);
    bool isDllLoaded(const std::wstring& dllName, DWORD PID);
    void jianceCRT();
    const QString fileName="C:\\Comment.txt";
     bool CTR;
     bool tty;
private slots:
    void on_pushButton_clicked();



    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;


};

#endif // MAINWINDOW_H
