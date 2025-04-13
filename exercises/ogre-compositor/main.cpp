/* Copyright Ryan McDougall 2009 -- for realXtend project (http://www.realxtend.org)
 * available under Apache 2.0 License (http://www.apache.org/licenses/LICENSE-2.0.html)
 */

#include "main.h"

#ifdef Q_WS_WIN
#ifdef _DEBUG
    #define DEBUG_MODE 1
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>
    #include <QtDebug>
#else
    #define DEBUG_MODE 0
#endif
#endif

#include <QtDebug>

//=============================================================================
// Keyboard State

KeyState::KeyState () : 
    key (0), 
    modifiers (0) 
{
}

KeyState::KeyState (int k, unsigned int m) : 
    key (k), modifiers (m)
{
}

KeyState::KeyState (const KeyState &c) : 
    key (c.key), 
    modifiers (c.modifiers) 
{
}

KeyState::KeyState (const QKeyEvent *e) : 
    key (e-> key()), 
    modifiers (e-> modifiers()) 
{
}

KeyState &KeyState::operator= (const KeyState &rhs) 
{ 
    key = rhs.key; 
    modifiers = rhs.modifiers; 
    return *this; 
}

bool KeyState::operator== (const KeyState &rhs)
{
    return (key == rhs.key);
}

bool KeyState::operator== (int k)
{
    return (key == k);
}

//=============================================================================
// Current State of the Keyboard

KeyStateListener::KeyStateListener ()
{
    state_.reserve (10); // we presume users only have 10 fingers
}

char KeyStateListener::KeyToAscii (const KeyState &k)
{
    if ((k.key > 0x1f) && (k.key < 0x7f))
    { // ASCII
        char c (k.key);
        if ((c > 0x40) && (c < 0x5b)) // upper
            if (!(k.modifiers & 0x02000000))
                c |= 0x20; // to lower
        return c;
    }
    else return 0;
}

KeyState KeyStateListener::AsciiToKey (char c)
{
    KeyState k;
    if ((c > 0x1f) && (c < 0x7f))
    { // printable characters
        if ((c > 0x40) && (c < 0x5b)) // upper
            k.modifiers |= 0x02000000;

        if ((c > 0x61) && (c < 0x7b)) // lower
            c &= 0xdf; // to upper
    }
    return k;
}

const std::vector<KeyState> &KeyStateListener::GetState () 
{ 
    return state_; 
}

const std::vector<char> KeyStateListener::GetCharState () 
{ 
    std::vector <char> charstate;

    std::vector <KeyState>::iterator i (state_.begin());
    for (; i != state_.end(); ++i)
        charstate.push_back (KeyToAscii (*i));

    return charstate; 
}

bool KeyStateListener::IsKeyDown (const KeyState &key) 
{ 
    return is_key_pressed_ (key); 
}

bool KeyStateListener::IsKeyDown (int key, unsigned int modifiers) 
{ 
    return is_key_pressed_ (KeyState (key, modifiers)); 
}

bool KeyStateListener::eventFilter (QObject *obj, QEvent *event)
{
    switch (event-> type())
    {
        case QEvent::KeyPress:
            press_key_ (KeyState (static_cast <QKeyEvent *> (event)));
            return false;

        case QEvent::KeyRelease:
            release_key_ (KeyState (static_cast <QKeyEvent *> (event)));
            return false;

        default:
            return QObject::eventFilter (obj, event);
    }
}

bool KeyStateListener::is_key_pressed_ (const KeyState &k)
{
    return (std::find (state_.begin(), state_.end(), k) != state_.end());
}

void KeyStateListener::press_key_ (const KeyState &k)
{ 
    state_.push_back (k); 
}

void KeyStateListener::release_key_ (const KeyState &k)
{ 
    state_.erase (std::find (state_.begin(), state_.end(), k)); 
}


//=============================================================================
// WorldModel

