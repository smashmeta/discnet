/*
 *
 */

#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include "ui/widgets/NodeDialog.h"
#include "ui/widgets/AdapterDialog.h"
#include <discnet/adapter.hpp>

namespace discnet::sim::ui
{
    class SimulatorScene;

    class AdapterItem : public QGraphicsPixmapItem
    {
    public:
        AdapterItem(const QPixmap& pixmap, QGraphicsItem *parent = nullptr);
        ~AdapterItem();

        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    };

    class NodeItem : public QGraphicsPixmapItem 
    {
    public:
        NodeItem(const uint16_t nodeId, SimulatorScene* scene, QGraphicsItem *parent = nullptr);
        ~NodeItem();

        void showProperties();
        void addAdapter();
        uint16_t node_id() const;

        void adapter_accepted(const adapter_t adapter);
        
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    private:
        uint16_t m_node_id;
        SimulatorScene* m_scene;
        NodeDialog* m_dialog;
        AdapterDialog* m_adapterDialog;
        std::vector<discnet::adapter_t> m_adapters;
    };
} // !namespace discnet::sim::ui