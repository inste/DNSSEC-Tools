/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "graphwidget.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QString fileName;
    if (app.arguments().count() > 1)
        fileName = app.arguments().last();
    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

    QWidget *mainWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout();
    QHBoxLayout *hbox = new QHBoxLayout();
    QHBoxLayout *infoBox = new QHBoxLayout();
    layout->addLayout(infoBox);

    QLineEdit *editBox = new QLineEdit();
    GraphWidget *graphWidget = new GraphWidget(0, editBox, fileName, infoBox);
    layout->addWidget(graphWidget);
    layout->addLayout(hbox);

    hbox->addWidget(editBox);

    QMainWindow mainWindow;
    mainWidget->setLayout(layout);
    mainWindow.setCentralWidget(mainWidget);
    mainWindow.setWindowIcon(QIcon(":/icons/dnssec-nodes-64x64.png"));

    QPushButton *button;
    hbox->addWidget(button = new QPushButton("+"));
    button->connect(button, SIGNAL(clicked()), graphWidget, SLOT(zoomIn()));

    hbox->addWidget(button = new QPushButton("-"));
    button->connect(button, SIGNAL(clicked()), graphWidget, SLOT(zoomOut()));

    QMenuBar *menubar = mainWindow.menuBar();

    QMenu *menu = menubar->addMenu("&File");

    menu->addSeparator();

    QAction *action = menu->addAction("&Clear Nodes");
    action->connect(action, SIGNAL(triggered()), graphWidget->nodeList(), SLOT(clear()));

    menu->addSeparator();

    action = menu->addAction("&Open and Watch A Log File");
    action->connect(action, SIGNAL(triggered()), graphWidget, SLOT(openLogFile()));

    action = menu->addAction("&ReRead All Log Files");
    action->connect(action, SIGNAL(triggered()), graphWidget->logWatcher(), SLOT(reReadLogFile()));

    menu->addSeparator();

    action = menu->addAction("&Quit");
    action->connect(action, SIGNAL(triggered()), &mainWindow, SLOT(close()));

    menu = menubar->addMenu("&Options");

    action = menu->addAction("Lock Nodes");
    action->setCheckable(true);
    action->connect(action, SIGNAL(triggered(bool)), graphWidget, SLOT(setLockedNodes(bool)));

    action = menu->addAction("Show NSEC3 Records");
    action->setCheckable(true);
    action->connect(action, SIGNAL(triggered(bool)), graphWidget, SLOT(setShowNSEC3Records(bool)));

    QMenu *layoutMenu = menu->addMenu("Layout");
    action = layoutMenu->addAction("tree");
    action->connect(action, SIGNAL(triggered()), graphWidget, SLOT(switchToTree()));
    action = layoutMenu->addAction("circle");
    action->connect(action, SIGNAL(triggered()), graphWidget, SLOT(switchToCircles()));

#if defined(Q_OS_SYMBIAN) || defined(Q_WS_MAEMO_5)
    mainWindow.menuBar()->addAction("Shuffle", graphWidget, SLOT(shuffle()));
    mainWindow.menuBar()->addAction("Zoom In", graphWidget, SLOT(zoomIn()));
    mainWindow.menuBar()->addAction("Zoom Out", graphWidget, SLOT(zoomOut()));
    mainWindow.showMaximized();
#else
    mainWindow.resize(1024,800);
    mainWindow.show();
#endif
    return app.exec();
}

