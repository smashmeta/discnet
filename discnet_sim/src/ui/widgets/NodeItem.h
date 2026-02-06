/*
 *
 */

#pragma once

#include <QGraphicsPixmapItem>
#include "ui/widgets/NodeDialog.h"

namespace discnet::sim::ui
{
    class NodeItem : public QGraphicsPixmapItem 
    {
    public:
        NodeItem(const uint16_t nodeId, QGraphicsItem *parent = nullptr);
        ~NodeItem();

        void showProperties() { m_dialog->show(); }
    private:
        NodeDialog* m_dialog;
    };
} // !namespace discnet::sim::ui