WorldModel::WorldModel (Ogre::SceneManager *sm) : 
    scenemgr_ (sm)
{
    // Create the camera
    camera_ = scenemgr_->createCamera("PlayerCam");

    // Position it at 500 in Z direction
    camera_->setPosition(Vector3(0,0,500));

    // Look back along -Z
    camera_->lookAt(Vector3(0,0,-300));
    camera_->setNearClipDistance(5);

    // Set ambient light
    scenemgr_->setAmbientLight(ColourValue(0.2f, 0.2f, 0.2f));

    // Create a light
    scenemgr_->createLight("MainLight")->setPosition(20, 80, 50);

    // Position the camera
    camera_->setPosition(60, 200, 70);
    camera_->lookAt(0, 0, 0);

    // Create a simple plane
    Ogre::MovablePlane *plane = new Ogre::MovablePlane ("Plane");
    plane->d = 0;
    plane->normal = Vector3::UNIT_Y;

    Ogre::MeshManager::getSingleton().
        createPlane ("PlaneMesh", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
                *plane, 120, 120, 1, 1, true, 1, 1, 1, Vector3::UNIT_Z);

    Ogre::Entity *planeent = scenemgr_-> createEntity ("PlaneEntity", "PlaneMesh");
    planeent-> setMaterialName ("PlaneMat");

    // Attach the plane to a scene node
    planenode_ = scenemgr_-> getRootSceneNode()-> createChildSceneNode ();
    planenode_-> attachObject (planeent);
}

WorldModel::~WorldModel ()
{
}


//=============================================================================
// rotate the plane on every frame

bool WorldController::frameStarted (const Ogre::FrameEvent& evt)
{
    model_-> planenode_-> yaw (Radian (evt.timeSinceLastFrame));

    return Ogre::FrameListener::frameStarted (evt);
}


//=============================================================================
//

WorldView::WorldView (WorldModel *model, Ogre::RenderWindow *win) :
    model_ (model), 
    win_ (win)
{
    root_ = Ogre::Root::getSingletonPtr();
    
    // Create one viewport, entire window
    view_ = win_-> addViewport (model_->camera_);
    view_-> setBackgroundColour (ColourValue (0,0,0));

    // Alter the camera aspect ratio to match the viewport
    model_-> camera_-> setAspectRatio 
        (Real (view_-> getActualWidth()) / 
         Real (view_-> getActualHeight()));

}

