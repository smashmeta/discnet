/*
 *
 */

#pragma once

#include <QGraphicsPixmapItem>

namespace discnet::sim::ui
{
    class RouterItem : public QGraphicsPixmapItem 
    {
    public:
        RouterItem(QGraphicsItem *parent = nullptr);
    };
} // !namespace discnet::sim::ui