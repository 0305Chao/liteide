/**************************************************************************
** This file is part of LiteIDE
**
** Copyright (c) 2011-2014 LiteIDE Team. All rights reserved.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** In addition, as a special exception,  that plugins developed for LiteIDE,
** are allowed to remain closed sourced and can be distributed under any license .
** These rights are included in the file LGPL_EXCEPTION.txt in this package.
**
**************************************************************************/
// Module: filebrowser.cpp
// Creator: visualfc <visualfc@gmail.com>

#include "filebrowser.h"
#include "createfiledialog.h"
#include "createdirdialog.h"
#include "golangdocapi/golangdocapi.h"
#include "liteenvapi/liteenvapi.h"
#include "litebuildapi/litebuildapi.h"
#include "fileutil/fileutil.h"
#include "filebrowser_global.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QTreeView>
#include <QHeaderView>
#include <QToolBar>
#include <QAction>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QLabel>
#include <QDebug>
#ifdef Q_OS_WIN
#include <windows.h>
//lite_memory_check_begin
#if defined(WIN32) && defined(_MSC_VER) &&  defined(_DEBUG)
     #define _CRTDBG_MAP_ALLOC
     #include <stdlib.h>
     #include <crtdbg.h>
     #define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
     #define new DEBUG_NEW
#endif
//lite_memory_check_end
#endif

//class QSortFileSystemProxyModel : public QSortFilterProxyModel
//{
//public:
//    QSortFileSystemProxyModel(QObject *parent) :
//        QSortFilterProxyModel(parent)
//    {
//    }
//    virtual bool lessThan( const QModelIndex & left, const QModelIndex & right ) const
//    {
//        QFileSystemModel *model = static_cast<QFileSystemModel*>(this->sourceModel());
//        QFileInfo l = model->fileInfo(left);
//        QFileInfo r = model->fileInfo(right);
//        if (l.isDir() && r.isFile()) {
//            return true;
//        } else if (l.isFile() && r.isDir()) {
//            return false;
//        }
//#ifdef Q_OS_WIN
//        if (l.filePath().length() <= 3 || r.filePath().length() <= 3) {
//            return l.filePath().at(0) < r.filePath().at(0);
//        }
//#endif
//        return (l.fileName().compare(r.fileName(),Qt::CaseInsensitive) < 0);
//    }
//};

FileBrowser::FileBrowser(LiteApi::IApplication *app, QObject *parent) :
    QObject(parent),
    m_liteApp(app)
{
    m_widget = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);

    QDir::Filters filters = QDir::AllDirs | QDir::Files | QDir::Drives
                            | QDir::Readable| QDir::Writable
                            | QDir::Executable /*| QDir::Hidden*/
                            | QDir::NoDotAndDotDot;

    bool bShowHiddenFiles = m_liteApp->settings()->value(FILEBROWSER_SHOW_HIDDEN_FILES,false).toBool();
    if (bShowHiddenFiles) {
        filters |= QDir::Hidden;
    }

#ifdef Q_OS_WIN // Symlinked directories can cause file watcher warnings on Win32.
    filters |= QDir::NoSymLinks;
#endif
    //create filter toolbar
    //m_filterToolBar = new QToolBar(m_widget);
    //m_filterToolBar->setIconSize(QSize(16,16));

    m_syncAct = new QAction(QIcon("icon:filebrowser/images/sync.png"),tr("Synchronize with editor"),this);
    m_syncAct->setCheckable(true);

    m_showHideFilesAct = new QAction(tr("Show Hidden Files"),this);
    m_showHideFilesAct->setCheckable(true);
    if (bShowHiddenFiles) {
        m_showHideFilesAct->setChecked(true);
    }
    connect(m_showHideFilesAct,SIGNAL(triggered(bool)),this,SLOT(showHideFiles(bool)));

