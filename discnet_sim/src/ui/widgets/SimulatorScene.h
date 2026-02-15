/*
 *
 */

#pragma once

#include <mutex>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QAction>
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
        ~SimulatorScene();

        void update(const discnet::time_point_t& time);

        void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
        void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
        void dropEvent(QGraphicsSceneDragDropEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;
        void keyReleaseEvent(QKeyEvent *event) override;
        void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
        void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

        std::string serialize() const;
        bool load(const std::string& json_str);

public slots:
        void adapterEvent(const uint16_t internal_id, const adapter_t adapter);

private slots:
        void onMenuDeletePressed();
        void onMenuProperiesPressed();
        void onAddAdapterPressed();

private:
        void setup_menus();
        void setup_adapter_menu();
        void setup_node_menu();
        void setup_router_menu();

        void removeNode(NodeItem* node);
        void removeAdapter(AdapterItem* adapter);
        void removeRouter(RouterItem* router);

private:
        std::shared_ptr<logic::simulator> m_simulator;
        std::mutex m_connector_mutex;
        QGraphicsLineItem* m_connector;

        QAction* m_properties_action;
        QAction* m_delete_action;

        QMenu* m_adapter_menu;
        QMenu* m_node_menu;
        QMenu* m_router_menu;

        discnet::application::configuration_t m_configuration;
    };
} // !namespace discnet::sim::ui