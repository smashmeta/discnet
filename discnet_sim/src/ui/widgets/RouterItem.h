/*
 *
 */

#pragma once

#include <nlohmann/json.hpp>
#include <QGraphicsPixmapItem>

namespace discnet::sim::ui
{
    class ConnectionItem;

    class RouterItem : public QGraphicsPixmapItem 
    {
    public:
        RouterItem(const std::string& name, QGraphicsItem *parent = nullptr);
    
        void add(ConnectionItem* connection);
        void remove(ConnectionItem* connection);
        std::vector<ConnectionItem*> connections();

        QPointF center() const;
        QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
        uint32_t internal_id() const;
        
        nlohmann::json serialize() const;
        static RouterItem* deserialize(const nlohmann::json& json);

    protected:
        uint32_t m_internal_id;
        std::string m_name;
        std::vector<ConnectionItem*> m_connections;
    };
} // !namespace discnet::sim::ui