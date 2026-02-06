/*
 *
 */

#include "ui/widgets/NodeItem.h"


namespace discnet::sim::ui
{
    NodeItem::NodeItem(const uint16_t nodeId, QGraphicsItem *parent)
        : QGraphicsPixmapItem(parent)
    {
        setPixmap(QPixmap(":/images/node.png"));
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        m_dialog = new NodeDialog(nodeId);
    }

    NodeItem::~NodeItem()
    {
        delete m_dialog;
    }
} // !namespace discnet::sim::ui