void WorldView::InitializeOverlay (int width, int height)
{ 
    // set up off-screen texture
    ui_overlay_texture_ =
        Ogre::TextureManager::getSingleton().createManual
        ("test/texture/UI",
         Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
         Ogre::TEX_TYPE_2D, width, height, 0, 
         Ogre::PF_A8R8G8B8, Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

    Ogre::MaterialPtr material
        (Ogre::MaterialManager::getSingleton().create
         ("test/material/UI", 
          Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));

    Ogre::TextureUnitState *state 
        (material->getTechnique(0)->getPass(0)->createTextureUnitState());

    state-> setTextureName ("test/texture/UI");

    material->getTechnique(0)->getPass(0)->setSceneBlending
        (Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);

    // set up overlays
    ui_overlay_ = 
        (Ogre::OverlayManager::getSingleton().create 
         ("test/overlay/UI"));

    ui_overlay_container_ =
        (Ogre::OverlayManager::getSingleton().createOverlayElement 
         ("Panel", "test/overlay/UIPanel"));

    ui_overlay_container_-> setMaterialName ("test/material/UI");
    ui_overlay_container_-> setMetricsMode (Ogre::GMM_PIXELS);

    ui_overlay_-> add2D (static_cast <Ogre::OverlayContainer *> (ui_overlay_container_));
    ui_overlay_-> show ();

    ResizeOverlay (width, height);
}

WorldView::~WorldView ()
{
}

void WorldView::ResizeWindow (int width, int height)
{
    if (win_)
    {
        win_-> resize (width, height); 
        win_-> windowMovedOrResized ();
    }
}

void WorldView::ResizeOverlay (int width, int height)
{
    if (Ogre::TextureManager::getSingletonPtr() &&
            Ogre::OverlayManager::getSingletonPtr())
    {
        // recenter the overlay
        int left = (win_-> getWidth() - width) / 2;
        int top = (win_-> getHeight() - height) / 2;

        // resize the container
        ui_overlay_container_-> setDimensions (width, height);
        ui_overlay_container_-> setPosition (left, top);

        // resize the backing texture
        ui_overlay_texture_-> freeInternalResources ();
        ui_overlay_texture_-> setWidth (width);
        ui_overlay_texture_-> setHeight (height);
        ui_overlay_texture_-> createInternalResources ();
    }
}

void WorldView::RenderOneFrame ()
{
    root_-> _fireFrameStarted ();
    win_-> update();
    root_-> _fireFrameRenderingQueued ();
    root_-> _fireFrameEnded ();
}

void WorldView::OverlayUI (Ogre::PixelBox &ui)
{
    ui_overlay_texture_-> getBuffer()-> blitFromMemory (ui);
}


//=============================================================================
//

QOgreUIView::QOgreUIView () :
    QGraphicsView (),
    win_ (0),
    view_ (0)
{
    initialize_ ();
}

QOgreUIView::QOgreUIView (QGraphicsScene *scene) : 
    QGraphicsView (scene),
    win_ (0),
    view_ (0)
{
    initialize_ ();
}

QOgreUIView::~QOgreUIView ()
{
}

void QOgreUIView::initialize_ ()
{
    setUpdatesEnabled (false); // reduces flicker and overpaint
    //setAttribute (Qt::WA_PaintOnScreen); // fixes input (!) issues on X11
    setAttribute (Qt::WA_PaintOutsidePaintEvent); // not required?
    setAttribute (Qt::WA_NoSystemBackground); // not required?

    setFocusPolicy (Qt::StrongFocus);
    setViewportUpdateMode (QGraphicsView::FullViewportUpdate);

    setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
}    

Ogre::RenderWindow *QOgreUIView::CreateRenderWindow ()
{
    bool stealparent 
        ((parentWidget())? true : false);

    QWidget *nativewin 
        ((stealparent)? parentWidget() : this);

    Ogre::NameValuePairList params;
    Ogre::String winhandle;

#ifdef Q_WS_WIN
    // According to Ogre Docs
    // positive integer for W32 (HWND handle)
    winhandle = Ogre::StringConverter::toString 
        ((unsigned int) 
         (nativewin-> winId ()));

    //Add the external window handle parameters to the existing params set.
    params["externalWindowHandle"] = winhandle;
#endif

#ifdef Q_WS_X11
    // GLX - According to Ogre Docs:
    // poslong:posint:poslong:poslong (display*:screen:windowHandle:XVisualInfo*)
    QX11Info info =  x11Info ();

    winhandle  = Ogre::StringConverter::toString 
        ((unsigned long)
         (info.display ()));
    winhandle += ":";

    winhandle += Ogre::StringConverter::toString 
        ((unsigned int)
         (info.screen ()));
    winhandle += ":";
    
    winhandle += Ogre::StringConverter::toString 
        ((unsigned long)
         nativewin-> winId());

    //Add the external window handle parameters to the existing params set.
    params["parentWindowHandle"] = winhandle;
#endif

    QSize size (nativewin-> size());
    win_ = Ogre::Root::getSingletonPtr()-> 
        createRenderWindow ("View", size.width(), size.height(), false, &params);

    // take over ogre window
    // needed with parent windows
    if (stealparent)
    {
        WId ogre_winid = 0x0;
#ifndef Q_WS_WIN
        win_-> getCustomAttribute ("WINDOW", &ogre_winid);
#else
        win_-> getCustomAttribute ("HWND", &ogre_winid);
#endif
        assert (ogre_winid);
        create (ogre_winid);
    }

    return win_;
}

void QOgreUIView::resizeEvent (QResizeEvent *e)
{
    QGraphicsView::resizeEvent (e);

    // resize render window and UI texture to match this
    if (view_) 
    {
        view_-> ResizeWindow (width(), height());
        view_-> ResizeOverlay (viewport()-> width(), viewport()-> height());
    }
}


//=============================================================================

Ogre3DApplication::Ogre3DApplication (QOgreUIView *uiview)
{
    std::string plugincfg; 
#ifdef Q_WS_WIN
    if (DEBUG_MODE)
        plugincfg = "plugins_win_d.cfg";
    else
        plugincfg = "plugins_win.cfg";
#elif defined Q_WS_X11
    plugincfg = "plugins.cfg";
#endif
    
    root_ = new Ogre::Root (plugincfg);

    setup_resources ();
    root_-> restoreConfig();

    root_-> initialise (false);
    win_ = uiview-> CreateRenderWindow ();

    scenemgr_ = root_-> createSceneManager (Ogre::ST_GENERIC, "SceneManager");

    Ogre::ResourceGroupManager::getSingleton().  initialiseAllResourceGroups ();

    model_ = new WorldModel (scenemgr_);
    controller_ = new WorldController (model_);
    view_ = new WorldView (model_, win_);

    uiview-> SetWorldView (view_);

    root_-> addFrameListener (controller_);
}

Ogre3DApplication::~Ogre3DApplication ()
{
    delete controller_;
    delete view_;
    delete model_;

    delete root_;
}

void Ogre3DApplication::setup_resources ()
{
    Ogre::String sec_name, type_name, arch_name;
    Ogre::ConfigFile cf;

#ifdef Q_WS_WIN
    cf.load ("resources_win.cfg");
#elif defined Q_WS_X11
    cf.load ("resources.cfg");
#endif

    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();
    while (seci.hasMoreElements())
    {
        sec_name = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            type_name = i->first;
            arch_name = i->second;

            Ogre::ResourceGroupManager::getSingleton().
                addResourceLocation (arch_name, type_name, sec_name);
        }
    }
}


