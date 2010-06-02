
#include <iostream>
#include <QtGui>

#ifdef Q_WS_X11
#include <QX11Info>
#include <X11/Xlib.h>

// stray defines from X.h
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#endif

using namespace std;

struct KeyInfo;
struct MouseInfo;
struct DragInfo;

struct MouseInfo
{
    MouseInfo () 
        : buttons (0), 
        wheel_delta (0), wheel_orientation (0), 
        global_x (0), global_y (0), x (0), y (0) {}

    MouseInfo (const MouseInfo &rhs) 
        : buttons (rhs.buttons), 
        wheel_delta (rhs.wheel_delta), wheel_orientation (rhs.wheel_orientation),
        global_x (rhs.global_x), global_y (rhs.global_y), x (rhs.x), y (rhs.y) {}

    MouseInfo (const QMouseEvent *e) 
        : buttons (e->buttons()), 
        wheel_delta (0), wheel_orientation (0),
        global_x (e->globalX()), global_y (e->globalY()), x (e->x()), y (e->y()) {}

    MouseInfo (const QWheelEvent *e) 
        : buttons (e->buttons()), 
        wheel_delta (e->delta()), wheel_orientation (e->orientation()),
        global_x (e->globalX()), global_y (e->globalY()), x (e->x()), y (e->y()) {}

    MouseInfo &operator= (const MouseInfo &rhs) 
    {
        buttons = rhs.buttons;
        wheel_delta = rhs.wheel_delta; wheel_orientation = rhs.wheel_orientation;
        global_x = rhs.global_x; global_y = rhs.global_y; x = rhs.x; y = rhs.y;
    }

    MouseInfo &operator= (const QMouseEvent *e)
    {
        buttons = e-> buttons();
        global_x = e-> globalX(); global_y = e-> globalY(); x = e-> x(); y = e-> y();
    }

    MouseInfo &operator= (const QWheelEvent *e)
    {
        buttons = e-> buttons();
        wheel_delta = e-> delta(); wheel_orientation = e-> orientation();
        global_x = e-> globalX(); global_y = e-> globalY(); x = e-> x(); y = e-> y();
    }

    int buttons;
    int wheel_delta;
    int wheel_orientation;
    int global_x;
    int global_y;
    int x;
    int y;
};

typedef std::vector <MouseInfo> DragList;

struct GestureInfo
{
    DragList drag;
};

struct KeyInfo
{
    KeyInfo () : key (0), modifiers (0), pressed (false) {}
    KeyInfo (int k, unsigned int m) : key (k), modifiers (m), pressed (false) {}
    KeyInfo (const KeyInfo &c) : key (c.key), modifiers (c.modifiers), pressed (c.pressed) {}
    KeyInfo (const QKeyEvent *e) : key (e-> key()), modifiers (e-> modifiers()), pressed (false) {}

    bool operator== (const KeyInfo &rhs) const { return (key == rhs.key); } 
    bool operator== (int k) const { return (key == k); }

    char ToAscii ()
    {
        if ((key > 0x1f) && (key < 0x7f))
        { // ASCII
            char c (key);
            if ((c > 0x40) && (c < 0x5b)) // upper
                if (!(modifiers & 0x02000000))
                    c |= 0x20; // to lower
            return c;
        }
        else return 0;
    }

    int key; // Qt::Key codes
    unsigned int modifiers; // modifiers

    bool pressed;
};

// maps Qt::Key codes to info structures
typedef std::map <int, KeyInfo *> KeyInfoMap;

// maps Qt::Key codes to Naali events
typedef std::map <int, int> KeyEventMap;
        

struct State : public QState
{
    State (QString name, QState *parent = 0) 
        : QState (parent) { setObjectName (name); }

    State (QString name, QState::ChildMode mode, QState *parent = 0) 
        : QState (mode, parent) { setObjectName (name); }

    virtual void onEntry (QEvent *e)
    {
        cout << "State::onEntry: " << qPrintable (objectName()) << endl;
        QState::onEntry (e);
    }

    virtual void onExit (QEvent *e)
    {
        cout << "State::onExit: " << qPrintable (objectName()) << endl;
        QState::onExit (e);
    }
};

struct FinalState : public QFinalState
{
    FinalState (QString name, QState *p = 0) 
        : QFinalState (p) { setObjectName (name); }
 
    virtual void onEntry (QEvent *e)
    {
        cout << "FinalState::onEntry: " << qPrintable (objectName()) << endl;
        QFinalState::onEntry (e);
    }

    virtual void onExit (QEvent *e)
    {
        cout << "FinalState::onExit: " << qPrintable (objectName()) << endl;
        QFinalState::onExit (e);
    }
};

