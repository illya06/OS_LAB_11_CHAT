#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGroupBox>
#include <QLabel>
#include <QMouseEvent>
#include "HTTPRequest.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <Windows.h>
#include <QTimer>
#include <QScrollBar>
#include <QDebug>
#include <QMessageBox>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <QSound>
using namespace std;

Ui::MainWindow *realUi;
QWidget *parent;
QTimer *updateTimer = new QTimer();
HANDLE ghMutex;

int moral_limit = 0;
int user_id = -1;
int last_change = 0;
int last_update = 0;

string NewMessages = "";

bool lighttheme = false;
void reload_theme(QString theme){
    QString field_colour, back_colour, income_colour, outcome_colour, text_colour = "white";
    if(theme == "dark"){
        field_colour = "#17212b";
        back_colour = "#0e1621";
        income_colour = "#182533";
        outcome_colour = "#2b5278";
        text_colour = "white";
    }
    QString chat_income_stylesheet = "[accessibleName=income] {border: none;background-color: " + income_colour + ";margin-top:6.1em;}[accessibleName=income]::title {subcontrol-origin: padding;subcontrol-position: left top;background: transparent;margin-top: -2.5em; color:" + text_colour + "}";
    QString chat_outcome_stylesheet = "[accessibleName=outcome] {border: none;background-color: " + income_colour + ";margin-top:6.1em;}[accessibleName=outcome]::title {subcontrol-origin: padding;subcontrol-position: left top;background: transparent;margin-top: -2.5em; color:" + text_colour + "}";
    QString layout_stylesheet = "[accessibleName=content]{color:" + text_colour + "; font-size:13px}";
    realUi->centralwidget->setStyleSheet(chat_income_stylesheet + chat_outcome_stylesheet + layout_stylesheet);
}
void clearLayout(QLayout *layout) {
    QLayoutItem *item;
    while((item = layout->takeAt(1))) {
        if (item->layout()) {
            clearLayout(item->layout());
            delete item->layout();
        }
        if (item->widget()) {
           delete item->widget();
        }
        delete item;
    }
}
void createMessage(bool income, QString author, QString time, QString date, int errors, QString content){
    QFrame *m_frame = new QFrame(parent);
    m_frame->setFrameShape(QFrame::NoFrame);
            QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored);
            QGroupBox *m_box = new QGroupBox();
            m_box->setLayout(new QHBoxLayout());
                m_box->setTitle((income ? author : "You") + " | " + time + " | " + QString::number(errors) + " " + (errors == 0 ? "✅" : (errors > moral_limit ? "✖" : "⚠️")));
                m_box->setToolTip(date);
                if(income) m_box->setStyleSheet("QGroupBox {border: none;background-color: #182533;margin-top:1.1em;}QGroupBox QGroupBox {background-color: green;}QGroupBox::title {subcontrol-origin: padding;subcontrol-position: left top;background: transparent;margin-top: -2.5em; color:white}");
                else m_box->setStyleSheet("QGroupBox {border: none;background-color: #2b5278;margin-top:1.1em;}QGroupBox QGroupBox {background-color: green;}QGroupBox::title {subcontrol-origin: padding;subcontrol-position: right top; right: .2em;background: transparent;margin-top: -2.5em; color:white}");
                QGridLayout *grid = new QGridLayout;
                grid->setMargin(0);
                    QLabel *text = new QLabel;
                    text->setSizePolicy(text->sizePolicy().horizontalPolicy(), QSizePolicy::Preferred);
                    text->setStyleSheet("QLabel{color:white; font-size:13px}");
                    text->setWordWrap(true);
                    text->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);
                    if(errors > moral_limit && income){
                        text->setText("MESSAGE WAS HIDDEN FOR YOUR SAFETY");
                        text->setStyleSheet("color:red; font-weight:bold; font-size:9px;border-color: red;border-width: 1px; border-style: solid; border-radius: 0px;");
                    }
                    else{
                        text->setText(content);
                    }
                    m_box->layout()->addWidget(text);
            if(income){
                grid->addWidget(m_box, 0, 0, 0, 2);
                grid->addWidget(new QFrame, 0, 2, 0, 1);
            }
            else{
                grid->addWidget(new QFrame, 0, 0, 0, 1);
                grid->addWidget(m_box, 0, 1, 0, 2);
            }
            m_frame->setLayout(grid);
    realUi->chat_layout->addWidget(m_frame);
}
void MainWindow::moveScrollBarToBottom(int min, int max)
{
    Q_UNUSED(min);
    int maximum = realUi->scrollArea->verticalScrollBar()->maximum();
    //qInfo() << maximum;
    realUi->scrollArea->verticalScrollBar()->setValue(maximum);
}
string url_encode(QString input) {
    const string value = input.toUtf8().constData();
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char) c);
        escaped << nouppercase;
    }
    return escaped.str();
}
string query(string url){
    DWORD dwWaitResult = WaitForSingleObject(ghMutex, INFINITE);
    string result = "FAIL";
    http::Request request(url);
    const http::Response response = request.send("GET");
    result = std::string(response.body.begin(), response.body.end());
    ReleaseMutex(ghMutex);
    return result;

}
void MainWindow::updating(){
   if(last_update < last_change){
       last_update = last_change;
       clearLayout(realUi->chat_layout);
       QJsonDocument jsonResponse = QJsonDocument::fromJson(QString::fromStdString(NewMessages).toUtf8());
       QJsonArray arr = jsonResponse.array();
       if(arr.size() > 0){
           for(int i = 0; i < arr.size(); i++){
               QJsonObject row = arr[i].toObject();
               createMessage(!(row["user"].toInt() == user_id), row["author"].toString(), row["form_time"].toString(), row["form_date"].toString(), row["errors"].toInt(), row["text"].toString());
           }
       }
       QSound::play(":/new/resources/alert.wav");
   }
}
unsigned int __stdcall check_new_messages(void *data){
    while(1){
        Sleep(1);
        int update = stoi(query("http://shumik.site/chat/check_new_messages.php"));
        if(update > last_change){
            NewMessages = query("http://shumik.site/chat/get_new_messages.php?start=0");
            last_change = update;
        }
    }
}
unsigned int __stdcall send_message(void *data){
    QString *query_text = (QString *)(data);
    string res = query((*query_text).toUtf8().constData());
}
void start_loops(){
    unsigned id;
    int num = 0;
    _beginthreadex(NULL, 0, check_new_messages, (void *)(&num), 0, &id);
    updateTimer->start();
}
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    ui->setupUi(this);
    realUi = ui;
    parent = this;
    ui->Allowed->setVisible(false);
    QObject::connect(ui->scrollArea->verticalScrollBar(), SIGNAL(rangeChanged(int,int)), this, SLOT(moveScrollBarToBottom(int, int)));
    updateTimer->setInterval(1000);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updating()));
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::mousePressEvent(QMouseEvent *event) {
    m_nMouseClick_X_Coordinate = event->x();
    m_nMouseClick_Y_Coordinate = event->y();
}
void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    move(event->globalX()-m_nMouseClick_X_Coordinate,event->globalY()-m_nMouseClick_Y_Coordinate);
}

