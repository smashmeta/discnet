/*
 *
 */

#pragma once

#include <QGraphicsPixmapItem>
#include <discnet/adapter.hpp>


namespace discnet::sim::ui
{
    class AdapterItem : public QGraphicsPixmapItem
    {
    public:
        AdapterItem(const adapter_t& adapter, QGraphicsItem *parent = nullptr);
        ~AdapterItem();
    private:
        adapter_t m_adapter;
    };
} // !namespace discnet::sim::ui