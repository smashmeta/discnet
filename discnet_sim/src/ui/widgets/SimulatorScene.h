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

        void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    private:
        std::mutex m_mutex;
        static uint32_t s_item_index;
        std::vector<ItemHandle> m_items;
    };
} // !namespace discnet::sim::ui