void MainWindow::on_pushButton_4_clicked()
{
    setWindowState(Qt::WindowMinimized);
}

void MainWindow::on_pushButton_3_clicked()
{
    if(this->windowState() == Qt::WindowMaximized){
        setWindowState(Qt::WindowNoState);
    }
    else{
        setWindowState(Qt::WindowMaximized);
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    close();
}

void MainWindow::on_pushButton_5_clicked()
{
    if(ui->login->text() == ""){
        ui->login->setFocus();
        return;
    }
    if(ui->pass->text() == ""){
        ui->login->setFocus();
        return;
    }
    string query_text = "http://shumik.site/chat/check_user.php?u=" + url_encode(ui->login->text()) + "&p=" + url_encode(ui->pass->text()) + "&m=" + to_string(ui->morale->value());
    string user = query(query_text);
    if(user == "false"){
        ui->pass->setFocus();
        ui->pass->setStyleSheet("QLineEdit{\n	color: red;\nbackground-color:#17212b;\nborder:none;\nborder-radius:none;\n}");
    }
    else{
        moral_limit = ui->morale->value();
        user_id = stoi(user);
        ui->title_screen->setVisible(false);
        ui->Allowed->setVisible(true);
        start_loops();
    }
}

void MainWindow::on_pushButton_clicked()
{
    if(ui->message->toPlainText() != ""){
        QString *query_text = new QString("http://shumik.site/chat/send_message.php?text=" + QString::fromStdString(url_encode(ui->message->toPlainText())) + "&id=" + QString::number(user_id));
        unsigned id;
        _beginthreadex(NULL, 0, send_message, (void *)(query_text), 0, &id);
        ui->message->clear();
    }
}

void MainWindow::on_pass_textEdited(const QString &arg1)
{
    ui->pass->setStyleSheet("QLineEdit{\n	color: white;\nbackground-color:#17212b;\nborder:none;\nborder-radius:none;\n}");
}
