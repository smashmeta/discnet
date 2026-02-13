/*
 *
 */

#include <iostream>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <nlohmann/json.hpp>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneContextMenuEvent>
#include "ui/widgets/ToolBoxItem.h"
#include "ui/widgets/SimulatorScene.h"


namespace discnet::sim::ui
{
    SimulatorScene::SimulatorScene(QObject *parent)
        : QGraphicsScene(parent), m_connector(nullptr)
    {
        // nothing for now
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
                    break;
                }
                case ToolBoxItemType::Router:
                {
                    auto router = new RouterItem(std::format("router_{}", router_sequence_number.fetch_add(1, std::memory_order_relaxed)));
                    addItem(router);
                    router->setPos(event->scenePos() - QPointF(64, 64));
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
        QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
        if (item) 
        {
            this->clearSelection();
            item->setSelected(true);

            auto menu = new QMenu();
            auto node = dynamic_cast<NodeItem*>(item);
            if (node)
            {
                auto action = menu->addAction("NODE");
                action->setEnabled(false);

                auto addAdapterAction = new QAction(QIcon(":/images/adapter.png"), QString("&Add Adapter"));
                addAdapterAction->setShortcut(QString("Add Adapter"));
                addAdapterAction->setStatusTip(QString("Add Adapter"));
                connect(addAdapterAction, &QAction::triggered, this, &SimulatorScene::onAddAdapterPressed);
                menu->addAction(addAdapterAction);
            }
            auto router = dynamic_cast<RouterItem*>(item);
            if (router)
            {
                auto action = menu->addAction("ROUTER");
                action->setEnabled(false);
            }
            
            auto propertiesAction = new QAction(QIcon(":/images/properties.png"), QString("&Properties"));
            propertiesAction->setShortcut(QString("Properties"));
            propertiesAction->setStatusTip(QString("Item Properties"));
            connect(propertiesAction, &QAction::triggered, this, &SimulatorScene::onMenuProperiesPressed);

            auto deleteAction = new QAction(QIcon(":/images/delete.png"), QString("&Delete"));
            deleteAction->setShortcut(QString("Delete"));
            deleteAction->setStatusTip(QString("Delete item from diagram"));
            connect(deleteAction, &QAction::triggered, this, &SimulatorScene::onMenuDeletePressed);

            menu->addAction(propertiesAction);
            menu->addAction(deleteAction);
            menu->popup(event->screenPos());
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
                    if (router && adapter)
                    {
                        auto previous = adapter->connection();
                        if (previous)
                        {
                            router->remove_connection(previous);
                            removeItem(previous);
                        }

                        auto connection = new ConnectionItem(adapter, router);
                        connection->setZValue(-1000.0f);
                        addItem(connection);
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
            }

            for (auto* node : nodes | std::views::values)
            {
                addItem(node);
            }

            for (auto* adapter : adapters | std::views::values)
            {
                addItem(adapter);
            }

            for (auto* connection : connections | std::views::values)
            {
                connection->setZValue(-1000.0f);
                addItem(connection);
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
                removeAdapter(adapter);
                return;
            }

            if (auto router = dynamic_cast<RouterItem*>(item); router)
            {
                removeRouter(router);
                return;
            }

            if (auto node = dynamic_cast<NodeItem*>(item); node)
            {
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

    void SimulatorScene::adapterEvent(const uint16_t internal_id, const adapter_t adapter)
    {
        for (auto& item : this->items())
        {
            auto node = dynamic_cast<NodeItem*>(item);
            if (node && node->internal_id() == internal_id)
            {
                auto adapter_item = new AdapterItem(adapter, node);
                auto position = node->pos() + QPointF(10.0f, 10.0f);
                adapter_item->setPos(position);
                addItem(adapter_item);
            }
        }
    }
} // !namespace discnet::sim::ui