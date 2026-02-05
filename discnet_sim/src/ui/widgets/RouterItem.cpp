/*
 *
 */

#include "ui/widgets/RouterItem.h"


namespace discnet::sim::ui
{
    RouterItem::RouterItem(QGraphicsItem *parent)
        : QGraphicsPixmapItem(parent)
    {
        setPixmap(QPixmap(":/images/router.png"));
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
    }
} // !namespace discnet::sim::ui