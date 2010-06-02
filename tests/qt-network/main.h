/* main.h -- main module
 *
 *			Ryan McDougall -- 2009
 */

#include <iostream>
#include <iomanip>

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

using namespace std;


// INFO: QIODevice abstracts any serial data source (QNetworkReply or QByteArray)
// INFO: QSharedPointer calls deleteLater() instad of delete; 

class Asset : public QObject
{
    public:
        Asset (QSharedPointer <QIODevice> data, const QString &id)
            : data_ (data), id_ (id)
        {}

        virtual ~Asset () {}

        virtual QString id () const { return id_; }
        virtual QString type () const { return type_; }
        virtual QSharedPointer <QIODevice> data () { return data_; }
        virtual size_t size () { return data_-> size(); }

    protected:
        QString id_;
        QString type_;
        QSharedPointer <QIODevice> data_;
};

class AssetStatus : public QObject
{
    Q_OBJECT

    public:
        AssetStatus () 
            : active (false), complete (false), 
            received (0), sent (0), size (0),
            errcode (0)
        {}

        AssetStatus (const QString &n) 
            : name (n), active (false), complete (false), 
            received (0), sent (0), size (0),
            errcode (0)
        {}

        virtual ~AssetStatus () {}

    public:
        QString name;
        bool    active;
        bool    complete;
        size_t  received;
        size_t  sent;
        size_t  size;
        int     errcode;

    signals:
        void on_ready ();
        void on_complete ();
        void on_update (size_t curr, size_t total);
        void on_error (int code);
};

class AssetWaiter : QObject
{
    Q_OBJECT

    public:
        AssetWaiter (QSharedPointer <AssetStatus> status, size_t time = 0) 
            : timeout (false), exit (false)
        { 
            connect (status.data(), SIGNAL (on_complete()), this, SLOT (on_complete()));

            if (time) // timeout in ms
                startTimer (time);

            while (!timeout && !exit) 
                QCoreApplication::processEvents (QEventLoop::WaitForMoreEvents);
        }

    protected slots:
        void on_complete () { exit = true; }
        void timerEvent (QTimerEvent *e) { timeout = true; }

    protected:
        bool timeout;
        bool exit;
};

class AssetFuture
{
    public:
        typedef QSharedPointer <Asset>          AssetPtr;
        typedef QSharedPointer <AssetStatus>    StatusPtr;
        
        AssetFuture ()
        {}

        AssetFuture (StatusPtr status, AssetPtr asset)
            : status_ (status), asset_ (asset)
        {}

        virtual ~AssetFuture () {}

        operator bool () const { return (status_); }
        int Error () const { return status_-> errcode; }
        bool Complete () const { return !(status_-> errcode) && (status_-> complete); }
        
        StatusPtr GetStatus () const { return status_; }
        AssetPtr GetAsset () const { return asset_; }
        AssetPtr WaitForAsset () { AssetWaiter (status_, 100); return asset_; } 

    protected:
        StatusPtr   status_;
        AssetPtr    asset_;
};

class AssetListener : public QObject
{
    Q_OBJECT

    public:
        AssetListener (const AssetFuture &future)
            : status (future.GetStatus()), asset (future.GetAsset())
        {
            if (status)
            {
                connect (status.data(), SIGNAL (on_ready()), this, SLOT (on_ready()));
                connect (status.data(), SIGNAL (on_complete()), this, SLOT (on_complete()));
                connect (status.data(), SIGNAL (on_update(size_t,size_t)), this, SLOT (on_update(size_t,size_t)));
                connect (status.data(), SIGNAL (on_error(int)), this, SLOT (on_error(int)));
            }
        }

    protected slots:
        virtual void on_ready () = 0;
        virtual void on_complete () = 0;
        virtual void on_update (size_t curr, size_t total) = 0;
        virtual void on_error (int code) = 0;

    protected:
        QSharedPointer <AssetStatus> status;
        QSharedPointer <Asset> asset;
};
 
// TODO: parse MIME type to record asset type
class HttpAsset : public Asset
{
    public:
        HttpAsset (QSharedPointer <QNetworkReply> reply)
            : Asset (reply, reply-> url().toString())
        {}
};

// INFO: copies will not connect signals
class HttpAssetStatus : public AssetStatus
{
    Q_OBJECT

    public:
        HttpAssetStatus (QSharedPointer <QNetworkReply> r)
            : AssetStatus (r-> url().toString()), reply (r)
        {
            connect_signals_ ();
        }

