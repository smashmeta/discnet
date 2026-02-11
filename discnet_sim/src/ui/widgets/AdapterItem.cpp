/*
 *
 */

#include "ui/widgets/ConnectionItem.h"
#include "ui/widgets/AdapterItem.h"


namespace discnet::sim::ui
{
    AdapterItem::AdapterItem(const adapter_t& adapter, QGraphicsItem* parent)
        : QGraphicsPixmapItem(QPixmap(":/images/adapter.png"), parent), m_adapter(adapter), m_connection(nullptr)
    {
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }

    AdapterItem::~AdapterItem()
    {
        // nothing for now
    }

    ConnectionItem* AdapterItem::connection()
    {
        return m_connection;
    }

    void AdapterItem::set_connection(ConnectionItem* connection)
    {
        m_connection = connection;
    }

    QPointF AdapterItem::center() const
    {
        return scenePos() + boundingRect().center();
    }

    QVariant AdapterItem::itemChange(GraphicsItemChange change, const QVariant &value) 
    {
        if (change == ItemPositionHasChanged && scene()) 
        {
            if (m_connection) 
            {
                m_connection->update_adapter(this);
            }
        }

        return QGraphicsItem::itemChange(change, value);
    }
} // !namespace discnet::sim::ui