template <int EventType>
class EventTransition : public QAbstractTransition
{
    public: 
        EventTransition (QState *state = 0) 
            : QAbstractTransition (state) {}

    protected:
        virtual bool eventTest (QEvent *event) { return (event-> type() == EventType); }
        virtual void onTransition (QEvent *event) {}
};


struct ActiveInputState : public State
{
    ActiveInputState (QString name, QGraphicsView *v, QState::ChildMode m, QState *p = 0) 
        : State (name, m, p), view (v)
    { 
#ifdef Q_WS_X11
        XGetKeyboardControl (view-> x11Info().display(), &x11_key_state);
#endif
    }
 
    void onEntry (QEvent *e)
    {
#ifdef Q_WS_X11
        if (x11_key_state.global_auto_repeat == AutoRepeatModeOn)
            XAutoRepeatOff (view-> x11Info().display());
#endif
        State::onEntry (e);
    }

    void onExit (QEvent *e)
    {
#ifdef Q_WS_X11
        //if (x11_key_state.global_auto_repeat == AutoRepeatModeOn)
            XAutoRepeatOn (view-> x11Info().display());
#endif
        State::onExit (e);
    }

    QGraphicsView   *view;
#ifdef Q_WS_X11
    XKeyboardState  x11_key_state;
#endif
};


struct ActiveKeyState : public State
{
    ActiveKeyState (QString name, KeyInfoMap &i, KeyEventMap &b, QState *p = 0)
        : State (name, p), keyinfo (i), bindings (b) {}

    void onEntry (QEvent *event)
    {
        QKeyEvent *e = static_cast <QKeyEvent *> (event);
        KeyInfo *info = get_key_info (e-> key());
        info-> pressed = true;

        State::onEntry (e);
    }

    void onExit (QEvent *event)
    {
        QKeyEvent *e = static_cast <QKeyEvent *> (event);
        KeyInfo *info = get_key_info (e-> key());
        info-> pressed = false;
        
        QState::onExit (e);
    }

    KeyInfo *get_key_info (int code, int modifiers = 0)
    {
        KeyInfo *info;
        KeyInfoMap::iterator i = keyinfo.find (code);

        if (i == keyinfo.end()) 
        {
            info = new KeyInfo (code, modifiers);
            keyinfo.insert (std::make_pair (code, info));
        }
        else
            info = i-> second;

        return info;
    }

    KeyInfoMap  &keyinfo;
    KeyEventMap &bindings;
};


struct ActiveButtonState : public State
{
    ActiveButtonState (QString name, MouseInfo &m, QState *p = 0)
        : State (name, p), mouse (m) {}

    void onEntry (QEvent *event)
    {
        QMouseEvent *e = static_cast <QMouseEvent *> (event);
        mouse = e;

        State::onEntry (e);
    }

    void onExit (QEvent *event)
    {
        QMouseEvent *e = static_cast <QMouseEvent *> (event);
        mouse = e;

        QState::onExit (e);
    }
        
    MouseInfo   &mouse;
};


struct ActiveGestureState : public State
{
    ActiveGestureState (QString name, GestureInfo &g, QState *p = 0)
        : State (name, p), gesture (g) {}

    void onEntry (QEvent *event)
    {
        QMouseEvent *e = static_cast <QMouseEvent *> (event);

        MouseInfo info (e);
        gesture.drag.push_back (info);

        State::onEntry (e);
    }

    void onExit (QEvent *event)
    {
        QMouseEvent *e = static_cast <QMouseEvent *> (event);

        MouseInfo info (e);
        gesture.drag.push_back (info);

        QState::onExit (e);
    }
        
    GestureInfo   &gesture;
};


class MouseWheelTransition : public EventTransition <QEvent::Wheel>
{
    public:
        MouseWheelTransition (MouseInfo &m, QState *p = 0) 
            : EventTransition <QEvent::Wheel> (p), mouse (m) {}

    protected:
        void onTransition (QEvent *event)
        {
            QWheelEvent *e = static_cast <QWheelEvent *> (event);
            mouse = e;
        }

    private:
        MouseInfo   &mouse;
};


class WorldInputLogic : public QStateMachine
{
    public:
        WorldInputLogic (QGraphicsView *view)
            : view_ (view), has_focus_ (false)
        {
            init_statemachine_();

            view-> installEventFilter (this);
            view-> viewport()-> installEventFilter (this);

            startTimer (100);
        }