    public:
        QSharedPointer <QNetworkReply>  reply;
    
    protected slots:
        virtual void ready_read () 
        {
            active = true;
            emit on_ready ();
        }

        virtual void metadata_changed () {}

        virtual void download_progress (qint64 curr, qint64 total)
        {
            received = curr;
            size = total;
            emit on_update (received, size);
        }

        virtual void finished () 
        { 
            complete = true; 
            emit on_complete ();
        }

        virtual void error (QNetworkReply::NetworkError code)
        { 
            errcode = code;
            complete = true;
            emit on_error (errcode);
        }

        virtual void ssl_errors (const QList <QSslError> &errs)
        {
            complete = true;
        }

    private:
        void connect_signals_ ()
        {
            connect (reply.data(), SIGNAL (readyRead()), this, SLOT (ready_read()));
            connect (reply.data(), SIGNAL (metaDataChanged()), this, SLOT (metadata_changed()));
            connect (reply.data(), SIGNAL (downloadProgress(qint64,qint64)), this, SLOT (download_progress(qint64,qint64)));
            connect (reply.data(), SIGNAL (finished()), this, SLOT (finished()));
            connect (reply.data(), SIGNAL (error(QNetworkReply::NetworkError)), this, SLOT (error(QNetworkReply::NetworkError)));
            connect (reply.data(), SIGNAL (sslErrors(const QList<QSslError>&)), this, SLOT (ssl_errors(const QList<QSslError>&)));
        }
};

class HttpAssetProvider : public QObject
{
    Q_OBJECT

    public:
        HttpAssetProvider ()
            : http (new QNetworkAccessManager (this)), cache (new QNetworkDiskCache (this))
        { 
            setup_cache_ ();
            connect_signals_ ();

            cb_asset_re.setPattern ("[/]assets/[a-zA-Z\\d\\-]{36}");
        }

        bool Accept (const QString &id) { return Accept (QUrl (id)); }
        bool Accept (const QUrl &id) { return id.isValid() && (id.scheme() == "http"); }

        AssetFuture Request (const QString &id) 
        {
            // TODO: metadata http://opensimulator.org/wiki/AssetServerProposal/ClientDocs

            QUrl url (id);

            if (cb_asset_re.exactMatch (url.path())) // CB-style asset "protocol"
                url.setPath (url.path() + "/data");

            if (!Accept (url))
                return AssetFuture ();

            QSharedPointer <AssetStatus> status (get_status_ (url));
            QSharedPointer <Asset> asset (get_asset_ (url));
            return AssetFuture (status, asset);
        }

    protected slots:
        void reply_finished (QNetworkReply *reply)
        {
            progress.remove (reply-> url().toString());
        }

        void authentication_required (QNetworkReply *reply, QAuthenticator *auth)
        {
            cout << "authentication required!!" << endl;
        }

        void proxy_authentication_required (const QNetworkProxy &proxy, QAuthenticator *auth)
        {
            cout << "proxy authentication required!!" << endl;
        }

        void ssl_errors (QNetworkReply *reply, const QList <QSslError> &errs)
        {
            cout << "ssl errors!!" << endl;
        }

    private:
        QSharedPointer <AssetStatus> get_status_ (const QUrl &id)
        {
            QString url (id.toString());

            if (progress.contains (url))
                return progress [url];
            
            QNetworkRequest request (url);
            QSharedPointer <QNetworkReply> reply (http-> get (request));
            QSharedPointer <HttpAssetStatus> status (new HttpAssetStatus (reply));

            progress [url] = status;
            return status;
        }

        QSharedPointer <Asset> get_asset_ (const QUrl &id)
        {
            QString url (id.toString());

            QSharedPointer <QIODevice> data (cache-> data (url));
            if (data) return QSharedPointer <Asset> (new Asset (data, url));

            QSharedPointer <HttpAssetStatus> status (progress [url]);
            if (status) return QSharedPointer <Asset> (new HttpAsset (status-> reply));

            return QSharedPointer <Asset> ();
        }

        void setup_cache_ ()
        {
            cache-> setCacheDirectory ("asset-cache-http"); // TODO: correct dir
            cache-> setMaximumCacheSize (200*1024*1024); // 200MB
            http-> setCache (cache);
        }

