#include <iostream>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>
#include <string>
#include <QTextCodec>
#include <QString>
#include <Windows.h>
#include "HTTPRequest.h"
#include <QByteArray>
#include <QBitArray>
#include <cctype>
#include <iomanip>
#include <sstream>

CRITICAL_SECTION cs;

using namespace std;
string url_encode(const string &value) {
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
int p_score(QString str, QString pattern) {
    QString v = str.toUpper();
    QString s = pattern.toUpper();
    int count = 0;
    for (int i = 0; i < str.size(); i++) {
        int index = v.indexOf(s, i);
        if (index != -1) {
            count++;
            i = index;
        }
    }
    return count;
}
string query(string url){
    EnterCriticalSection( &cs );
    string result = "FAIL";
    try
    {
        http::Request request(url);
        const http::Response response = request.send("GET");
        result = std::string(response.body.begin(), response.body.end());
    }
    catch (const std::exception& e)
    {
        cout << "Request failed, error: " << e.what() << '\n';
    }
    LeaveCriticalSection( &cs );
    return result;
}
int main(int argc, char *argv[])
{
    setlocale( LC_ALL, "");
    QTextCodec *utfcodec = QTextCodec::codecForName("UTF-8");
    QCoreApplication a(argc, argv);
    InitializeCriticalSection( &cs );
    QString word;
    int morale;
    if(argc <= 1){
        word = "NOT FROM PANEL";
        morale = 0;
    }
    else{
        word = QString::fromUtf8(QByteArray::fromBase64(argv[1]));
        morale = atoi(argv[2]);
        qInfo() << word << " x " << morale << endl;
    }
    int last_message = stoi(query("http://shumik.site/chat/check_new_messages.php?only_id=true"));
    while(1){
        Sleep(1);
        string json = query("http://shumik.site/chat/get_new_messages.php?start=" + to_string(last_message));
        QJsonDocument jsonResponse = QJsonDocument::fromJson(QString::fromStdString(json).toUtf8());
        QJsonArray arr = jsonResponse.array();
        if(arr.size() > 0){
            bool at_least = false;
            QJsonArray increment;
            for(int i = 0; i < arr.size(); i++){
                QJsonObject row = arr[i].toObject();
                int id = row["id"].toInt();
                QString text = row["text"].toString();
                if(id > last_message) last_message = id;
                int score = p_score(text, word)*morale;
                if(score > 0){
                    at_least = true;
                    QJsonObject inc_row;
                    inc_row.insert("id", QJsonValue::fromVariant(id));
                    inc_row.insert("increment", QJsonValue::fromVariant(score));
                    increment.push_back(inc_row);
                    qInfo() << "+" << score << " in " << text;
                }
            }
            if(at_least){
                QJsonDocument doc(increment);
                const char *data = doc.toJson().data();
                cout << query("http://shumik.site/chat/increment_next.php?str=" + url_encode(string(data)));
            }
        }
    }
    return a.exec();
}
