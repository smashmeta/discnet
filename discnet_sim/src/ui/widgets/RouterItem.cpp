/*
 *
 */

#include "ui/widgets/ConnectionItem.h"
#include "ui/widgets/RouterItem.h"
#include <QFont>

namespace discnet::sim::ui
{
    RouterItem::RouterItem(const std::string& name, QGraphicsItem *parent)
        : QGraphicsPixmapItem(parent), m_name(name)
    {
        setPixmap(QPixmap(":/images/router.png"));
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }

    void RouterItem::add(ConnectionItem* connection)
    {
        m_connections.push_back(connection);
    }

    void RouterItem::remove(ConnectionItem* connection)
    {
        auto existing = std::find(m_connections.begin(), m_connections.end(), connection);
        if (existing != m_connections.end())
        {
            m_connections.erase(existing);
        }
    }

    std::vector<ConnectionItem*> RouterItem::connections()
    {
        return m_connections;
    }

    QPointF RouterItem::center() const
    {
        return scenePos() + boundingRect().center();
    }

    QVariant RouterItem::itemChange(GraphicsItemChange change, const QVariant &value) 
    {
        if (change == ItemPositionHasChanged && scene()) 
        {
            for (ConnectionItem* connection : m_connections) 
            {
                connection->update_router(this);
            }
        }

        return QGraphicsItem::itemChange(change, value);
    }
} // !namespace discnet::sim::ui