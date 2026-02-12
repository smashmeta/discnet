/*
 *
 */

#pragma once

#include <nlohmann/json.hpp>
#include <QGraphicsItem>
#include <QGraphicsLineItem>


namespace discnet::sim::ui
{
    class AdapterItem;
    class RouterItem;

    class ConnectionItem : public QGraphicsLineItem
    {
    public:
        ConnectionItem(AdapterItem* adapter, RouterItem* router, QGraphicsItem *parent = nullptr);
        
        void update_adapter(AdapterItem* adapter);
        void update_router(RouterItem* router);

        AdapterItem* adapter();
        RouterItem* router();

        nlohmann::json serialize() const;

    private:
        void internal_update();
    
        uint32_t m_internal_id;
        AdapterItem* m_adapter;
        RouterItem* m_router;
    };
} // !namespace discnet::sim::ui