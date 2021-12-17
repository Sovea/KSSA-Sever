#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <WinSock2.h>
#include <iostream>
#include <unistd.h>
#include <QDesktopServices>
#include <QDebug>
#include <string>
#include <QString>
#include <QUrl>
#include <QSqlDatabase>
#include <QSignalMapper>
#include <QSqlQuery>
#include <QSqlError>
#include <QtDebug>
#include <QApplication>
#include <QSqlRecord>
#include <QPainter>
#include <QPixmap>
#include <QFile>
#include <QBuffer>
#include <QDataStream>
#include <QSqlQueryModel>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QTime>
#include <QThread>
const int serverport = 24576;
pid_t pid;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->tcpServer = NULL;
    this->tcpSocket = NULL;
    myMapper = new QSignalMapper();
    this->Alive_users.count = 0;
    this->Alive_users.userid.clear();
    this->Alive_users.ipaddress.clear();
    this->Alive_users.port.clear();
    this->StartServer();
}

MainWindow::~MainWindow()
{
    delete ui;
}
//start server...
void MainWindow::StartServer()
{
    this->tcpServer = new QTcpServer(this);
    this->tcpServer->listen(QHostAddress::Any, 24576);
    qDebug() << "Server is running...";
    qDebug() << "server ip:" << this->tcpServer->serverAddress();
    qDebug() << "server port:" << this->tcpServer->serverPort();
    this->ui->label_ip->setText("Server's ip is：" + GetLocalIPAddress());
    this->init_Alive();
    this->timer = new QTimer(this);
    this->timer->start(90000);
    connect(this->timer, SIGNAL(timeout()), this, SLOT(Check_HeartBeat()));
    connect(this->tcpServer, SIGNAL(newConnection()), this, SLOT(newConnect()));
    this->db = QSqlDatabase::addDatabase("QMYSQL");
    this->db.setHostName("localhost");
    this->db.setPort(3306);
    this->db.setUserName("root");
    this->db.setPassword("root");
    this->db.setDatabaseName("network_p2p");
    this->db.open();
    if (!this->db.open())
    {
        qDebug() << "connect to mysql error" << this->db.lastError();
        return;
    }
    else
    {
        qDebug() << "connect to mysql OK";
    }
}

void MainWindow::init_Alive()
{
    this->Alive_users.count = 0;
    this->Alive_users.port.clear();
    this->Alive_users.time.clear();
    this->Alive_users.userid.clear();
    this->Alive_users.subtime.clear();
    this->Alive_users.ipaddress.clear();
    this->Alive_users.ListTcpsocket.clear();
}

