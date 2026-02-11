/*
 *
 */

#include "ui/widgets/ConnectionItem.h"
#include "ui/widgets/AdapterItem.h"


namespace discnet::sim::ui
{
    AdapterItem::AdapterItem(const adapter_t& adapter, QGraphicsItem* parent)
        : QGraphicsPixmapItem(QPixmap(":/images/adapter.png"), parent), m_adapter(adapter)
    {
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }

    AdapterItem::~AdapterItem()
    {
        // nothing for now
    }

    void AdapterItem::add(ConnectionItem* connection)
    {
        m_connections.push_back(connection);
    }

    QPointF AdapterItem::center() const
    {
        return scenePos() + boundingRect().center();
    }

    QVariant AdapterItem::itemChange(GraphicsItemChange change, const QVariant &value) 
    {
        if (change == ItemPositionHasChanged && scene()) 
        {
            for (ConnectionItem* connection : m_connections) 
            {
                connection->update_adapter(this);
            }
        }

        return QGraphicsItem::itemChange(change, value);
    }
} // !namespace discnet::sim::ui