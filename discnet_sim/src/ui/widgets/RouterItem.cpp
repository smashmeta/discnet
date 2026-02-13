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
        static std::atomic<int> s_sequence_number = 0;
        m_internal_id = s_sequence_number.fetch_add(1, std::memory_order_relaxed);

        setPixmap(QPixmap(":/images/router.png"));
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }

    void RouterItem::add_connection(ConnectionItem* connection)
    {
        m_connections.push_back(connection);
    }

    void RouterItem::remove_connection(ConnectionItem* connection)
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

    uint32_t RouterItem::internal_id() const
    {
        return m_internal_id;
    }

    nlohmann::json RouterItem::serialize() const
    {
        nlohmann::json position = {
            { "x", scenePos().x() },
            { "y", scenePos().y() }
        };

        nlohmann::json result = {
            { "internal_id", m_internal_id },
            { "name", m_name },
            { "position", position }
        };

        return result;
    }

    RouterItem* RouterItem::deserialize(const nlohmann::json& json)
    {
        RouterItem* result = nullptr;
        auto name = json["name"].get<std::string>();
        auto x = json["position"]["x"].get<double>();
        auto y = json["position"]["y"].get<double>();
        result = new RouterItem(name);
        result->setPos(QPointF(x, y));
        
        return result;
    }
} // !namespace discnet::sim::ui