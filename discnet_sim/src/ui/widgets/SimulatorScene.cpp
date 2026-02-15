/*
 *
 */

#include <iostream>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/qt_sinks.h>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneContextMenuEvent>
#include "ui/widgets/ToolBoxItem.h"
#include "ui/widgets/SimulatorScene.h"


namespace discnet::sim::ui
{
    SimulatorScene::SimulatorScene(QObject *parent)
        : QGraphicsScene(parent), m_connector(nullptr)
    {
        m_configuration.m_multicast_address = boost::asio::ip::make_address_v4("224.1.1.2");
        m_configuration.m_multicast_port = 1337;

        auto network_traffic_simulator_log_view = new QTextEdit();
        auto network_traffic_simulator_sink = std::make_shared<spdlog::sinks::qt_color_sink_mt>(network_traffic_simulator_log_view, 10000);
        auto network_traffic_simulator_logger = std::make_shared<spdlog::logger>("traffic", network_traffic_simulator_sink);
        spdlog::register_logger(network_traffic_simulator_logger);
        m_simulator = std::make_shared<discnet::sim::logic::simulator>();

        setup_menus();
    }

    void SimulatorScene::update(const discnet::time_point_t& time)
    {
        m_simulator->update(time);
    }

    void SimulatorScene::setup_menus()
    {
        m_properties_action = new QAction(QIcon(":/images/properties.png"), QString("&Properties"));
        m_properties_action->setShortcut(QString("Properties"));
        m_properties_action->setStatusTip(QString("Item Properties"));
        connect(m_properties_action, &QAction::triggered, this, &SimulatorScene::onMenuProperiesPressed);

        m_delete_action = new QAction(QIcon(":/images/delete.png"), QString("&Delete"));
        m_delete_action->setShortcut(QString("Delete"));
        m_delete_action->setStatusTip(QString("Delete item from diagram"));
        connect(m_delete_action, &QAction::triggered, this, &SimulatorScene::onMenuDeletePressed);

        setup_adapter_menu();
        setup_node_menu();
        setup_router_menu();
    }

    void SimulatorScene::setup_adapter_menu()
    {
        m_adapter_menu = new QMenu();
        auto action = m_adapter_menu->addAction("ADAPTER");
        action->setEnabled(false);
        m_adapter_menu->addAction(m_properties_action);
        m_adapter_menu->addAction(m_delete_action);
    }

    void SimulatorScene::setup_node_menu()
    {
        m_node_menu = new QMenu();
        auto action = m_node_menu->addAction("NODE");
        action->setEnabled(false);
        auto addAdapterAction = new QAction(QIcon(":/images/adapter.png"), QString("&Add Adapter"));
        connect(addAdapterAction, &QAction::triggered, this, &SimulatorScene::onAddAdapterPressed);
        m_node_menu->addAction(m_properties_action);
        m_node_menu->addAction(addAdapterAction);
        m_node_menu->addAction(m_delete_action);
    }

    void SimulatorScene::setup_router_menu()
    {
        m_router_menu = new QMenu();
        auto action = m_router_menu->addAction("ROUTER");
        action->setEnabled(false);
        m_router_menu->addAction(m_properties_action);
        m_router_menu->addAction(m_delete_action);
    }

    SimulatorScene::~SimulatorScene()
    {
        delete m_node_menu;
        delete m_adapter_menu;
        delete m_router_menu;
    }

    void SimulatorScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
    {
        if (event->mimeData()->hasFormat("application/qt-discnet-tool-item")) 
        {
            event->acceptProposedAction();
        }
        else
        {
            event->ignore();
        }
    }

    void SimulatorScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
    {
        if (event->mimeData()->hasFormat("application/qt-discnet-tool-item")) 
        {
            event->acceptProposedAction();
        } else 
        {
            event->ignore();
        }
    }

    void SimulatorScene::dropEvent(QGraphicsSceneDragDropEvent *event)
    {
        if (event->mimeData()->hasFormat("application/qt-discnet-tool-item")) 
        {
            QByteArray itemData = event->mimeData()->data("application/qt-discnet-tool-item");
            QDataStream dataStream(&itemData, QIODevice::ReadOnly);

            ToolBoxItemType tool_type;
            dataStream >> tool_type;
            event->acceptProposedAction();

            static std::atomic<uint16_t> node_id_sequence_number = 1000;
            static std::atomic<uint16_t> router_sequence_number = 1;

            switch (tool_type)
            {
                case ToolBoxItemType::Node:
                {
                    auto node = new NodeItem(node_id_sequence_number.fetch_add(1, std::memory_order_relaxed), this);
                    addItem(node);
                    node->setPos(event->scenePos() - QPointF(64, 64));

                    discnet::application::configuration_t configuration = m_configuration;
                    configuration.m_log_instance_id = std::format("node_{}", node->internal_id());
                    configuration.m_node_id = node->node_id();
                    m_simulator->add_node(node->internal_id(), configuration, node->dialog()->log());

                    break;
                }
                case ToolBoxItemType::Router:
                {
                    auto router = new RouterItem(std::format("router_{}", router_sequence_number.fetch_add(1, std::memory_order_relaxed)));
                    addItem(router);
                    router->setPos(event->scenePos() - QPointF(64, 64));

                    discnet::sim::logic::router_properties properties;
                    properties.m_drop_rate = 0.0f;
                    properties.m_latency = std::chrono::milliseconds(0);
                    m_simulator->add_router(router->internal_id(), properties);

                    break;
                }
            }
        } 
        else 
        {
            event->ignore();
        }
    }

