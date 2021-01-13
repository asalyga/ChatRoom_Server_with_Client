#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    server_ip = "127.0.0.1";
    port_no   = 6000;
    username  = "user1";

    ui->btn_sendFile->setEnabled(false);
    ui->btn_sendTxt->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::receiverThread() {
    int ret = 1;
    u_int32_t bytes_expected;
    while(ret) {
        ret = client_obj->ccread_t(&bytes_expected, sizeof(bytes_expected), 60);
        if(ret > 0) {
            char rx_buf[bytes_expected + 1];
            ret = client_obj->ccread_t(rx_buf, bytes_expected, 60);
            if(ret > 0) {
                rx_buf[bytes_expected] = '\0';
//                cout << "Received Message:\n" << rx_buf << endl;
                ui->lbl_rcvdText->setText(QString::fromUtf8(rx_buf));
            }
        }
        else
            break;
    }
    exit(0);
}

void MainWindow::on_btn_connect_clicked()
{
    username = ui->line_usrnaem->text().toStdString();
    server_ip = ui->line_ServerIP->text().toStdString();
    port_no  = ui->line_ServerPort->text().toInt();

    client_obj = new TCP_Client();
    if(client_obj->connect_t(server_ip.c_str(), port_no, 600, 10) < 0) {
        exit(0);
    }

    thread_rx = std::thread(&MainWindow::receiverThread, this);

    u_int32_t bytes_to_send = username.size();
    client_obj->cwrite(&bytes_to_send, sizeof(bytes_to_send));
    client_obj->cwrite((void*)username.c_str(), bytes_to_send);

    ui->btn_sendFile->setEnabled(true);
    ui->btn_sendTxt->setEnabled(true);
}

void MainWindow::on_btn_sendTxt_clicked()
{
    text_in = "1" + ui->line_input->text().toStdString();
    u_int32_t bytes_to_send = text_in.size();
    client_obj->cwrite(&bytes_to_send, sizeof(bytes_to_send));
    client_obj->cwrite((void*)text_in.c_str(), bytes_to_send);
}

void MainWindow::on_btn_sendFile_clicked()
{
    text_in = "2" + ui->line_input->text().toStdString();
    u_int32_t bytes_to_send = text_in.size();
    client_obj->cwrite(&bytes_to_send, sizeof(bytes_to_send));
    client_obj->cwrite((void*)text_in.c_str(), bytes_to_send);
    text_in = ui->line_input->text().toStdString();
    client_obj->sendCustomFile(text_in.c_str());
}
