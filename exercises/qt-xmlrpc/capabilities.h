/* capabilities.h -- Qt-based implementation of Capabilities
 *
 */


#ifndef incl_Capabilities_h
#define incl_Capabilities_h

#include <QString>
#include <QMap>
#include <QUrl>
#include <QNetworkAccessManager>

namespace Capabilities
{
    QMap <QString, QUrl> parse_capabilities (QNetworkReply *reply);

    //=========================================================================
    // Parse Caps request automatically on finish

    class Caps : public QObject
    {
        Q_OBJECT

        public:
            Caps (QNetworkReply *r);
            ~Caps ();

            QNetworkReply           *reply;
            QMap <QString, QUrl>    result;

        public slots:
            void on_result ();
    };

    //=========================================================================
    // Get capabilities for a given seed cap
    
    class Client : public QObject
    {
        public:
            Client (const QUrl &seed, QNetworkAccessManager *http = 0);

            Caps *request ();

        private:
            QUrl                    seed_;
            QNetworkAccessManager   *http_;
    };
}

#endif
