/* llsession.h -- llsession implementation
 *
 */

#ifndef incl_LLSession_h
#define incl_LLSession_h

#include <QString>
#include <QList>
#include <QUrl>
#include <QMap>
#include <QVariantMap>

#include "xmlrpc.h"
#include "capabilities.h"
#include "session.h"

class QNetworkAccessManager;
class QNetworkReply;

void variant_print (const QVariant &v, const QString &p);
void variant_print (const QVariantMap &m, const QString &p);
void variant_print (const QVariantList &l, const QString &p);
void variant_print (const QStringList &l, const QString &p);
void variant_print (const QString &s, const QString &p);
void variant_print (bool v, const QString &p);
void variant_print (int v, const QString &p);

//=========================================================================
// LL Buddies

struct LLBuddy
{
    QString buddy_id;
    int buddy_rights_given;
    int buddy_rights_has;
};

struct LLBuddyList
{
    QList <LLBuddy> list;
};

//=========================================================================
// LL Inventory Skeleton

struct LLFolderSkeleton
{
    QString name;
    QString folder_id;
    QString parent_id;
    int     type_default;
    int     version;
};

struct LLInventorySkeleton
{
    QString owner;
    LLFolderSkeleton root;
    QList <LLFolderSkeleton> folders;
};

//=========================================================================
// Parameters used in LLStream

struct LLStreamParameters
{
    QString agent_id;
    QString session_id;
    int circuit_code;
};

//=========================================================================
// Parameters used in LLSession

struct LLSessionParameters
{
    QString message;

    QString sim_ip;
    int     sim_port;

    QUrl seed_capabilities;
    QMap <QString, QUrl> capabilities;
};


//=========================================================================
// Parameters used by LLAgent

struct LLAgentParameters
{
    QString      first_name;
    QString      last_name;

    LLBuddyList         buddies;
    LLInventorySkeleton inventory;

    QString     home;
    QString     look_at;
    QString     start_location;
    int         region_x;
    int         region_y;
};

//=========================================================================
// Parameters used by LLLogin

struct LLLoginParameters
{
    QString first;
    QString last;
    QString pass;
    QUrl    service;
};

//=========================================================================
// LL Session (Future)

struct LLSession : public Session::Session
{
    LLSession () : Session (::Session::LLSessionType) {}
    
    LLAgentParameters   agent_parameters;
    LLSessionParameters session_parameters;
    LLStreamParameters  stream_parameters;

};

//=========================================================================
// Login object for LLSession

class LLLogin : public QObject
{
    Q_OBJECT

    public:
        LLLogin ();
        ~LLLogin ();

        Session::Session *operator() (const LLLoginParameters &params);

    public slots:
        void on_login_result ();
        void on_caps_result ();

    public:
        LLSession               *session;
        QNetworkAccessManager   *http;

        XmlRpc::Call            *login;
        Capabilities::Caps      *caps;
};

//=========================================================================
// Login object for LLSession

struct LLLogout
{
    bool operator() (Session::Session *session) {}
};

//=========================================================================
// Handler for LLSession

class LLSessionHandler : public Session::Handler
{
    public:
        bool accepts (const QVariantMap &params);
        Session::Session *login (const QVariantMap &params);
        bool owns (Session::Session *session);
        bool logout (Session::Session *session);

    private:
        LLLogin login_;
        LLLogout logout_;
};

#endif
