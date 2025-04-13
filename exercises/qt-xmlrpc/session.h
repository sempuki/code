/* session.h -- session data model and manager
 *
 */

#ifndef incl_Session_h
#define incl_Session_h

#include <QList>
#include <QVariantMap>

namespace Session
{
    enum Type
    {
        LLSessionType,
        TaigaSessionType
    };

    struct Session 
    {
        Session () : ready (false), type (0) {}
        Session (Type t) : ready (false), type (t) {}

        bool ready;
        int type;
        
    };
    
    struct Handler 
    {
        virtual bool accepts (const QVariantMap &params) = 0;
        virtual Session *login (const QVariantMap &params) = 0;

        virtual bool owns (Session *session) = 0;
        virtual bool logout (Session *session) = 0;
    };

    class Manager
    {
        public:
            void Register (Handler *handl);
            Session *Login (QVariantMap params);
            bool Logout (Session *session);

        private:
            QList <Handler *> handlers_;
    };
}

#endif
