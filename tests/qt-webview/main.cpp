
#include <iostream>
#include <QtGui>
#include <QWebView>


//=============================================================================
//
int main (int argc, char **argv)
{
    QApplication app (argc, argv);

    QGraphicsView *view = new QGraphicsView ();
    QGraphicsScene *scene = new QGraphicsScene ();

    QWebView *web = new QWebView ();
    web-> load (QUrl ("http://google.com"));

    QDialog *dialog1 = new QDialog ();
    QDialog *dialog2 = new QDialog ();

    dialog1-> setWindowOpacity (0.8);
    dialog1-> setWindowTitle ("testing baby");
    dialog1-> setLayout (new QVBoxLayout);
    dialog1-> layout()-> addWidget (new QLineEdit);

    dialog2-> setWindowOpacity (0.8);
    dialog2-> setWindowTitle ("you suck");
    dialog2-> setLayout (new QVBoxLayout);
    dialog2-> layout()-> addWidget (new QLineEdit);

    QGraphicsProxyWidget *item1 = scene-> addWidget (dialog1);
    QGraphicsProxyWidget *item2 = scene-> addWidget (dialog2);
    QGraphicsProxyWidget *item3 = scene-> addWidget (web);

    view-> setScene (scene);
    view-> setViewportUpdateMode (QGraphicsView::FullViewportUpdate);

    item1-> setWindowFlags (Qt::Dialog);
    item1-> setFlag (QGraphicsItem::ItemIsMovable);
    item1-> setPos (10, 50);

    item2-> setWindowFlags (Qt::Dialog);
    item2-> setFlag (QGraphicsItem::ItemIsMovable);
    item2-> setPos (10, 100);
    
    item3-> setWindowFlags (Qt::Dialog);
    item3-> setFlag (QGraphicsItem::ItemIsMovable);
    item3-> setPos (10, 150);

    view-> show();
    
    return app.exec();
}