    void SimulatorScene::keyPressEvent(QKeyEvent *event)
    {
        if (event->key() == Qt::Key_Control) 
        {
            
        }
        QGraphicsScene::keyPressEvent(event);
    }

    void SimulatorScene::keyReleaseEvent(QKeyEvent *event) 
    {
        if (event->key() == Qt::Key_Control) 
        {
            std::lock_guard lock{m_connector_mutex};
            if (m_connector != nullptr)
            {
                removeItem(m_connector);
                delete m_connector;
                m_connector = nullptr;
            }
        }
        QGraphicsScene::keyReleaseEvent(event);
    }

    void SimulatorScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
    {
        auto cursor_position = event->scenePos();
        this->clearSelection();
        
        AdapterItem* adapter = nullptr;
        NodeItem* node = nullptr;
        RouterItem* router = nullptr;
        for (auto item : items(event->scenePos()))
        {
            if (auto temp = dynamic_cast<AdapterItem*>(item); temp)
            {
                adapter = temp;
            }
            if (auto temp = dynamic_cast<NodeItem*>(item); temp)
            {
                node = temp;
            }
            if (auto temp = dynamic_cast<RouterItem*>(item); temp)
            {
                router = temp;
            }
        }

        if (adapter)
        {
            adapter->setSelected(true);
            m_adapter_menu->popup(event->screenPos());
        }
        else if (node)
        {
            node->setSelected(true);
            m_node_menu->popup(event->screenPos());
        }
        else if (router)
        {
            router->setSelected(true);
            m_router_menu->popup(event->screenPos());
        }
        else
        {
            QGraphicsScene::contextMenuEvent(event);
        }
    }

    void SimulatorScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
    {
        if (event->modifiers() == Qt::KeyboardModifier::ControlModifier) 
        {
            QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
            if (item)
            {
                auto adapter = dynamic_cast<AdapterItem*>(item);
                if (adapter)
                {
                    std::lock_guard connector_lock{m_connector_mutex};
                    if (m_connector == nullptr)
                    {
                        
                        QPointF start = adapter->center();
                        QPointF end = event->scenePos();

                        auto brush = QBrush(Qt::GlobalColor::blue, Qt::BrushStyle::SolidPattern);
                        m_connector = addLine(start.x(), start.y(), end.x(), end.y(), QPen(brush, 4.0f));
                        addItem(m_connector);
                    }
                    
                }
            }
        }
        else
        {
            QGraphicsScene::mousePressEvent(event);
        }
    }

    void SimulatorScene::mouseMoveEvent([[maybe_unused]] QGraphicsSceneMouseEvent *event)
    {
        if (event->modifiers() == Qt::KeyboardModifier::ControlModifier) 
        {
            std::lock_guard connector_lock{m_connector_mutex};
            if (m_connector != nullptr)
            {
                bool snapped = false;
                auto line = m_connector->line();
                for (auto item : items(event->scenePos()))
                {
                    if (item)
                    {
                        auto router = dynamic_cast<RouterItem*>(item);
                        if (router)
                        {
                            QPointF router_position = router->center();
                            m_connector->setLine(line.p1().x(), line.p1().y(), router_position.x(), router_position.y());
                            auto brush = QBrush(Qt::GlobalColor::green, Qt::BrushStyle::SolidPattern);
                            m_connector->setPen(QPen(brush, 6.0f));
                            snapped = true;
                            break;
                        }
                    }
                }
                
                if (!snapped)
                {
                    auto brush = QBrush(Qt::GlobalColor::blue, Qt::BrushStyle::SolidPattern);
                    m_connector->setPen(QPen(brush, 4.0f));
                    m_connector->setLine(line.p1().x(), line.p1().y(), event->scenePos().x(), event->scenePos().y());
                }
            }
        }

        if (event->buttons() & Qt::LeftButton)
        {
            QGraphicsScene::mouseMoveEvent(event);
        }
        else
        {
            event->ignore();
        }
    }

