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

        void set_connection(ConnectionItem* connection);
        ConnectionItem* connection();
        QPointF center() const;
        QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    private:
        adapter_t m_adapter;
        ConnectionItem* m_connection;
    };
} // !namespace discnet::sim::ui