//new Connection coming...
void MainWindow::newConnect()
{
    qDebug() << "There is a new connect come in...";
    Alive_users.ListTcpsocket.append(this->tcpServer->nextPendingConnection());
    this->tcpSocket = Alive_users.ListTcpsocket.last();
    connect(Alive_users.ListTcpsocket.last(), SIGNAL(readyRead()), myMapper, SLOT(map()));
    myMapper->setMapping(Alive_users.ListTcpsocket.last(), Alive_users.count);
    connect(myMapper, SIGNAL(mapped(int)), this, SLOT(changeTcpsocket(int)));
}
void MainWindow::changeTcpsocket(int index)
{
    this->tcpSocket = Alive_users.ListTcpsocket[index];
    QHostAddress hisIp = this->tcpSocket->peerAddress();
    int inthisPort = this->tcpSocket->peerPort();
    QHostAddress myIp = this->tcpSocket->localAddress();
    int intmyPort = this->tcpSocket->localPort();
    qDebug() << "IP of the target is: ";
    qDebug() << hisIp << endl;
    qDebug() << "Port of the target is: ";
    qDebug() << inthisPort << endl;
    qDebug() << "IP of mine is: ";
    qDebug() << myIp << endl;
    qDebug() << "Port of the mine is: ";
    qDebug() << intmyPort << endl;
    ReadMessages();
}
//read new message...
void MainWindow::ReadMessages()
{
    qDebug() << "I get the message!";
    QByteArray data = this->tcpSocket->readAll();
    QBuffer buf(&data);
    buf.open(QIODevice::ReadOnly);
    QDataStream in(&buf);
    MsgType type;
    in >> type;
    if (type == UsrLogin)
    {
        LoginMessage Message;
        QString user_id;
        in >> user_id;
        this->user_id = user_id;
        QString user_password;
        in >> user_password;
        in >> Message.Message;
        qDebug() << Message.Message;
        QString msg;
        if (this->connect_mysql_toverify(user_id, user_password))
        {
            this->connect_mysql_to_get_userinfo(user_id);
        }
        else
        {
            QByteArray data;
            data.resize(sizeof(LoginMessage));
            QBuffer buffer(&data);
            buffer.open(QIODevice::WriteOnly);
            QDataStream out(&buffer);
            LoginMessage Message;
            Message.type = UsrLogin;
            QString nomean = "NULL";
            Message.Message = "You are not one of us now!";
            out << Message.type << nomean << nomean << Message.Message << false;
            buffer.close();
            this->SendMessages(data);
        }
    }
    else if (type == Friendsinfo)
    {
        int count = 0;
        in >> count;
        QString userid;
        in >> userid;
        this->connect_mysql_getrelalist(userid);
    }
    else if (type == HeartBeat)
    {
        struct_heartbeat heartbeat;
        in >> heartbeat.userid;
        in >> heartbeat.ipaddress;
        in >> heartbeat.port;
        in >> heartbeat.Message;
        in >> heartbeat.time;
        qDebug() << heartbeat.Message;
        if (this->Alive_users.userid.contains(heartbeat.userid) == false)
        {
            this->Alive_users.userid << user_id;
            QHostAddress hisIp = this->tcpSocket->peerAddress();
            int inthisPort = this->tcpSocket->peerPort();
            QHostAddress myIp = this->tcpSocket->localAddress();
            int intmyPort = this->tcpSocket->localPort();
            qDebug() << "IP of the target is: ";
            qDebug() << hisIp << endl;
            qDebug() << "Port of the target is: ";
            qDebug() << inthisPort << endl;
            qDebug() << "IP of mine is: ";
            qDebug() << myIp << endl;
            qDebug() << "Port of the mine is: ";
            qDebug() << intmyPort << endl;
            this->Alive_users.count = this->Alive_users.count + 1;
            this->Alive_users.ipaddress << hisIp.toString();
            this->Alive_users.port << QString::number(heartbeat.port, 10);
            this->Alive_users.time << heartbeat.time.toString("HH:mm:ss");
            this->Alive_users.subtime << "0";
        }
        else
        {
            int index = this->Alive_users.userid.indexOf(heartbeat.userid);
            this->Alive_users.ipaddress.replaceInStrings(this->Alive_users.ipaddress[index], heartbeat.ipaddress);
            this->Alive_users.port.replaceInStrings(this->Alive_users.port[index], QString::number(heartbeat.port, 10));
            this->Alive_users.time.replaceInStrings(this->Alive_users.time[index], heartbeat.time.toString());
            QTime lasttime = QTime::fromString(this->Alive_users.time[index]);
            QTime nowtime = QTime::currentTime();
            int subtime = lasttime.secsTo(nowtime);
            this->Alive_users.subtime.replaceInStrings(this->Alive_users.subtime[index], QString::number(subtime));
        }
    }
    else if (type == LinkFriend)
    {
        qDebug() << "Now online targets are: " << this->Alive_users.userid << this->Alive_users.port;
        struct_Link_friend Link_friend;
        in >> Link_friend.userid;
        if (this->Alive_users.userid.contains(Link_friend.userid) == true)
        {
            QByteArray data;
            data.resize(sizeof(struct_Link_friend));
            QBuffer buffer(&data);
            buffer.open(QIODevice::WriteOnly);
            QDataStream out(&buffer);
            Link_friend.type = LinkFriend;
            int index = this->Alive_users.userid.indexOf(Link_friend.userid);
            qDebug() << index;
            Link_friend.port = this->Alive_users.port[index].toInt(nullptr, 10);
            qDebug() << Link_friend.port;
            Link_friend.ipaddress = QHostAddress(this->Alive_users.ipaddress[index]);
            Link_friend.online = true;
            out << Link_friend.type << Link_friend.userid << Link_friend.port << Link_friend.ipaddress << Link_friend.online;
            buffer.close();
            this->SendMessages(data);
        }
        else
        {
            qDebug() << "the one you want is offline.";
            QByteArray data;
            data.resize(sizeof(struct_Link_friend));
            QBuffer buffer(&data);
            buffer.open(QIODevice::WriteOnly);
            QDataStream out(&buffer);
            Link_friend.type = LinkFriend;
            out << Link_friend.type << Link_friend.userid << Link_friend.port << Link_friend.ipaddress << false;
            buffer.close();
            this->SendMessages(data);
        }
    }
    else if (type == DeleteFriend)
    {
        struct_handle_friend handle_friend;
        in >> handle_friend.Myid;
        in >> handle_friend.Hisid;
        qDebug() << handle_friend.Myid << handle_friend.Hisid;
        bool ifdele = this->connect_mysql_to_delete_friend(handle_friend.Myid, handle_friend.Hisid);
        qDebug() << "delete this one" << ifdele;
        handle_friend.ifsuccess = ifdele;
        QByteArray data;
        data.resize(sizeof(struct_handle_friend));
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        QDataStream out(&buffer);
        handle_friend.type = DeleteFriend;
        out << handle_friend.type << handle_friend.Myid << handle_friend.Hisid << handle_friend.ifsuccess;
        buffer.close();
        this->SendMessages(data);
    }
    else if (type == SelectFriend)
    {
        struct_Userinfo thisfriend;
        QString Myid;
        in >> Myid;
        in >> thisfriend.userid;
        qDebug() << "Id you want to search is " << thisfriend.userid;
        this->connect_mysql_to_get_hisinfo(thisfriend.userid);
    }
    else if (type == AddFriend)
    {
        struct_Userinfo thisfriend;
        QString Myid;
        in >> Myid;
        in >> thisfriend.userid;
        this->connect_mysql_to_add_friend(Myid, thisfriend.userid);
    }
}

