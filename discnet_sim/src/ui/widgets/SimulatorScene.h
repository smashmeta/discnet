/*
 *
 */

#pragma once

#include <mutex>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QGraphicsScene>
#include "ui/widgets/RouterItem.h"
#include "ui/widgets/NodeItem.h"
#include "ui/widgets/AdapterItem.h"
#include "ui/widgets/ConnectionItem.h"
#include "simulator/simulator.h"


namespace discnet::sim::ui
{
    struct ItemHandle
    {
        uint32_t m_identifier;
        QGraphicsItem* m_handle;
    };

    class SimulatorScene : public QGraphicsScene
    {
        Q_OBJECT
    public:
        explicit SimulatorScene(QObject *parent = nullptr);

        void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
        void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
        void dropEvent(QGraphicsSceneDragDropEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;
        void keyReleaseEvent(QKeyEvent *event) override;
        void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
        void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

public slots:
        void adapterEvent(const uint16_t node_id, const adapter_t adapter);

private slots:
        void onMenuDeletePressed();
        void onMenuProperiesPressed();
        void onAddAdapterPressed();

private:
        void removeNode(NodeItem* node);
        void removeAdapter(AdapterItem* adapter);
        void removeRouter(RouterItem* router);

private:
        std::mutex m_mutex;
        static uint32_t s_item_index;
        std::vector<ItemHandle> m_items;
        logic::simulator m_simulator;

        std::mutex m_connector_mutex;
        QGraphicsLineItem* m_connector;
    };
} // !namespace discnet::sim::ui