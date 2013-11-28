/**************************************************************************
** This file is part of LiteIDE
**
** Copyright (c) 2011-2013 LiteIDE Team. All rights reserved.
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
// Module: golangpresentedit.cpp
// Creator: visualfc <visualfc@gmail.com>

#include "golangpresentedit.h"
#include "editorutil/editorutil.h"
#include "fileutil/fileutil.h"
#include "liteenvapi/liteenvapi.h"
#include "liteeditorapi/liteeditorapi.h"
#include <QToolBar>
#include <QMenu>
#include <QAction>
#include <QTextCursor>
#include <QTextBlock>
#include <QProcess>
#include <QFileDialog>
#include <QDesktopServices>
#include <QRegExp>
#include <QDebug>

GolangPresentEdit::GolangPresentEdit(LiteApi::IApplication *app, LiteApi::IEditor *editor, QObject *parent) :
    QObject(parent), m_liteApp(app), m_htmldoc(0), m_process(0)
{
    m_editor = LiteApi::getTextEditor(editor);
    if (!m_editor) {
        return;
    }
    m_ed = LiteApi::getPlainTextEdit(editor);
    if (m_ed) {
        m_ed->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    }

    connect(m_liteApp->editorManager(),SIGNAL(editorSaved(LiteApi::IEditor*)),this,SLOT(editorSaved(LiteApi::IEditor*)));

    LiteApi::IActionContext *actionContext = m_liteApp->actionManager()->getActionContext(this,"GoSlide");

    QAction *s1 = new QAction(QIcon("icon:golangpresent/images/s1.png"),tr("Section (s1)"),this);
    actionContext->regAction(s1,"Section","Ctrl+1");

    QAction *s2 = new QAction(QIcon("icon:golangpresent/images/s2.png"),tr("Subsection (s2)"),this);
    actionContext->regAction(s2,"Subsection","Ctrl+2");

    QAction *s3 = new QAction(QIcon("icon:golangpresent/images/s3.png"),tr("Sub-subsection (s3)"),this);
    actionContext->regAction(s3,"Sub-subsection","Ctrl+3");

    QAction *bold = new QAction(QIcon("icon:golangpresent/images/bold.png"),tr("Bold"),this);
    actionContext->regAction(bold,"Bold",QKeySequence::Bold);

    QAction *italic = new QAction(QIcon("icon:golangpresent/images/italic.png"),tr("Italic"),this);
    actionContext->regAction(italic,"Italic",QKeySequence::Italic);

    QAction *code = new QAction(QIcon("icon:golangpresent/images/code.png"),tr("Inline Code"),this);
    actionContext->regAction(code,"InlineCode","Ctrl+K");

    QAction *bullets = new QAction(QIcon("icon:golangpresent/images/bullets.png"),tr("Switch Bullets"),this);
    actionContext->regAction(bullets,"Switch Bullets","Ctrl+Shift+U");

    QAction *comment = new QAction(tr("Comment/Uncomment Selection"),this);
    actionContext->regAction(comment,"Comment","Ctrl+/");

    QAction *exportHtml = new QAction(QIcon("icon:golangpresent/images/exporthtml.png"),tr("Export HTML"),this);
    actionContext->regAction(exportHtml,"Export HTML","");

    QAction *verify = new QAction(QIcon("icon:golangpresent/images/verify.png"),tr("Verify Present"),this);
    actionContext->regAction(verify,"Verify Present","");

    //QAction *exportPdf = new QAction(QIcon("icon:golangpresent/images/exportpdf.png"),tr("Export PDF"),this);
    //actionContext->regAction(exportPdf,"Export PDF","");

    connect(m_editor,SIGNAL(destroyed()),this,SLOT(deleteLater()));
    connect(s1,SIGNAL(triggered()),this,SLOT(s1()));
    connect(s2,SIGNAL(triggered()),this,SLOT(s2()));
    connect(s3,SIGNAL(triggered()),this,SLOT(s3()));
    connect(bold,SIGNAL(triggered()),this,SLOT(bold()));
    connect(italic,SIGNAL(triggered()),this,SLOT(italic()));
    connect(code,SIGNAL(triggered()),this,SLOT(code()));
    connect(bullets,SIGNAL(triggered()),this,SLOT(bullets()));
    connect(comment,SIGNAL(triggered()),this,SLOT(comment()));
    connect(exportHtml,SIGNAL(triggered()),this,SLOT(exportHtml()));
    connect(verify,SIGNAL(triggered()),this,SLOT(verify()));
    //connect(exportPdf,SIGNAL(triggered()),this,SLOT(exportPdf()));

    QToolBar *toolBar = LiteApi::findExtensionObject<QToolBar*>(editor,"LiteApi.QToolBar");
    if (toolBar) {
        toolBar->addSeparator();
        toolBar->addAction(s1);
        toolBar->addAction(s2);
        toolBar->addAction(s3);
        toolBar->addSeparator();
        toolBar->addAction(bold);
        toolBar->addAction(italic);
        toolBar->addAction(code);
        toolBar->addSeparator();
        toolBar->addAction(bullets);
        toolBar->addSeparator();
        toolBar->addAction(verify);
        toolBar->addSeparator();
        toolBar->addAction(exportHtml);
        //toolBar->addAction(exportPdf);
    }

    QMenu *menu = LiteApi::getEditMenu(editor);
    if (menu) {
        menu->addSeparator();
        menu->addAction(s1);
        menu->addAction(s2);
        menu->addAction(s3);
        menu->addSeparator();
        menu->addAction(bold);
        menu->addAction(italic);
        menu->addAction(code);
        menu->addSeparator();
        menu->addAction(bullets);
        menu->addSeparator();
        menu->addAction(comment);
        menu->addSeparator();
        menu->addAction(verify);
        menu->addSeparator();
        menu->addAction(exportHtml);
        //menu->addAction(exportPdf);
    }

    menu = LiteApi::getContextMenu(editor);
    if (menu) {
       menu->addSeparator();
       menu->addAction(s1);
       menu->addAction(s2);
       menu->addAction(s3);
       menu->addSeparator();
       menu->addAction(bold);
       menu->addAction(italic);
       menu->addAction(code);
       menu->addSeparator();
       menu->addAction(bullets);
       menu->addSeparator();
       menu->addAction(comment);
    }
}

void GolangPresentEdit::editorSaved(LiteApi::IEditor *editor)
{
    if (editor == m_editor) {
        this->verify();
    }
}

void GolangPresentEdit::s1()
{
    EditorUtil::InsertHead(m_ed,"* ");
}

void GolangPresentEdit::s2()
{
    EditorUtil::InsertHead(m_ed,"** ");
}

void GolangPresentEdit::s3()
{
    EditorUtil::InsertHead(m_ed,"*** ");
}

void GolangPresentEdit::bold()
{
    EditorUtil::MarkSelection(m_ed,"*");
}

void GolangPresentEdit::italic()
{
    EditorUtil::MarkSelection(m_ed,"_");
}

void GolangPresentEdit::code()
{
    EditorUtil::MarkSelection(m_ed,"`");
}

void GolangPresentEdit::bullets()
{
    EditorUtil::SwitchHead(m_ed,"- ",QStringList() << "- ");
}

void GolangPresentEdit::comment()
{
    EditorUtil::SwitchHead(m_ed,"# ",QStringList() << "# " << "#");
}

void GolangPresentEdit::verify()
{
    startExportHtmlDoc(EXPORT_TYPE_VERIFY);
}

void GolangPresentEdit::exportHtml()
{
    startExportHtmlDoc(EXPORT_TYPE_HTML);
}

void GolangPresentEdit::exportPdf()
{
    startExportHtmlDoc(EXPORT_TYPE_PDF);
}

void GolangPresentEdit::extOutput(const QByteArray &data, bool bError)
{
    if (!bError) {
        m_exportData.append(data);
    } else {
        QString msg = QString::fromUtf8(data);
        m_liteApp->appendLog("GolangPresent",msg,true);
        LiteApi::ILiteEditor *liteEditor = LiteApi::getLiteEditor(m_editor);
        if (liteEditor) {
            liteEditor->setNavigateHead(LiteApi::EditorNavigateError,QString::fromUtf8(data));
        }
        QRegExp re("(\\w?:?[\\w\\d_\\-\\\\/\\.]+):(\\d+):");
        re.indexIn(msg);
        if (re.captureCount() >= 2) {
            bool ok = false;
            int line = re.cap(2).toInt(&ok);
            if (ok) {
                liteEditor->gotoLine(line-1,0,true);
            }
        }
    }
}

void GolangPresentEdit::extFinish(bool error, int code, QString /*msg*/)
{
    if (!error && code == 0) {
        int exportType = m_process->userData(0).toInt();
        if (exportType == EXPORT_TYPE_VERIFY) {
            m_liteApp->appendLog("GolangPresent","verify success",false);
            LiteApi::ILiteEditor *liteEditor = LiteApi::getLiteEditor(m_editor);
            if (liteEditor) {
                liteEditor->setNavigateHead(LiteApi::EditorNavigateNormal,tr("Present verify success"));
            }
        } else if (exportType == EXPORT_TYPE_HTML) {
            static QString init = QFileInfo(m_editor->filePath()).absolutePath();
            QString outdir = QFileDialog::getExistingDirectory(m_liteApp->mainWindow(),tr("Select export html directory"),init);
            if (outdir.isEmpty()) {
                return;
            }
            init = outdir;
            QDir dir(outdir);
            QFile file(QFileInfo(dir,QFileInfo(m_editor->filePath()).fileName()+".html").filePath());
            if (!file.open(QFile::WriteOnly)) {
                return;
            }
            file.write(m_exportData);
            dir.mkdir("static");
            dir.mkdir("js");
            FileUtil::CopyDirectory(m_liteApp->resourcePath()+"/gopresent/static",dir.path()+"/static");
            FileUtil::CopyDirectory(m_liteApp->resourcePath()+"/gopresent/js",dir.path()+"/js");
            QDesktopServices::openUrl(QUrl::fromLocalFile(outdir));
        } else if (exportType == EXPORT_TYPE_PDF) {
            QString init = QFileInfo(m_editor->filePath()).absolutePath()+"/"+QFileInfo(m_editor->filePath()).completeBaseName()+".pdf";
            m_pdfFileName = QFileDialog::getSaveFileName(m_liteApp->mainWindow(),tr("Export PDF"),init,"*.pdf");
            if (m_pdfFileName.isEmpty()) {
                return;
            }
            if (!m_htmldoc) {
                m_htmldoc = m_liteApp->htmlWidgetManager()->createDocument(this);
                connect(m_htmldoc,SIGNAL(loadFinished(bool)),this,SLOT(loadHtmlFinished(bool)));
            }
            QUrl url = QUrl::fromLocalFile(m_liteApp->resourcePath()+"/gopresent/export.html");
            m_htmldoc->setHtml(QString::fromUtf8(m_exportData),url);
        }
    }
}

