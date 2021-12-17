#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QMessageBox>
#include <QByteArray>
#include <QCryptographicHash>
#include <QTime>
#include <QTimer>
#include <QSignalMapper>
QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    enum MsgType
    {
        Msg,
        UsrLogin,
        UsrLeft,
        Userinfo,
        Friendsinfo,
        HeartBeat,
        LinkFriend,
        File,
        Refuse,
        DeleteFriend,
        AddFriend,
        SelectFriend,
        HelloToMyfriend
    }; //消息类型
    typedef struct
    {
        MsgType type;
        QString userid;
        QString password;
        QString Message;
        bool success;
    } LoginMessage;
    typedef struct
    {
        MsgType type;
        QString username;
        QString userid;
        QString useremail;
        QString url_headimg;
    } struct_Userinfo;
    typedef struct
    {
        MsgType type;
        int count;
        QString clientid;
        QString message;
        QStringList userid;
        QStringList username;
        QStringList useremail;
        QStringList url_headimg;
    } struct_Friendsinfo;
    typedef struct
    {
        int count;
        QStringList userid;
        QStringList ipaddress;
        QStringList port;
        QStringList time;
        QStringList subtime;
        QVector<QTcpSocket *> ListTcpsocket;
    } struct_alive_users;
    typedef struct
    {
        MsgType type;
        QString userid;
        QString ipaddress;
        int port;
        QString Message;
        QTime time;
    } struct_heartbeat;
    typedef struct
    {
        MsgType type;
        QString userid;
        int port;
        QHostAddress ipaddress;
        bool online;
    } struct_Link_friend;
    typedef struct
    {
        MsgType type;
        QString Myid;
        QString Hisid;
        bool ifsuccess;
    } struct_handle_friend;

private:
    Ui::MainWindow *ui;
    QTcpServer *tcpServer;
    QTcpSocket *tcpSocket;
    QString user_id;
    struct_alive_users Alive_users;
    QTimer *timer;
    QSqlDatabase db;
    QSignalMapper *myMapper;
    QString GetLocalIPAddress();
    void SendMessages(QByteArray data);
    void StartServer();
    int connect_mysql_getrelalist(QString UserId);
    bool connect_mysql_toverify(QString user_id, QString user_password);
    void connect_mysql_to_get_userinfo(QString UserId);
    bool connect_mysql_to_delete_friend(QString Myid, QString Hisid);
    void connect_mysql_to_get_hisinfo(QString UserId);
    bool connect_mysql_to_add_friend(QString Myid, QString Hisid);
    void init_Alive();
private slots:
    void newConnect();
    void ReadMessages();
    void Check_HeartBeat();
    void changeTcpsocket(int index);
};
#endif // MAINWINDOW_H
