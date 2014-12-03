#include "sidewindowstyle.h"
#include "tooldockwidget.h"
#include "rotationtoolbutton.h"

#include <QStatusBar>

OutputActionBar::OutputActionBar(QSize iconSize, QMainWindow *window, Qt::DockWidgetArea _area)
    : QObject(window), area(_area), bHideToolBar(false)
{
    toolBar = new QToolBar;
    toolBar->hide();
    toolBar->setObjectName(QString("side_tool_%1").arg(area));
    toolBar->setMovable(false);

    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    spacerAct = toolBar->addWidget(spacer);
    toolBar->addSeparator();

    dock = new ToolDockWidget(iconSize, window);
    dock->setObjectName(QString("side_dock_%1").arg(area));
    dock->setWindowTitle(QString("side_dock_%1").arg(area));
    dock->setFeatures(QDockWidget::DockWidgetClosable);
    dock->hide();
    dock->createMenu(area,false);

    window->addDockWidget(area,dock);

    connect(dock,SIGNAL(visibilityChanged(bool)),this,SLOT(dockVisible(bool)));
    connect(dock,SIGNAL(moveActionTo(Qt::DockWidgetArea,QAction*,bool)),this,SIGNAL(moveActionTo(Qt::DockWidgetArea,QAction*,bool)));
}

OutputActionBar::~OutputActionBar()
{
    qDeleteAll(m_actionStateMap);
}

ToolDockWidget *OutputActionBar::dockWidget() const
{
    return dock;
}

void OutputActionBar::addAction(QAction *action, QWidget *widget, const QString &id, const QString &title, QList<QAction *> widgetActions)
{
    RotationToolButton *btn = new RotationToolButton;
    btn->setDefaultAction(action);
    if (area == Qt::LeftDockWidgetArea) {
        btn->setRotation(RotationToolButton::CounterClockwise);
    } else if (area == Qt::RightDockWidgetArea) {
        btn->setRotation(RotationToolButton::Clockwise);
    }
    SideActionState *state = new SideActionState;
    state->toolBtn = btn;
    state->widget = widget;
    state->id = id;
    state->title = title;
    state->widgetActions = widgetActions;
    m_actionStateMap.insert(action,state);
    dock->addAction(action,title);
    toolBar->insertWidget(spacerAct,btn);
    if (toolBar->isHidden() && !bHideToolBar) {
        toolBar->show();
    }
    connect(action,SIGNAL(toggled(bool)),this,SLOT(toggledAction(bool)));
}

void OutputActionBar::removeAction(QAction *action)
{
    SideActionState *state = m_actionStateMap.value(action);
    if (state) {
        delete state->toolBtn;
    }
    m_actionStateMap.remove(action);
    delete state;
    dock->removeAction(action);
    if (dock->actions().isEmpty()) {
        toolBar->hide();
    }
}

void OutputActionBar::setHideToolBar(bool b)
{
    bHideToolBar = b;
    if (bHideToolBar) {
        toolBar->hide();
    } else {
        if (!dock->actions().isEmpty()){
            toolBar->show();
        }
    }
}

QAction *OutputActionBar::findToolAction(QWidget *widget)
{
    QMapIterator<QAction*,SideActionState*> i(m_actionStateMap);
    while (i.hasNext()) {
        i.next();
        if (i.value()->widget == widget) {
            return i.key();
        }
    }
    return 0;
}

void OutputActionBar::dockVisible(bool b)
{
    QAction *action = dock->checkedAction();
    if (action) {
        action->setChecked(dock->isVisible());
    } else if (b && !dock->actions().isEmpty()) {
        dock->actions().first()->setChecked(true);
    }
}

void OutputActionBar::toggledAction(bool b)
{
    QAction *action = (QAction*)sender();
    SideActionState *state = m_actionStateMap.value(action);
    if (!state) {
        return;
    }
    if (action->isChecked()) {
        if (dock->isHidden()) {
            dock->show();
        }
        dock->setWidget(state->widget);
        dock->setWidgetActions(state->widgetActions);
        dock->setWindowTitle(state->title);
    } else {
        if (!dock->checkedAction()) {
            dock->hide();
        }
    }
}

SideWindowStyle::SideWindowStyle(QSize iconSize, QMainWindow *window, QObject *parent)
    : IWindowStyle(parent),m_mainWindow(window)
{
    m_sideBar = new OutputActionBar(iconSize,window,Qt::LeftDockWidgetArea);
    m_outputBar = new OutputActionBar(iconSize,window,Qt::BottomDockWidgetArea);

    m_mainWindow->addToolBar(Qt::LeftToolBarArea,m_sideBar->toolBar);
    m_mainWindow->addDockWidget(Qt::LeftDockWidgetArea,m_sideBar->dock);

    m_mainWindow->setCorner(Qt::BottomLeftCorner,Qt::LeftDockWidgetArea);

    m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea,m_outputBar->dock);

    m_mainWindow->setDockNestingEnabled(true);
    m_mainWindow->setDockOptions(QMainWindow::AllowNestedDocks);

    m_statusBar = new QStatusBar;

    m_hideSideAct = new QAction(tr("Hide Sidebars"),this);
    m_hideSideAct->setIcon(QIcon("icon:images/hidesidebar.png"));
    m_hideSideAct->setCheckable(true);

    QToolButton *btn = new QToolButton;
    btn->setDefaultAction(m_hideSideAct);
    btn->setStyleSheet("QToolButton {border:0}"
                       "QToolButton:checked {background : qlineargradient(spread:pad, x1:0, y1:1, x2:1, y2:0, stop:0 rgba(55, 57, 59, 255), stop:1 rgba(255, 255, 255, 255));}");
    m_statusBar->addWidget(btn);

    m_statusBar->setContentsMargins(0,0,0,0);

    m_statusBar->addWidget(m_outputBar->toolBar,1);

    m_mainWindow->setStatusBar(m_statusBar);

}

SideWindowStyle::~SideWindowStyle()
{

}

void SideWindowStyle::hideAllToolWindows()
{

}

void SideWindowStyle::showOrHideToolWindow()
{

}

void SideWindowStyle::hideToolWindow(Qt::DockWidgetArea area)
{

}

bool SideWindowStyle::restoreState(const QByteArray &state, int version)
{
    return true;
}

bool SideWindowStyle::loadInitToolState(const QByteArray &state, int version)
{
    return true;
}

QByteArray SideWindowStyle::saveToolState(int version) const
{
    return QByteArray();
}

void SideWindowStyle::moveToolWindow(Qt::DockWidgetArea area, QAction *action, bool split)
{

}

QAction *SideWindowStyle::findToolWindow(QWidget *widget)
{
    QAction *act = m_sideBar->findToolAction(widget);
    if (act) {
        return act;
    }
    return m_outputBar->findToolAction(widget);
}

void SideWindowStyle::removeToolWindow(QAction *action)
{

}

QAction *SideWindowStyle::addToolWindow(LiteApi::IApplication *app, Qt::DockWidgetArea area, QWidget *widget, const QString &id, const QString &title, bool split, QList<QAction *> widgetActions)
{
    QAction *action = new QAction(this);
    action->setText(title);
    action->setCheckable(true);
    action->setObjectName(id);

    if (area == Qt::TopDockWidgetArea || area == Qt::BottomDockWidgetArea) {
        m_outputBar->addAction(action,widget,id,title,widgetActions);
    } else {
        m_sideBar->addAction(action,widget,id,title,widgetActions);
    }
    return action;
}
