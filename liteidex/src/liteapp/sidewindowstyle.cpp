#include "sidewindowstyle.h"

SideWindowStyle::SideWindowStyle(QSize iconSize, QMainWindow *window, QObject *parent)
    : IWindowStyle(parent),m_mainWindow(window)
{

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

QAction *SideWindowStyle::findToolWindow(QWidget *wiget)
{
    return 0;
}

void SideWindowStyle::removeToolWindow(QAction *action)
{

}

QAction *SideWindowStyle::addToolWindow(LiteApi::IApplication *app, Qt::DockWidgetArea area, QWidget *widget, const QString &id, const QString &title, bool split, QList<QAction *> widgetActions)
{
    return 0;
}
