/* Copyright Ryan McDougall 2009 -- for realXtend project (http://www.realxtend.org)
 * available under Apache 2.0 License (http://www.apache.org/licenses/LICENSE-2.0.html)
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include <QtCore>
#include <QtGui>
#include <QWebView>
#include <QThread>

#ifndef Q_WS_WIN
#include <QX11Info>
#endif

#include <Ogre.h>
#include <OgreConfigFile.h>

using Ogre::Real;
using Ogre::Radian;
using Ogre::Vector3;
using Ogre::ColourValue;

class WorldModel
{
    public:
        WorldModel (Ogre::SceneManager *sm);
        virtual ~WorldModel ();

    private:
        Ogre::SceneManager      *scenemgr_;
        Ogre::Camera            *camera_;

        Ogre::SceneNode		    *planenode_;

        friend class WorldView;
        friend class WorldController;
};


class WorldController : public Ogre::FrameListener
{
    public:
        WorldController (WorldModel *model) 
            : model_ (model) {}
        
        bool frameStarted (const Ogre::FrameEvent& evt);

    private:
        WorldModel          *model_;
};


class WorldView
{
    public:
        WorldView (WorldModel *model, Ogre::RenderWindow *win);
        virtual ~WorldView ();

        void InitializeOverlay (int width, int height);

        void ResizeWindow (int width, int height);
        void ResizeOverlay (int width, int height);

        void RenderOneFrame ();
        void OverlayUI (Ogre::PixelBox &ui);

    private:

        Ogre::Root              *root_;

        Ogre::Viewport          *view_;
        Ogre::RenderWindow      *win_;

        Ogre::TexturePtr        ui_overlay_texture_;
        Ogre::Overlay           *ui_overlay_;
        Ogre::OverlayElement    *ui_overlay_container_;

        WorldModel              *model_;
};

class QOgreUIView : public QGraphicsView
{
    Q_OBJECT

    public:
        QOgreUIView ();
        QOgreUIView (QGraphicsScene *scene);
        virtual ~QOgreUIView ();

        void SetWorldView (WorldView *view) { view_ = view; }
        Ogre::RenderWindow *CreateRenderWindow ();

    protected:
        void resizeEvent (QResizeEvent *e);

    private:
        void initialize_ ();

        Ogre::RenderWindow  *win_;
        WorldView           *view_;
};


class Ogre3DApplication
{
    public:
        Ogre3DApplication (QOgreUIView *window);
        virtual ~Ogre3DApplication ();

        WorldView *GetView () { return view_; }

        void Run () { root_-> startRendering(); }

    protected:
        void setup_resources ();

    private:

        Ogre::Root          *root_;
        Ogre::Camera        *camera_;
        Ogre::SceneManager  *scenemgr_;
        Ogre::RenderWindow  *win_;

        WorldModel          *model_;
        WorldController     *controller_;
        WorldView           *view_;
};

class QtApplication : public QApplication
{
    Q_OBJECT

    public:
        QtApplication (int &argc, char **argv);
        ~QtApplication();

        QOgreUIView *GetView () { return view_; }

    private:
        QOgreUIView     *view_;
        QGraphicsScene  *scene_;
};

class ApplicationLoop : public QObject
{
    Q_OBJECT

    public:
        ApplicationLoop (QGraphicsView *uiview, WorldView *world);

    protected:
        void timerEvent (QTimerEvent *e);

    private:
        void get_input_ ();
        void render_ ();

        QGraphicsView           *uiview_;
        WorldView               *worldview_;
};

#endif