void MainWindow::SendMessages(QByteArray data)
{
    qDebug() << "I am sending the message...";
    this->tcpSocket->waitForBytesWritten();
    this->tcpSocket->write(data);
    this->tcpSocket->flush();
}
void MainWindow::Check_HeartBeat()
{
    for (int i = 0; i < this->Alive_users.count; i++)
    {
        if (this->Alive_users.subtime[i].toInt() > 90)
        {
            qDebug() << "I will drop this user.";
            this->myMapper->removeMappings(Alive_users.ListTcpsocket[i]);
            this->Alive_users.count--;
            this->Alive_users.userid.removeAt(i);
            this->Alive_users.ipaddress.removeAt(i);
            this->Alive_users.port.removeAt(i);
            this->Alive_users.time.removeAt(i);
        }
    }
}
// get ip of this server
QString MainWindow::GetLocalIPAddress()
{
    QString vAddress;
#ifdef _WIN32
    QHostInfo vHostInfo = QHostInfo::fromName(QHostInfo::localHostName());
    QList<QHostAddress> vAddressList = vHostInfo.addresses();
#else
    QList<QHostAddress> vAddressList = QNetworkInterface::allAddresses();
#endif
    for (int i = 0; i < vAddressList.size(); i++)
    {
        if (!vAddressList.at(i).isNull() &&
            vAddressList.at(i) != QHostAddress::LocalHost &&
            vAddressList.at(i).protocol() == QAbstractSocket::IPv4Protocol)
        {
            vAddress = vAddressList.at(i).toString();
            break;
        }
    }

    return vAddress;
}
// sign up check
bool MainWindow::connect_mysql_toverify(QString user_id, QString user_password)
{
    qDebug() << "account：" << user_id;
    QSqlQuery query = QSqlQuery(this->db);
    query.exec("SELECT * FROM Network_user_info;");
    QSqlRecord recode = query.record();
    while (query.next())
    {
        if (query.value("user_id").toString() == user_id)
        {
            if (query.value("password").toString() == user_password)
            {
                return true;
            }
        }
        else
        {
            continue;
        }
    }
    return false;
}
void MainWindow::connect_mysql_to_get_userinfo(QString UserId)
{
    QString user_id = UserId;
    qDebug() << "account：" << user_id;
    QSqlQuery query = QSqlQuery(this->db);
    query.exec("SELECT user_id,user_name,user_email,head_image FROM network_user_info WHERE user_id=" + user_id + "");
    QSqlRecord recode = query.record();
    int count_info = recode.count();
    query.next();
    if (query.value("user_id") == user_id)
    {
        QByteArray data;
        data.resize(sizeof(struct_Userinfo));
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        QDataStream out(&buffer);
        struct_Userinfo User_info;
        User_info.type = Userinfo;
        User_info.username = query.value("user_name").toString();
        User_info.userid = user_id;
        User_info.useremail = query.value("user_email").toString();
        User_info.url_headimg = query.value("head_image").toString();
        out << User_info.type << User_info.username << User_info.userid << User_info.useremail << User_info.url_headimg;
        buffer.close();
        this->SendMessages(data);
    }
    return;
}
bool MainWindow::connect_mysql_to_delete_friend(QString Myid, QString Hisid)
{
    QSqlQuery query = QSqlQuery(this->db);
    query.exec("DELETE FROM network_user_rela WHERE user_id=" + Myid + " AND friend_id=" + Hisid + " OR user_id=" + Hisid + " AND friend_id=" + Myid + "");
    QSqlRecord recode = query.record();
    query.exec("SELECT user_id,friend_id FROM network_user_rela WHERE user_id=" + Myid + " AND friend_id=" + Hisid + " OR user_id=" + Hisid + " AND friend_id=" + Myid + "");
    if (query.next() == NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
    //    db.close();
}
int MainWindow::connect_mysql_getrelalist(QString UserId)
{
    QSqlQuery query = QSqlQuery(this->db);
    QSqlRecord query_recode;
    QSqlQueryModel *queryModel = new QSqlQueryModel();
    queryModel->setQuery(query);
    query_recode.clear();
    query.exec("SELECT user_id,friend_id FROM network_user_rela WHERE user_id=" + UserId + " OR friend_id=" + UserId + ";");
    query_recode = query.record();
    int count_rela = 0;
    while (query.next())
    {
        qDebug() << query.value("user_id");
        count_rela++;
    }
    qDebug() << count_rela;
    QByteArray data;
    data.resize(sizeof(struct_Friendsinfo));
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    QDataStream out(&buffer);
    struct_Friendsinfo Friends;
    Friends.type = Friendsinfo;
    Friends.count = count_rela;
    query.exec("SELECT user_id,friend_id FROM Network_user_rela WHERE user_id=" + UserId + " OR friend_id=" + UserId + ";");
    QString nomean = "";
    Friends.message = nomean;
    while (query.next())
    {
        if (query.value("user_id") == UserId)
        {
            QSqlQuery query_info = QSqlQuery(this->db);
            QString friend_id = query.value("friend_id").toString();
            Friends.userid << friend_id;
            query_info.exec("SELECT user_id,user_name,user_email,head_image FROM network_user_info WHERE user_id=" + friend_id + "");
            QSqlRecord recode_info = query_info.record();
            int count_info = recode_info.count();
            query_info.next();
            Friends.username << query_info.value("user_name").toString();
            Friends.useremail << query_info.value("user_email").toString();
            Friends.url_headimg << query_info.value("head_image").toString();
        }
        else if (query.value("friend_id") == UserId)
        {
            QSqlQuery query_info = QSqlQuery(this->db);
            QString friend_id = query.value("user_id").toString();
            Friends.userid << friend_id;
            query_info.exec("SELECT user_id,user_name,user_email,head_image FROM network_user_info WHERE user_id=" + friend_id + "");
            QSqlRecord recode_info = query_info.record();
            int count_info = recode_info.count();
            query_info.next();
            Friends.username << query_info.value("user_name").toString();
            Friends.useremail << query_info.value("user_email").toString();
            Friends.url_headimg << query_info.value("head_image").toString();
        }
    }
    out << Friends.type << Friends.count << nomean << Friends.message << Friends.userid << Friends.username << Friends.useremail << Friends.url_headimg;
    buffer.close();
    //        db.close();
    this->SendMessages(data);
    return 0;
}
void MainWindow::connect_mysql_to_get_hisinfo(QString UserId)
{
    QString user_id = UserId;
    qDebug() << "account：" << user_id;
    QSqlQuery query = QSqlQuery(this->db);
    query.exec("SELECT user_id,user_name,user_email,head_image FROM network_user_info WHERE user_id=" + user_id + "");
    QSqlRecord recode = query.record();
    int count_info = recode.count();
    query.next();
    if (query.value("user_id") == user_id)
    {
        QByteArray data;
        data.resize(sizeof(struct_Userinfo));
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        QDataStream out(&buffer);
        struct_Userinfo User_info;
        User_info.type = SelectFriend;
        User_info.username = query.value("user_name").toString();
        User_info.userid = user_id;
        User_info.useremail = query.value("user_email").toString();
        User_info.url_headimg = query.value("head_image").toString();
        out << User_info.type << User_info.username << User_info.userid << User_info.useremail << User_info.url_headimg;
        buffer.close();
        this->SendMessages(data);
    }
    return;
}
bool MainWindow::connect_mysql_to_add_friend(QString Myid, QString Hisid)
{
    QSqlQuery query = QSqlQuery(this->db);
    query.exec("SELECT user_id,friend_id FROM network_user_rela WHERE user_id=" + Myid + " AND friend_id=" + Hisid + " OR user_id=" + Hisid + " AND friend_id=" + Myid + "");
    if (query.next() == NULL)
    {
        qDebug() << "inserting！";
        qDebug() << Hisid << Myid;

        query.exec("SELECT MAX(rela_id) FROM network_user_rela");
        QSqlRecord recode = query.record();
        int count_info = recode.count();
        query.next();
        int this_relaid = query.value(0).toInt();
        this_relaid++;
        QString count = QString::number(this_relaid);
        QDateTime curDateTime = QDateTime::currentDateTime();
        query.clear();
        query.prepare("INSERT INTO network_user_rela (rela_id,user_id,friend_id,timestamp) "
                      "values (?,?,?,?)");
        query.addBindValue(this_relaid);
        query.addBindValue(Myid);
        query.addBindValue(Hisid);
        query.addBindValue(curDateTime);
        query.exec();
        query.clear();
        query.exec("SELECT user_id,friend_id FROM network_user_rela WHERE user_id=" + Myid + " AND friend_id=" + Hisid + " OR user_id=" + Hisid + " AND friend_id=" + Myid + "");
        if (query.next() != NULL)
        {
            QByteArray data;
            data.resize(sizeof(struct_handle_friend));
            QBuffer buffer(&data);
            buffer.open(QIODevice::WriteOnly);
            QDataStream out(&buffer);
            struct_handle_friend addfriend;
            addfriend.type = AddFriend;
            addfriend.Myid = Myid;
            addfriend.Hisid = Hisid;
            addfriend.ifsuccess = true;
            out << addfriend.type << addfriend.Myid << addfriend.Hisid << addfriend.ifsuccess;
            buffer.close();
            this->SendMessages(data);
        }
    }
}
