// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIAGRAMITEM_H
#define DIAGRAMITEM_H

#include <QGraphicsPixmapItem>
#include <QTextBrowser>
#include <QList>
#include "nodedialog.h"

QT_BEGIN_NAMESPACE
class QPixmap;
class QGraphicsSceneContextMenuEvent;
class QMenu;
class QPolygonF;
QT_END_NAMESPACE

class Arrow;

//! [0]
class DiagramItem : public QGraphicsPolygonItem
{
public:
    enum { Type = UserType + 15 };
    enum DiagramType { Step, Conditional, StartEnd, Io };

    DiagramItem(DiagramType diagramType, QMenu *contextMenu, QGraphicsItem *parent = nullptr);
    ~DiagramItem();

    void removeArrow(Arrow *arrow);
    void removeArrows();
    DiagramType diagramType() const { return myDiagramType; }
    QPolygonF polygon() const { return myPolygon; }
    void addArrow(Arrow *arrow);
    QPixmap image() const;
    int type() const override { return Type; }

    uint16_t node_id() const { return m_node_id; }
    QTextEdit* log_handle() const { return m_dialog->log(); }
    void show_properties() const { m_dialog->show(); }
protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    DiagramType myDiagramType;
    QPolygonF myPolygon;
    QMenu *myContextMenu;
    QList<Arrow *> arrows;
    NodeDialog* m_dialog;
    uint16_t m_node_id;
};
//! [0]

#endif // DIAGRAMITEM_H