//=============================================================================
//

QtApplication::QtApplication (int &argc, char **argv) : 
    QApplication (argc, argv)
{
    view_ = new QOgreUIView ();
    scene_ = new QGraphicsScene ();

    QWebView *web = new QWebView ();
    web-> load (QUrl ("http://google.com"));

    QDialog *dialog1 = new QDialog ();
    QDialog *dialog2 = new QDialog ();

    dialog1-> setWindowOpacity (0.8);
    dialog1-> setWindowTitle ("Testing baby");
    dialog1-> setLayout (new QVBoxLayout());
    dialog1-> layout()-> addWidget (new QLineEdit());

    dialog2-> setWindowOpacity (0.8);
    dialog2-> setWindowTitle ("You suck");
    dialog2-> setLayout (new QVBoxLayout());
    dialog2-> layout()-> addWidget (new QLineEdit());

    QGraphicsProxyWidget *item1 = scene_-> addWidget (dialog1);
    QGraphicsProxyWidget *item2 = scene_-> addWidget (dialog2);
    QGraphicsProxyWidget *item3 = scene_-> addWidget (web);
    
    view_-> setScene (scene_);

    item1-> setWindowFlags (Qt::Dialog);
    item1-> setFlag (QGraphicsItem::ItemIsMovable);
    item1-> setPos (10, 50);

    item2-> setWindowFlags (Qt::Dialog);
    item2-> setFlag (QGraphicsItem::ItemIsMovable);
    item2-> setPos (50, 50);

    item3-> setWindowFlags (Qt::Dialog);
    item3-> setFlag (QGraphicsItem::ItemIsMovable);
    item3-> setPos (10, 150);

    view_-> resize (1024, 768);
    view_-> show ();

    scene_-> setSceneRect (view_-> rect()); // non-default bounding rect
}

QtApplication::~QtApplication()
{
    delete view_;
    delete scene_;

#ifdef Q_WS_WIN
    _CrtDumpMemoryLeaks();
#endif
}

//=============================================================================
//

ApplicationLoop::ApplicationLoop (QGraphicsView *uiview, WorldView *world) : 
    uiview_ (uiview), worldview_ (world)
{
    uiview_-> installEventFilter (&keylistener_);

    worldview_-> InitializeOverlay 
        (uiview_-> viewport()-> width(), uiview_-> viewport()-> height());

    startTimer (20);
}

void ApplicationLoop::timerEvent (QTimerEvent *e) 
{ 
    get_input_ ();
    render_ ();
}

void ApplicationLoop::get_input_ ()
{
    if (!uiview_-> scene()-> focusItem())
    { // no UI item has focus, so keyboard events are given to the world

        std::string direction, action;

        if (keylistener_.IsKeyDown ('W'))
            direction += " forward";

        if (keylistener_.IsKeyDown ('A'))
            direction += " left";

        if (keylistener_.IsKeyDown ('S'))
            direction += " back";

        if (keylistener_.IsKeyDown ('D'))
            direction += " right";

        if (direction.size())
        {
            if (keylistener_.IsKeyDown ('F'))
                action = "fly";
            else
                action = "walk";
        }

        if (action.size())
            cout << "player is " << action << direction << endl;
    }
}

void ApplicationLoop::render_ ()
{
    QSize viewsize (uiview_-> viewport()-> size());
    QRect viewrect (QPoint (0, 0), viewsize);

    // compositing back buffer
    QImage buffer (viewsize, QImage::Format_ARGB32);
    QPainter painter (&buffer);
    
    // paint ui view into buffer
    uiview_-> render (&painter);

    // blit ogre view into buffer
    Ogre::Box bounds (0, 0, viewsize.width(), viewsize.height());
    Ogre::PixelBox bufbox (bounds, Ogre::PF_A8R8G8B8, (void *) buffer.bits());
    
    worldview_-> OverlayUI (bufbox);
    worldview_-> RenderOneFrame ();
}

//=============================================================================
//


#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char **argv)
{
    QtApplication qtapp (argc, argv);
    Ogre3DApplication ogreapp (qtapp.GetView());
    ApplicationLoop loop (qtapp.GetView(), ogreapp.GetView());

    return qtapp.exec();
}

#ifdef __cplusplus
}
#endif


