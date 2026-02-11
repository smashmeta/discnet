/*
 *
 */

#include <QPainter>
#include "ui/widgets/RouterItem.h"
#include "ui/widgets/AdapterItem.h"
#include "ui/widgets/ConnectionItem.h"

namespace discnet::sim::ui
{
    ConnectionItem::ConnectionItem(AdapterItem* adapter, RouterItem* router, QGraphicsItem *parent)
        : QGraphicsLineItem(parent), m_adapter(adapter), m_router(router)
    {
        internal_update();
    }

    void ConnectionItem::update_adapter(AdapterItem* adapter)
    {
        m_adapter = adapter;
        internal_update();
    }
    
    void ConnectionItem::update_router(RouterItem* router)
    {
        m_router = router;
        internal_update();
    }

    AdapterItem* ConnectionItem::adapter()
    {
        return m_adapter;
    }

    RouterItem* ConnectionItem::router()
    {
        return m_router;
    }

    void ConnectionItem::internal_update()
    {
        setLine(m_adapter->center().x(), m_adapter->center().y(), m_router->center().x(), m_router->center().y());
        auto brush = QBrush(Qt::GlobalColor::black, Qt::BrushStyle::SolidPattern);
        setPen(QPen(brush, 5.0f));
    }
} // !namespace discnet::sim::ui