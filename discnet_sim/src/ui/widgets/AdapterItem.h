/*
 *
 */

#pragma once

#include <nlohmann/json.hpp>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPixmapItem>
#include <discnet/adapter.hpp>


namespace discnet::sim::ui
{
    class NodeItem;
    class ConnectionItem;

    class AdapterItem : public QGraphicsPixmapItem
    {
    public:
        AdapterItem(const adapter_t& adapter, NodeItem* node, QGraphicsItem *parent = nullptr);
        ~AdapterItem();

        NodeItem* node();
        void set_connection(ConnectionItem* connection);
        ConnectionItem* connection();
        QPointF center() const;
        adapter_t adapter() const;

        nlohmann::json serialize() const;
        static AdapterItem* deserialize(const nlohmann::json& json, NodeItem* node);

    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

    private:
        adapter_t m_adapter;
        NodeItem* m_node;
        ConnectionItem* m_connection;
    };
} // !namespace discnet::sim::ui