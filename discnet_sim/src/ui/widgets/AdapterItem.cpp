/*
 *
 */


#include <iostream>
#include "ui/widgets/ConnectionItem.h"
#include "ui/widgets/NodeItem.h"
#include "ui/widgets/AdapterItem.h"


namespace discnet::sim::ui
{
    AdapterItem::AdapterItem(const adapter_t& adapter, NodeItem* node, QGraphicsItem* parent)
        : QGraphicsPixmapItem(QPixmap(":/images/adapter.png"), parent), m_adapter(adapter),
        m_node(node), m_connection(nullptr)
    {
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }

    AdapterItem::~AdapterItem()
    {
        // nothing for now
    }

    NodeItem* AdapterItem::node()
    {
        return m_node;
    }

    void AdapterItem::set_connection(ConnectionItem* connection)
    {
        m_connection = connection;
    }

    ConnectionItem* AdapterItem::connection()
    {
        return m_connection;
    }

    QPointF AdapterItem::center() const
    {
        return scenePos() + boundingRect().center();
    }

    QVariant AdapterItem::itemChange(GraphicsItemChange change, const QVariant &value) 
    {
        if (change == ItemPositionHasChanged && scene()) 
        {
            if (m_connection) 
            {
                m_connection->update_adapter(this);
            }
        }

        return QGraphicsItem::itemChange(change, value);
    }

    void AdapterItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) 
    {
        QPointF new_position = event->scenePos() - boundingRect().center();
        QPointF node_position = m_node->scenePos() + (m_node->boundingRect().center()/2);
        auto distance = QLineF(node_position, new_position).length();
        if (distance < 90.0)
        {
            setPos(new_position);
        }
        else
        {
            QPointF endpoint = QPointF(new_position - node_position);
            QLineF line = QLineF(QPointF(0.0, 0.0), endpoint).unitVector();
            line.setLength(90);
            QPointF direction = line.p2();

            std::cout << std::format("direction: x: {}, y: {}.", direction.x(), direction.y()) << std::endl;
            setPos(node_position + direction);
        }
    }
} // !namespace discnet::sim::ui