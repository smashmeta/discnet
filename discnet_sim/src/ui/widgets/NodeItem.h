/*
 *
 */

#pragma once

#include <QGraphicsPixmapItem>

namespace discnet::sim::ui
{
    class NodeItem : public QGraphicsPixmapItem 
    {
    public:
        NodeItem(QGraphicsItem *parent = nullptr);
    };
} // !namespace discnet::sim::ui