/*
 *
 */

#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneContextMenuEvent>
#include "ui/widgets/ToolBoxItem.h"
#include "ui/widgets/SimulatorScene.h"


namespace discnet::sim::ui
{
    uint32_t SimulatorScene::s_item_index = 0;

    SimulatorScene::SimulatorScene(QObject *parent)
        : QGraphicsScene(parent)
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
        if (event->mimeData()->hasFormat("application/qt-discnet-tool-item")) {
            QByteArray itemData = event->mimeData()->data("application/qt-discnet-tool-item");
            QDataStream dataStream(&itemData, QIODevice::ReadOnly);

            ToolBoxItemType tool_type;
            dataStream >> tool_type;
            event->acceptProposedAction();

            static uint16_t node_id_sequence_number = 1000;
            static uint16_t router_sequence_number = 1;

            switch (tool_type)
            {
                case ToolBoxItemType::Node:
                {
                    std::lock_guard<std::mutex> lock {m_mutex};
                    auto node = new NodeItem(node_id_sequence_number++, this);
                    m_items.push_back({++s_item_index, node});
                    addItem(node);
                    node->setPos(event->scenePos() - QPointF(64, 64));
                    break;
                }
                case ToolBoxItemType::Router:
                {
                    std::lock_guard<std::mutex> lock {m_mutex};
                    auto router = new RouterItem(std::format("router_{}", router_sequence_number++));
                    m_items.push_back({++s_item_index, router});
                    addItem(router);
                    router->setPos(event->scenePos() - QPointF(64, 64));
                    break;
                }
            }
        } else {
            event->ignore();
        }
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

    void SimulatorScene::onMenuDeletePressed()
    {
        QList<QGraphicsItem *> selected = this->selectedItems();
        if (selected.size() == 1)
        {
            auto item = selected.first();
            this->removeItem(item);
            delete item;
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

    void SimulatorScene::adapterEvent([[maybe_unused]] const uint16_t node_id, [[maybe_unused]] const adapter_t adapter)
    {
        for (auto& item : this->items())
        {
            auto node = dynamic_cast<NodeItem*>(item);
            if (node)
            {
                if (node->node_id() == node_id)
                {
                    node->adapter_accepted(adapter);
                }
            }
        }
    }
} // !namespace discnet::sim::ui