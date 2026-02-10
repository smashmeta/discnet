/*
 *
 */

#pragma once

#include <QGraphicsPixmapItem>
#include <discnet/adapter.hpp>


namespace discnet::sim::ui
{
    class ConnectionItem;

    class AdapterItem : public QGraphicsPixmapItem
    {
    public:
        AdapterItem(const adapter_t& adapter, QGraphicsItem *parent = nullptr);
        ~AdapterItem();

        void add(ConnectionItem* connection);
        QPointF center() const;
        QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    private:
        adapter_t m_adapter;
        std::vector<ConnectionItem*> m_connections;
    };
} // !namespace discnet::sim::ui