//    m_filterCombo = new QComboBox;
//    m_filterCombo->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
//    m_filterCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
//    m_filterCombo->setEditable(true);
//    m_filterCombo->addItem("*");
//    m_filterCombo->addItem("Makefile;*.go;*.cgo;*.s;*.goc;*.y;*.e64;*.pro");
//    m_filterCombo->addItem("*.sh;Makefile;*.go;*.cgo;*.s;*.goc;*.y;*.*.c;*.cpp;*.h;*.hpp;*.e64;*.pro");

    //m_filterToolBar->addAction(m_syncAct);
    //m_filterToolBar->addSeparator();
    //m_filterToolBar->addWidget(m_filterCombo);

    //create root toolbar
    m_rootToolBar = new QToolBar(m_widget);
    m_rootToolBar->setIconSize(QSize(16,16));

    m_cdupAct = new QAction(QIcon("icon:filebrowser/images/cdup.png"),tr("Open Parent"),this);

    m_rootCombo = new QComboBox;
    m_rootCombo->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    m_rootCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    m_rootCombo->setEditable(false);

    m_rootToolBar->addAction(m_cdupAct);
    m_rootToolBar->addSeparator();
    m_rootToolBar->addWidget(m_rootCombo);

    m_fileWidget = new FileSystemWidget(false,m_liteApp);
    m_fileWidget->setHideRoot(true);
    m_fileWidget->treeView()->setRootIsDecorated(true);
    m_fileWidget->model()->setFilter(filters);

    //mainLayout->addWidget(m_filterToolBar);
    mainLayout->addWidget(m_rootToolBar);
    mainLayout->addWidget(m_fileWidget);
    m_widget->setLayout(mainLayout);

    m_setRootAct = new QAction(tr("Set As Root Folder"),this);
    m_openFolderInNewWindowAct = new QAction(tr("Open Folder in New Window"),this);
    m_addToFoldersAct = new QAction(tr("Add to Folders"),this);

    connect(m_setRootAct,SIGNAL(triggered()),this,SLOT(setFolderToRoot()));
    connect(m_cdupAct,SIGNAL(triggered()),this,SLOT(cdUp()));
    connect(m_openFolderInNewWindowAct,SIGNAL(triggered()),this,SLOT(openFolderInNewWindow()));
    connect(m_addToFoldersAct,SIGNAL(triggered()),this,SLOT(addToFolders()));

    QList<QAction*> actions;
    m_configMenu = new QMenu(tr("Config"));
    m_configMenu->setIcon(QIcon("icon:markdown/images/config.png"));
    m_configMenu->addAction(m_showHideFilesAct);
    actions << m_configMenu->menuAction() << m_syncAct;

    m_toolWindowAct = m_liteApp->toolWindowManager()->addToolWindow(Qt::LeftDockWidgetArea,m_widget,"filesystem",tr("File System"),true,actions);
    connect(m_toolWindowAct,SIGNAL(toggled(bool)),this,SLOT(visibilityChanged(bool)));
    //connect(m_filterCombo,SIGNAL(activated(QString)),this,SLOT(activatedFilter(QString)));
    connect(m_rootCombo,SIGNAL(activated(QString)),this,SLOT(activatedRoot(QString)));
    connect(m_syncAct,SIGNAL(triggered(bool)),this,SLOT(syncFileModel(bool)));
    connect(m_liteApp->editorManager(),SIGNAL(currentEditorChanged(LiteApi::IEditor*)),this,SLOT(currentEditorChanged(LiteApi::IEditor*)));
    connect(m_fileWidget,SIGNAL(aboutToShowContextMenu(QMenu*,LiteApi::FILESYSTEM_CONTEXT_FLAG,QFileInfo)),this,SLOT(aboutToShowContextMenu(QMenu*,LiteApi::FILESYSTEM_CONTEXT_FLAG,QFileInfo)));

    QString root = m_liteApp->settings()->value("FileBrowser/root","").toString();
    if (!root.isEmpty()) {
        addFolderToRoot(root);
    }
    bool b = m_liteApp->settings()->value("FileBrowser/synceditor",true).toBool();
    if (b) {
        m_syncAct->setChecked(true);
    }
}

