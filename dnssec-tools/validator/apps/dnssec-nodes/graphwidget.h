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

#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QtGui/QGraphicsView>
#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtCore/QString>
#include <QtCore/QRegExp>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QTextStream>
#include <QtCore/QFile>

class Node;
class Edge;

//! [0]
class GraphWidget : public QGraphicsView
{
    Q_OBJECT

public:
    GraphWidget(QWidget *parent = 0, QLineEdit *editor = 0, const QString &fileName = "", QHBoxLayout *infoBox = 0);

    enum LayoutType { springyLayout, treeLayout, circleLayout };

    void itemMoved();

    void createStartingNode();
    Node *addNode(const QString &nodeName, const QString &parentName = "", int depth=0);

    void busy();
    void unbusy();
    void doActualLookup(const QString &lookupString);

    Node * node(const QString &nodeName);
    void addNodes(const QString &nodeName);

    void parseLogMessage(QString logMessage);
    void parseLogFile(const QString &file = "");

    qreal nodeScale() { return m_nodeScale; }
    bool isLocked() { return m_lockNodes; }

    int layoutTreeNode(Node *node, int minX, int minY);
    void layoutCircleNode(Node *node, qreal startX, qreal startY, qreal startingDegrees, qreal maxDegrees);

    LayoutType layoutType() { return m_layoutType; }
    void setLayoutType(LayoutType layoutType);

    void setInfo(const QString &text);
    void setInfo(Node *node);

public slots:
    void shuffle();
    void zoomIn();
    void zoomOut();

    void reLayout();
    void layoutInTree();
    void layoutInCircles();
    void switchToTree();
    void switchToCircles();

    void addRootNode(QString newNode);
    void doLookupFromLineEdit();
    void doLookup(QString lookupString);
    void scaleWindow();
    void resizeEvent(QResizeEvent *event);
    void openLogFile();
    void reReadLogFile();

    void parseTillEnd();

    void toggleLockedNodes();
    void setLockedNodes(bool newVal);
    void setShowNSEC3Records(bool newVal);

    void clear();

protected:
    void keyPressEvent(QKeyEvent *event);
    void timerEvent(QTimerEvent *event);
    void wheelEvent(QWheelEvent *event);
    void drawBackground(QPainter *painter, const QRectF &rect);

    void scaleView(qreal scaleFactor);

private:
    int timerId;
    Node *centerNode;
    QMap<QString, Node *> m_nodes;
    QMap<QPair<QString, QString>, Edge *> m_edges;

    QGraphicsScene *myScene;
    QLineEdit   *m_editor;
    QString      m_libValDebugLog;
    qreal        m_nodeScale;
    bool         m_localScale;
    bool         m_lockNodes;
    bool         m_shownsec3;
    QFile       *m_logFile;
    QTextStream *m_logStream;
    QTimer      *m_timer;
    LayoutType   m_layoutType;
    int          m_childSize;

    QRegExp    m_validatedRegexp;
    QRegExp    m_lookingUpRegexp;
    QRegExp    m_bogusRegexp;
    QRegExp    m_trustedRegexp;
    QRegExp    m_pinsecureRegexp;

    QHBoxLayout *m_infoBox;
    QLabel      *m_infoLabel;
};
//! [0]

#endif