    void SimulatorScene::mouseReleaseEvent([[maybe_unused]] QGraphicsSceneMouseEvent *event)
    {
        if (event->modifiers() == Qt::KeyboardModifier::ControlModifier) 
        {
            std::lock_guard connector_lock{m_connector_mutex};
            if (m_connector != nullptr)
            {
                auto line = m_connector->line();
                QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
                if (item)
                {
                    AdapterItem* adapter = nullptr;
                    auto items_at_location = items(m_connector->line().p1());
                    for (auto curr : items_at_location)
                    {
                        auto temp = dynamic_cast<AdapterItem*>(curr);
                        if (temp)
                        {
                            adapter = temp;
                            break;
                        }
                    }

                    auto router = dynamic_cast<RouterItem*>(item);
                    if (router == nullptr)
                    {
                        for (auto temp : items(event->scenePos())) 
                        {
                            if (auto rtemp = dynamic_cast<RouterItem*>(temp); rtemp)
                            {
                                router = rtemp;
                                break;
                            }
                        }
                    }
                    
                    if (router && adapter)
                    {
                        auto previous = adapter->connection();
                        if (previous)
                        {
                            router->remove_connection(previous);
                            removeItem(previous);

                            m_simulator->remove_link(adapter->node()->internal_id(), adapter->internal_id());
                        }

                        auto connection = new ConnectionItem(adapter, router);
                        connection->setZValue(-1000.0f);
                        addItem(connection);

                        m_simulator->add_link(adapter->node()->internal_id(), adapter->internal_id(), router->internal_id());
                    }
                }

                removeItem(m_connector);
                delete m_connector;
                m_connector = nullptr;
            }
        }

        QGraphicsScene::mouseReleaseEvent(event);
    }

    std::string SimulatorScene::serialize() const
    {
        nlohmann::json nodes = nlohmann::json::array();
        nlohmann::json adapters = nlohmann::json::array();
        nlohmann::json routers = nlohmann::json::array();
        nlohmann::json connections = nlohmann::json::array();
        for (const auto& item : items())
        {
            if (auto node = dynamic_cast<NodeItem*>(item); node)
            {
                nodes.push_back(node->serialize());
            }

            if (auto adapter = dynamic_cast<AdapterItem*>(item); adapter)
            {
                adapters.push_back(adapter->serialize());
            }

            if (auto router = dynamic_cast<RouterItem*>(item); router)
            {
                routers.push_back(router->serialize());
            }

            if (auto connection = dynamic_cast<ConnectionItem*>(item); connection)
            {
                connections.push_back(connection->serialize());
            }
        }

        nlohmann::json result = {
            { "nodes", nodes },
            { "adapters", adapters },
            { "routers", routers },
            { "connections", connections } 
        };

        return result.dump(4);
    }

    bool SimulatorScene::load(const std::string& json_str)
    {
        bool result = true;
        using router_entries = std::map<uint32_t, RouterItem*>;
        using node_entries = std::map<uint32_t, NodeItem*>;
        using adapter_entries = std::map<uint32_t, AdapterItem*>;
        using connection_entries = std::map<uint32_t, ConnectionItem*>;
        router_entries routers;
        node_entries nodes;
        adapter_entries adapters;
        connection_entries connections;

        try
        {
            m_simulator = std::make_shared<discnet::sim::logic::simulator>();

            auto json = nlohmann::json::parse(json_str);
            for (const auto& router_json : json["routers"])
            {
                auto router = RouterItem::deserialize(router_json);
                auto internal_id = router_json["internal_id"].get<uint32_t>();
                routers.insert(std::make_pair(internal_id, router));
            }
            for (const auto& node_json : json["nodes"])
            {
                auto node = NodeItem::deserialize(node_json, this);
                auto internal_id = node_json["internal_id"].get<uint32_t>();
                nodes.insert(std::make_pair(internal_id, node));
            }
            for (const auto& adapter_json : json["adapters"])
            {
                auto internal_node_id = adapter_json["node_internal_id"].get<uint32_t>();
                auto internal_id = adapter_json["internal_id"].get<uint32_t>();
                auto node = nodes.find(internal_node_id);

                auto adapter = AdapterItem::deserialize(adapter_json, node->second);
                adapters.insert(std::make_pair(internal_id, adapter));
            }
            for (const auto& connection_json : json["connections"])
            {
                auto adapter_internal_id = connection_json["adapter"].get<uint32_t>();
                auto router_internal_id = connection_json["router"].get<uint32_t>();
                auto internal_id = connection_json["internal_id"].get<uint32_t>();
                auto adapter = adapters.find(adapter_internal_id);
                auto router = routers.find(router_internal_id);
                auto connection = new ConnectionItem(adapter->second, router->second);
                connections.insert(std::make_pair(internal_id, connection));
            }

            clear();

            for (auto* router : routers | std::views::values)
            {
                addItem(router);

                discnet::sim::logic::router_properties properties;
                properties.m_drop_rate = 0.0f;
                properties.m_latency = std::chrono::milliseconds(0);
                m_simulator->add_router(router->internal_id(), properties);
            }

            for (auto* node : nodes | std::views::values)
            {
                addItem(node);

                discnet::application::configuration_t configuration = m_configuration;
                configuration.m_log_instance_id = std::format("node_{}", node->internal_id());
                configuration.m_node_id = node->node_id();
                m_simulator->add_node(node->internal_id(), configuration, node->dialog()->log());
            }

            for (auto* adapter : adapters | std::views::values)
            {
                addItem(adapter);

                m_simulator->add_adapter(adapter->node()->internal_id(), adapter->internal_id(), adapter->adapter());
            }

            m_simulator->update(sys_clock_t::now()); // trigger system to add added adapters to network traffic manager

            for (auto* connection : connections | std::views::values)
            {
                connection->setZValue(-1000.0f);
                addItem(connection);

                m_simulator->add_link(connection->adapter()->node()->internal_id(), connection->adapter()->internal_id(), connection->router()->internal_id());
            }
        }
        catch (...)
        {
            for (auto* router : routers | std::views::values)
            {
                delete router;
            }

            for (auto* node : nodes | std::views::values)
            {
                delete node;
            }

            for (auto* adapter : adapters | std::views::values)
            {
                delete adapter;
            }

            for (auto* connection : connections | std::views::values)
            {
                delete connection;
            }

            m_simulator = std::make_shared<discnet::sim::logic::simulator>();

            result = false;
        }

        return result;
    }