        void connect_signals_ ()
        {
            connect (http, SIGNAL (finished(QNetworkReply*)), 
                    this, SLOT (reply_finished(QNetworkReply*)));
            connect (http, SIGNAL (authenticationRequired(QNetworkReply*,QAuthenticator*)), 
                    this, SLOT (authentication_required(QNetworkReply*,QAuthenticator*)));
            connect (http, SIGNAL (proxyAuthenticationRequired(const QNetworkProxy&,QAuthenticator*)), 
                    this, SLOT (proxy_authentication_required(const QNetworkProxy&,QAuthenticator*)));
            connect (http, SIGNAL (sslErrors(QNetworkReply*,const QList<QSslError>&)), 
                    this, SLOT (ssl_errors(QNetworkReply*,const QList<QSslError>&)));
        }

    private:
        QNetworkDiskCache       *cache;
        QNetworkAccessManager   *http;
        QRegExp                 cb_asset_re;

        QMap <QString, QSharedPointer <HttpAssetStatus> >   progress;
};

struct TestAssetListener : public AssetListener
{
    TestAssetListener (const AssetFuture &future) 
        : AssetListener (future) 
    {}

    void on_ready ()
    {
        cout << "== on_ready (" << qPrintable (asset-> id()) << ")" << endl;
    }

    void on_complete ()
    {
        cout << "== on_complete (" << qPrintable (asset-> id()) << ")" << endl;
    }

    void on_update (size_t curr, size_t total)
    {
        cout << "== on_update (" << qPrintable (asset-> id()) << ") : " << curr << " / " << total << endl;
    }

    void on_error (int code)
    {
        cout << "== on_error (" << qPrintable (asset-> id()) << ") : " << code << endl;
    }
};

// INFO: mimick the naali main loop
class Test : public QApplication
{
    Q_OBJECT 

    public:
        typedef QSharedPointer <TestAssetListener> AssetListenerPtr;

        Test (int &argc, char** argv)
            : QApplication (argc, argv), complete (0)
        {
            // INFO: set up test asset requests

            ids << "adec83e1-6031-4e3c-97db-abd8a84c7ca0"
                << "http://example.com:8004/assets/adec83e1-6031-4e3c-97db-abd8a84c7ca0"
                << "http://example.com/~ryanm/Jack.mesh"
                << "http://example.com/~ryanm/Jill.mesh"
                << "http://example.com/~ryanm/arcticfox.jpg";

            foreach (QString id, ids)
            {
                cout << "getting " << qPrintable (id) << endl;

                if (provider.Accept (id))
                {
                    futures.push_back (provider.Request (id));
                    listeners.push_back (AssetListenerPtr (new TestAssetListener (provider.Request (id))));
                }
            }
        }

        // INFO: set up update loop
        int Go ()
        {
            QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(Update()));

            set_timer();
            return exec ();
        }

    public slots:

        // INFO: main update loop
        void Update()
        {
            cout << "do X" << endl;
            bool doread = true;

            foreach (AssetFuture fut, futures)
            {
                if (fut) 
                {
                    if (doread)
                    {
                        // pump event loop until the asset is downloaded (or timed out)
                        AssetFuture::AssetPtr p = fut.WaitForAsset (); 

                        if (fut.Complete())
                        {
                            p-> data()-> read (buffer, 1024);

                            for (int i=0; i < 5; i++)
                                cout << hex << (short) buffer[i] << ", ";
                            cout << endl;
                        }
                    }


                    if (fut.Complete())
                    {
                        cout << "** completed retrieval of " << qPrintable (fut.GetStatus()->name) << endl;
                        ++ complete;
                    }
                    else if (fut.Error())
                        cout << "** errored retrieval of " << qPrintable (fut.GetStatus()->name) << endl;
                    else
                        cout << "** incomplete retrieval of " << qPrintable (fut.GetStatus()->name) << endl;
                }
                else
                    cout << "** invalid future for " << qPrintable (fut.GetStatus()->name) << endl;
            }

            if (complete >= 2) 
                quit();

            set_timer();
            
            cout << "do Y" << endl;
        }

    protected:
        void set_timer()
        {
            timer.setSingleShot (true);
            timer.start (0); 
        }

        QTimer timer;
        HttpAssetProvider provider;
        
        QList <QString>             ids;
        QList <AssetFuture>         futures;
        QList <AssetListenerPtr>    listeners;

        int complete;
        char buffer [1024];
};
