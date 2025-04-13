/****************************************************************************
 **
 ** Copyright (C) Ryan McDougall, Qxt Foundation. Some rights reserved.
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of the Common Public License, version 1.0, as published
 ** by IBM, and/or under the terms of the GNU Lesser General Public License,
 ** version 2.1, as published by the Free Software Foundation.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL and the LGPL along with this
 ** file. See the LICENSE file and the cpl1.0.txt/lgpl-2.1.txt files
 ** included with the source distribution for more information.
 ** If you did not receive a copy of the licenses, contact the Qxt Foundation.
 **
 ****************************************************************************/

#ifndef _MAIN_H_
#define _MAIN_H_


#include <iostream>
#include "llsession.h"

class TestApp : QObject
{
    Q_OBJECT

    public:
        TestApp ()
            : session (0)
        {
            sessions.Register (new LLSessionHandler);

            QVariantMap ui_login_params;
            session = sessions.Login (ui_login_params);

            LLSession *llsession;
            if (session-> type == Session::LLSessionType)
                llsession = static_cast <LLSession *> (session);

            startTimer (1000);
        }

        void timerEvent (QTimerEvent *e)
        {
            std::cout << "session is ";

            if (session-> ready) 
                std::cout << "ready\n";
            else
                std::cout << "not ready\n";
        }

    private:
        bool connected;
        Session::Manager sessions;
        Session::Session *session;
};

#endif
