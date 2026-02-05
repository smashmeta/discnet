/*
 *
 */

#include "ui/widgets/NodeItem.h"


namespace discnet::sim::ui
{
    NodeItem::NodeItem(QGraphicsItem *parent)
        : QGraphicsPixmapItem(parent)
    {
        setPixmap(QPixmap(":/images/node.png"));
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
    }
} // !namespace discnet::sim::ui