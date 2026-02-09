/*
 *
 */

#include "ui/widgets/AdapterItem.h"


namespace discnet::sim::ui
{
    AdapterItem::AdapterItem(const adapter_t& adapter, QGraphicsItem* parent)
        : QGraphicsPixmapItem(QPixmap(":/images/adapter.png"), parent), m_adapter(adapter)
    {
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
    }

    AdapterItem::~AdapterItem()
    {
        // nothing for now
    }
} // !namespace discnet::sim::ui