void GolangPresentEdit::loadHtmlFinished(bool b)
{
    if (!b) {
        m_liteApp->appendLog("GolangPresent","Failed export PDF document!");
        return;
    }
#ifndef QT_NO_PRINTER
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setCreator("LiteIDE");
        printer.setOutputFileName(m_pdfFileName);
        m_htmldoc->print(&printer);
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_pdfFileName).path()));
#endif
}

bool GolangPresentEdit::startExportHtmlDoc(EXPORT_TYPE type)
{
    m_liteApp->editorManager()->saveEditor(m_editor);
    QString cmd = FileUtil::lookupLiteBin("gopresent",m_liteApp);
    if (cmd.isEmpty()) {
        m_liteApp->appendLog("GolangPresent","Not find gopresent",true);
        return false;
    }
    QFileInfo info(m_editor->filePath());
    if (!m_process) {
        m_process = new ProcessEx(this);
        m_process->setWorkingDirectory(info.absolutePath());
        connect(m_process,SIGNAL(extOutput(QByteArray,bool)),this,SLOT(extOutput(QByteArray,bool)));
        connect(m_process,SIGNAL(extFinish(bool,int,QString)),this,SLOT(extFinish(bool,int,QString)));
    }
    if (m_process->isRunning()) {
        m_process->waitForFinished(3000);
        if (m_process->isRunning()) {
            return false;
        }
    }
    m_exportData.clear();
    m_process->setUserData(0,type);
    if (type == EXPORT_TYPE_VERIFY) {
        m_process->startEx(cmd,"-v -i "+info.fileName().toUtf8());
    } else {
        m_process->startEx(cmd,"-stdout -i "+info.fileName().toUtf8());
    }
    return true;
}
