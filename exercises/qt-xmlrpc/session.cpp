/* session.h -- session data model and manager
 *
 */

#include "session.h"

namespace Session
{
    void Manager::Register (Handler *handl)
    {
        handlers_.push_back (handl);
    }

    Session *Manager::Login (QVariantMap params)
    {
        // parameters received from UI as strings
        // handlers volunteer to accept parameters

        Handler *accepted = 0;
        foreach (Handler *hdl, handlers_)
            if (hdl-> accepts (params))
                accepted = hdl;

        if (!accepted)
            return 0;

        return accepted-> login (params);
    }

    bool Manager::Logout (Session *session)
    {
        // session point received from logic
        // handlers claim ownership of session

        Handler *owner = 0;
        foreach (Handler *hdl, handlers_)
            if (hdl-> owns (session))
                owner = hdl;

        if (!owner)
            return false;

        return owner-> logout (session);
    }
}
