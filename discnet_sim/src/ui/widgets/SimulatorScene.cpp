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

            switch (tool_type)
            {
                case ToolBoxItemType::Node:
                {
                    std::lock_guard<std::mutex> lock {m_mutex};
                    auto node = new NodeItem();
                    m_items.push_back({++s_item_index, node});
                    addItem(node);
                    node->setPos(event->scenePos() - QPointF(64, 64));
                    break;
                }
                case ToolBoxItemType::Router:
                {
                    std::lock_guard<std::mutex> lock {m_mutex};
                    auto router = new RouterItem();
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
            }
            auto router = dynamic_cast<RouterItem*>(item);
            if (router)
            {
                auto action = menu->addAction("ROUTER");
                action->setEnabled(false);
            }

            auto deleteAction = new QAction(QIcon(":/images/delete.png"), QString("&Delete"));
            deleteAction->setShortcut(QString("Delete"));
            deleteAction->setStatusTip(QString("Delete item from diagram"));
            connect(deleteAction, &QAction::triggered, this, &SimulatorScene::onMenuDeletePressed);

            auto propertiesAction = new QAction(QIcon(":/images/properties.png"), QString("&Properties"));
            propertiesAction->setShortcut(QString("Properties"));
            propertiesAction->setStatusTip(QString("Item Properties"));
            connect(propertiesAction, &QAction::triggered, this, &SimulatorScene::onMenuProperiesPressed);

            menu->addAction(deleteAction);
            menu->addAction(propertiesAction);
            menu->popup(event->screenPos());
        }
        else
        {
            QGraphicsScene::contextMenuEvent(event);
        }
    }

    void SimulatorScene::onMenuDeletePressed()
    {
        QList<QGraphicsItem *> selectedItems = this->selectedItems();
        for (QGraphicsItem *item : std::as_const(selectedItems)) 
        {
            this->removeItem(item);
            delete item;
        }
    }

    void SimulatorScene::onMenuProperiesPressed()
    {

    }
} // !namespace discnet::sim::ui