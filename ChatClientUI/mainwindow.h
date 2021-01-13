#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <thread>

#include "client.h"

using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btn_connect_clicked();

    void on_btn_sendTxt_clicked();

    void on_btn_sendFile_clicked();

private:
    Ui::MainWindow *ui;
    string username, server_ip, text_in;
    int port_no;
    std::thread thread_rx;

    TCP_Client *client_obj;

    void receiverThread();

};

#endif // MAINWINDOW_H
