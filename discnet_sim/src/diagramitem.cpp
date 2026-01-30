// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "diagramitem.h"
#include "arrow.h"

#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>

static uint16_t s_node_id = 0;
//! [0]
DiagramItem::DiagramItem(DiagramType diagramType, QMenu *contextMenu,
                         QGraphicsItem *parent)
    : QGraphicsPolygonItem(parent), myDiagramType(diagramType)
    , myContextMenu(contextMenu)
    , m_node_id(++s_node_id)
    , m_dialog(new NodeDialog(m_node_id, nullptr))
{
    QGraphicsPixmapItem *imageItem = new QGraphicsPixmapItem(this);
    QPixmap pix(":/images/pc.png");
    imageItem->setPixmap(pix);

    // Center the text within the polygon's bounding rect
    QRectF imageRect = imageItem->boundingRect();
    imageRect.moveCenter(boundingRect().center());
    imageItem->setPos(imageRect.topLeft());

    QGraphicsTextItem *textItem = new QGraphicsTextItem(QString::number(m_node_id), this);
    textItem->setTextWidth(boundingRect().width());
    
    // Optional: Customize font, color, etc.
    QFont font;
    font.setPointSize(30);
    textItem->setFont(font);
    textItem->setDefaultTextColor(Qt::black);

    // Center the text within the polygon's bounding rect
    QRectF textRect = textItem->boundingRect();
    textRect.moveCenter(boundingRect().center());
    textRect.moveRight(15);
    textRect.moveBottom(15);
    textItem->setPos(textRect.topLeft());
    
    QPainterPath path;
    switch (myDiagramType) {
        case StartEnd:
            path.moveTo(200, 50);
            path.arcTo(150, 0, 50, 50, 0, 90);
            path.arcTo(50, 0, 50, 50, 90, 90);
            path.arcTo(50, 50, 50, 50, 180, 90);
            path.arcTo(150, 50, 50, 50, 270, 90);
            path.lineTo(200, 25);
            myPolygon = path.toFillPolygon();
            break;
        case Conditional:
            myPolygon << QPointF(-100, 0) << QPointF(0, 100)
                    << QPointF(100, 0) << QPointF(0, -100)
                    << QPointF(-100, 0);
            break;
        case Step:
            myPolygon << QPointF(-100, -100) << QPointF(100, -100)
                    << QPointF(100, 100) << QPointF(-100, 100)
                    << QPointF(-100, -100);
            break;
        default:
            myPolygon << QPointF(-120, -80) << QPointF(-70, 80)
                    << QPointF(120, 80) << QPointF(70, -80)
                    << QPointF(-120, -80);
            break;
    }
    
    setPolygon(myPolygon);
    
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}
//! [0]

DiagramItem::~DiagramItem()
{
    delete m_dialog;
}

//! [1]
void DiagramItem::removeArrow(Arrow *arrow)
{
    arrows.removeAll(arrow);
}
//! [1]

//! [2]
void DiagramItem::removeArrows()
{
    // need a copy here since removeArrow() will
    // modify the arrows container
    const auto arrowsCopy = arrows;
    for (Arrow *arrow : arrowsCopy) {
        arrow->startItem()->removeArrow(arrow);
        arrow->endItem()->removeArrow(arrow);
        scene()->removeItem(arrow);
        delete arrow;
    }
}
//! [2]

//! [3]
void DiagramItem::addArrow(Arrow *arrow)
{
    arrows.append(arrow);
}
//! [3]

//! [4]
QPixmap DiagramItem::image() const
{
    QPixmap pixmap(250, 250);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(Qt::black, 8));
    painter.translate(125, 125);
    painter.drawPolyline(myPolygon);

    return pixmap;
}
//! [4]

//! [5]
void DiagramItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    scene()->clearSelection();
    setSelected(true);
    myContextMenu->popup(event->screenPos());
}
//! [5]

//! [6]
QVariant DiagramItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionChange) {
        for (Arrow *arrow : std::as_const(arrows))
            arrow->updatePosition();
    }

    return value;
}
//! [6]