FileBrowser::~FileBrowser()
{
    QString root = m_rootCombo->currentText();
    m_liteApp->settings()->setValue("FileBrowser/root",root);
    m_liteApp->settings()->setValue("FileBrowser/synceditor",m_syncAct->isChecked());
    delete m_configMenu;
    delete m_widget;
}

void FileBrowser::visibilityChanged(bool)
{
}

void FileBrowser::currentEditorChanged(LiteApi::IEditor *editor)
{
    if (!m_syncAct->isChecked()) {
        return;
    }
    if (!editor) {
        return;
    }
    QString fileName = editor->filePath();
    if (fileName.isEmpty()) {
        return;
    }
    QFileInfo info(fileName);

    addFolderToRoot(info.path());

    QModelIndex index = m_fileWidget->model()->findPath(fileName);
    if (index.isValid()) {
        m_fileWidget->treeView()->setCurrentIndex(index);
    }
}

void FileBrowser::syncFileModel(bool b)
{
    if (b == false) {
        return;
    } else {
        currentEditorChanged(m_liteApp->editorManager()->currentEditor());
    }
}

void FileBrowser::aboutToShowContextMenu(QMenu *menu, LiteApi::FILESYSTEM_CONTEXT_FLAG flag, const QFileInfo &/*fileInfo*/)
{
    if (flag == LiteApi::FILESYSTEM_FOLDER || flag == LiteApi::FILESYSTEM_ROOTFOLDER) {
        menu->addSeparator();
        if (flag == LiteApi::FILESYSTEM_ROOTFOLDER) {
            menu->addAction(m_cdupAct);
        } else {
            menu->addAction(m_setRootAct);
        }
        menu->addAction(m_addToFoldersAct);
        menu->addAction(m_openFolderInNewWindowAct);
    }
}

void FileBrowser::showHideFiles(bool b)
{
    if (isShowHideFiles() == b) {
        return;
    }
    QDir::Filters filters = m_fileWidget->model()->filter();
    if (b) {
        filters |= QDir::Hidden;
    } else {
        filters ^= QDir::Hidden;
    }
    m_fileWidget->model()->setFilter(filters);
    m_liteApp->settings()->setValue(FILEBROWSER_SHOW_HIDDEN_FILES,b);
}

bool FileBrowser::isShowHideFiles() const
{
    return m_fileWidget->model()->filter() & QDir::Hidden;
}

void FileBrowser::openFolderInNewWindow()
{
    QDir dir = m_fileWidget->contextDir();
    m_liteApp->fileManager()->openFolderInNewWindow(dir.path());
}

void FileBrowser::addToFolders()
{
    QDir dir = m_fileWidget->contextDir();
    m_liteApp->fileManager()->addFolderList(dir.path());
}

void FileBrowser::addFolderToRoot(const QString &path)
{
    int index = -1;
    for (int i = 0; i < m_rootCombo->count(); i++) {
        QString text = m_rootCombo->itemText(i);
        if (text == path) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        m_rootCombo->addItem(path);
        index = m_rootCombo->count()-1;
    }
    m_rootCombo->setCurrentIndex(index);
    activatedRoot(path);
}

void FileBrowser::setFolderToRoot()
{
    QDir dir = m_fileWidget->contextDir();
    addFolderToRoot(dir.path());
}

void FileBrowser::activatedRoot(QString path)
{
    m_fileWidget->setRootPath(path);
}

void FileBrowser::cdUp()
{
    QString path = m_fileWidget->rootPath();
    if (path.isEmpty()) {
        return;
    }
    QDir dir(path);
    if (!dir.path().isEmpty() && dir.cdUp()) {
        addFolderToRoot(dir.path());
    }
}
