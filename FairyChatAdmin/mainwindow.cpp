#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <Windows.h>
#include <QMessageBox>
#include <QDebug>
using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::on_pushButton_clicked()
{
    QString text = ui->stop->text().trimmed();
    if(text.indexOf(" ") >= 0){
        QMessageBox::warning(this, "Can't moderate string", "Only 1 word supplied");
        return;
    }
    STARTUPINFO sti;
    PROCESS_INFORMATION pi;
    QString strCMD = "ChatModerator.exe ";
    strCMD = strCMD + text.toUtf8().toBase64() + " " + QString::number(ui->morale->value());
    wstring CommandLine = strCMD.toStdWString();
    LPWSTR lpwCmdLine = &CommandLine[0];
    ZeroMemory(&sti, sizeof(STARTUPINFO));
    /*sti.dwFlags = STARTF_USEPOSITION | STARTF_USESIZE;
    sti.dwX = 100;
    sti.dwY = 100;
    sti.dwXSize = 100;
    sti.dwYSize = 400;*/
    sti.cb = sizeof(STARTUPINFO);
    CreateProcess(NULL, lpwCmdLine, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &sti, &pi);
}