    protected:
        bool eventFilter (QObject *obj, QEvent *event)
        {
            switch (event-> type())
            {
                // route select Qt events to the state machine
                case QEvent::Close:
                case QEvent::MouseMove:
                case QEvent::MouseButtonPress:
                case QEvent::MouseButtonRelease:
                case QEvent::Wheel:
                case QEvent::KeyPress:
                case QEvent::KeyRelease:
                    postEvent (clone_event_ (event));
                    break;
            }

            return QObject::eventFilter (obj, event);
        }

        void timerEvent (QTimerEvent *e)
        {
            if (view_-> scene()-> focusItem())
            {
                if (has_focus_)
                {
                    postEvent (new QFocusEvent (QEvent::FocusOut));
                    has_focus_ = false;
                }
            }
            else 
            {
                if (!has_focus_)
                {
                    postEvent (new QFocusEvent (QEvent::FocusIn));
                    has_focus_ = true;
                }
            }
        }

    private:
        void init_statemachine_ ()
        {
            State *active = new State ("active");
            FinalState *exit = new FinalState ("exit"); 

            State *unfocused = new State ("unfocused", active);

            ActiveInputState *focused = new ActiveInputState ("focused", view_, QState::ParallelStates, active);

            State *mouse = new State ("mouse", focused);
            State *keyboard = new State ("keyboard", focused);

            ActiveKeyState *key_active = new ActiveKeyState ("key active", key_state_, key_bindings_, keyboard);
            State *key_waiting = new State ("key waiting", keyboard);
            keyboard-> setInitialState (key_waiting);

            ActiveButtonState *button_active = new ActiveButtonState ("button active", mouse_state_, mouse);
            State *mouse_waiting = new State ("mouse waiting", mouse);
            mouse-> setInitialState (mouse_waiting);

            State *gesture_active = new State ("gesture active", button_active);
            State *gesture_waiting = new State ("gesture waiting", button_active);
            button_active-> setInitialState (gesture_waiting);

            addState (active);
            addState (exit);
            
            setInitialState (active);
            active-> setInitialState (unfocused);
            
            (new EventTransition <QEvent::Close> (active))-> setTargetState (exit);
            (new EventTransition <QEvent::FocusIn> (unfocused))-> setTargetState (focused);
            (new EventTransition <QEvent::FocusOut> (focused))-> setTargetState (unfocused);
            
            (new EventTransition <QEvent::KeyPress> (key_waiting))-> setTargetState (key_active);
            (new EventTransition <QEvent::KeyRelease> (key_active))-> setTargetState (key_waiting);

            (new EventTransition <QEvent::MouseButtonPress> (mouse_waiting))-> setTargetState (button_active);
            (new EventTransition <QEvent::MouseMove> (gesture_waiting))-> setTargetState (gesture_active);

            (new EventTransition <QEvent::MouseButtonRelease> (button_active))-> setTargetState (mouse_waiting);
            (new EventTransition <QEvent::MouseButtonRelease> (gesture_active))-> setTargetState (mouse_waiting);

            (new MouseWheelTransition (mouse_state_, mouse_waiting));
        }

        QEvent *clone_event_ (QEvent *event)
        {
            // state machine will take ownership of cloned events
            switch (event-> type())
            {
                case QEvent::MouseMove:
                case QEvent::MouseButtonPress:
                case QEvent::MouseButtonRelease:
                    return new QMouseEvent (*static_cast <QMouseEvent *> (event));

                case QEvent::Wheel:
                    return new QWheelEvent (*static_cast <QWheelEvent *> (event));

                case QEvent::KeyPress:
                case QEvent::KeyRelease:
                    return new QKeyEvent (*static_cast <QKeyEvent *> (event));

                case QEvent::Close:
                    return new QCloseEvent (*static_cast <QCloseEvent *> (event));
            }
        }

    private:
        QGraphicsView   *view_;
        bool            has_focus_;

        KeyEventMap     key_bindings_;
        KeyInfoMap      key_state_;
        MouseInfo       mouse_state_;
        GestureInfo     gesture_state_;
};

//=============================================================================
//
int main (int argc, char **argv)
{
    QApplication app (argc, argv);

    QGraphicsView *view = new QGraphicsView ();
    QGraphicsScene *scene = new QGraphicsScene ();
    //view-> installEventFilter (new Listener);

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

    view-> setScene (scene);
    view-> setViewportUpdateMode (QGraphicsView::FullViewportUpdate);

    item1-> setWindowFlags (Qt::Dialog);
    item1-> setFlag (QGraphicsItem::ItemIsMovable);
    item1-> setPos (10, 50);

    item2-> setWindowFlags (Qt::Dialog);
    item2-> setFlag (QGraphicsItem::ItemIsMovable);
    item2-> setPos (10, 100);

    view-> show();

    WorldInputLogic machine (view);
    machine.start();

    return app.exec();
}
