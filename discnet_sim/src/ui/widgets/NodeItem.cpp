/*
 *
 */

#include <QUuid>
#include <random>
#include <discnet/typedefs.hpp>
#include "ui/widgets/NodeItem.h"
#include "ui/widgets/SimulatorScene.h"


namespace discnet::sim::ui
{
    AdapterItem::AdapterItem(const QPixmap& pixmap, QGraphicsItem* parent)
        : QGraphicsPixmapItem(pixmap, parent)
    {
        // nothing 
    }

    AdapterItem::~AdapterItem()
    {
        // nothing for now
    }

    void AdapterItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) 
    {
        QGraphicsPixmapItem::mouseMoveEvent(event); // Call base class if needed
    }

    NodeItem::NodeItem(const uint16_t node_id, SimulatorScene* scene, QGraphicsItem *parent)
        : QGraphicsPixmapItem(parent), m_node_id(node_id), m_scene(scene)
    {
        setPixmap(QPixmap(":/images/node.png"));
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        m_dialog = new NodeDialog(node_id);
        m_adapterDialog = new AdapterDialog(node_id);

        m_scene->connect(m_adapterDialog, &AdapterDialog::accepted, m_scene, &SimulatorScene::adapterEvent);
    }

    NodeItem::~NodeItem()
    {
        delete m_dialog;
    }

    void NodeItem::showProperties()
    {
        m_dialog->show(); 
    }

    void NodeItem::addAdapter()
    {
        adapter_t adapter;

        adapter.m_guid = QUuid::createUuid().toString().toStdString();
        
        std::random_device random_device;
        std::uniform_int_distribution<unsigned int> distribution(0, 255);
        for (size_t index = 0; index < 6; ++index) 
        {
            auto byte = static_cast<char>(distribution(random_device));

            if (index > 0)
            {
                adapter.m_mac_address += ":";
            }

            adapter.m_mac_address += std::format("{:02X}", byte);
        }

        adapter.m_index = static_cast<uint8_t>(m_adapters.size());
        adapter.m_name = std::format("eth{}", adapter.m_index);
        adapter.m_description = std::format("{} adapter description.", adapter.m_name);
        adapter.m_address_list.push_back(std::make_pair<address_t, address_t>(boost::asio::ip::make_address_v4("192.200.1.1"), boost::asio::ip::make_address_v4("255.255.255.0")));
        adapter.m_enabled = true;
        adapter.m_loopback = false;

        m_adapterDialog->setData(adapter);
        m_adapterDialog->show();
    }

    uint16_t NodeItem::node_id() const
    {
        return m_node_id;
    }

    void NodeItem::adapter_accepted(const adapter_t adapter)
    {
        auto existing = std::find_if(m_adapters.begin(), m_adapters.end(), [&](const auto val){ return val.m_guid == adapter.m_guid; });
        if (existing == m_adapters.end())
        {
            m_adapters.push_back(adapter);
            auto adapterItem = new AdapterItem(QPixmap(":/images/adapter.png"), this);
            adapterItem->setPos(10, 10);
            m_scene->addItem(adapterItem);
        }
    }

    void NodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) 
    {
        if (flags() & QGraphicsItem::ItemIsMovable) 
        {
            bool processed = false;
            auto test = event->pos();
            QPointF scenePos = mapToScene(event->scenePos()); // From View
            QPointF localPos = mapFromScene(scenePos);
            for (auto child : this->childItems())
            {
                auto bounding_box = child->boundingRect();
                if (bounding_box.contains(test))
                {
                    auto adapter = dynamic_cast<AdapterItem*>(child);
                    if (adapter)
                    {
                        adapter->mouseMoveEvent(event);
                        processed = true;
                    }
                }
            }

            if (!processed)
            {
                QGraphicsItem::mouseMoveEvent(event);
            }

            // QPointF scenePos = mapToScene(event->scenePos()); // From View
            // QGraphicsItem* item = m_scene->itemAt(scenePos, QTransform());
            // auto adapter = dynamic_cast<AdapterItem*>(item);
            // if (adapter)
            // {
            //     adapter->mouseMoveEvent(event);
            // }
            // else
            // {
            //     QGraphicsItem::mouseMoveEvent(event);
            // }
        }
    }
} // !namespace discnet::sim::ui