    void SimulatorScene::onMenuDeletePressed()
    {
        QList<QGraphicsItem *> selected = this->selectedItems();
        if (selected.size() == 1)
        {
            auto item = selected.first();
            
            if (auto adapter = dynamic_cast<AdapterItem*>(item); adapter)
            {
                m_simulator->remove_adapter(adapter->node()->internal_id(), adapter->internal_id());
                removeAdapter(adapter);
                return;
            }

            if (auto router = dynamic_cast<RouterItem*>(item); router)
            {
                m_simulator->remove_router(router->internal_id());
                removeRouter(router);
                return;
            }

            if (auto node = dynamic_cast<NodeItem*>(item); node)
            {
                m_simulator->remove_node(node->internal_id());
                removeNode(node);
                return;
            }
        }
    }

    void SimulatorScene::onMenuProperiesPressed()
    {
        QList<QGraphicsItem *> selected = this->selectedItems();
        if (selected.size() == 1)
        {
            auto item = selected.first();
            auto node = dynamic_cast<NodeItem*>(item);
            if (node)
            {
                node->showProperties();
            }
            
            auto router = dynamic_cast<RouterItem*>(item);
            if (router)
            {
                // todo: add router properties menu
            }
        }
    }

    void SimulatorScene::onAddAdapterPressed()
    {
        QList<QGraphicsItem *> selected = this->selectedItems();
        if (selected.size() == 1)
        {
            auto item = selected.first();
            auto node = dynamic_cast<NodeItem*>(item);
            if (node)
            {
                node->addAdapter();
            }
        }
    }

    void SimulatorScene::removeNode(NodeItem* node)
    {
        if (node)
        {
            for (auto adapter : node->adapters())
            {
                removeAdapter(adapter);
            }

            removeItem(node);
            delete node;
        }
    }

    void SimulatorScene::removeAdapter(AdapterItem* adapter)
    {
        if (adapter)
        {
            auto connection = adapter->connection();
            if (connection)
            {
                auto router = connection->router();
                router->remove_connection(connection);
            }

            auto node = adapter->node();
            node->remove_adapter(adapter);

            removeItem(connection);
            delete connection;

            removeItem(adapter);
            delete adapter;
        }
    }

    void SimulatorScene::removeRouter(RouterItem* router)
    {
        if (router)
        {
            for (auto connection : router->connections())
            {
                connection->adapter()->set_connection(nullptr);
                removeItem(connection);
                delete connection;
            }

            removeItem(router);
            delete router;
        }
    }

    void SimulatorScene::adapterEvent(const uint16_t internal_id, const adapter_t adapter_settings)
    {
        for (auto& item : this->items())
        {
            auto node = dynamic_cast<NodeItem*>(item);
            if (node && node->internal_id() == internal_id)
            {
                auto adapter_item = new AdapterItem(adapter_settings, node);
                auto position = node->pos() + QPointF(10.0f, 10.0f);
                adapter_item->setPos(position);
                addItem(adapter_item);

                m_simulator->add_adapter(node->internal_id(), adapter_item->internal_id(), adapter_settings);
            }
        }
    }
} // !namespace discnet::sim::ui