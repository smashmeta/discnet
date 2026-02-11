/*
 *
 */

#pragma once

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
        
    protected:
        std::string m_name;
        std::vector<ConnectionItem*> m_connections;
    };
} // !namespace discnet::sim::ui