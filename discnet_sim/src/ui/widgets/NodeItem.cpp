/*
 *
 */

#include <QUuid>
#include <random>
#include <discnet/typedefs.hpp>
#include "ui/widgets/NodeItem.h"
#include "ui/widgets/AdapterItem.h"
#include "ui/widgets/SimulatorScene.h"


namespace discnet::sim::ui
{
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

    void NodeItem::add_adapter_item(AdapterItem* adapter)
    {
        m_adapters.push_back(adapter);
    }

    std::vector<AdapterItem*> NodeItem::adapters()
    {
        return m_adapters;
    }

    void NodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) 
    {
        QPointF delta = event->scenePos() - event->lastScenePos();
        this->moveBy(delta.x(), delta.y());
        for (QGraphicsItem* adapter : m_adapters) 
        {
            adapter->moveBy(delta.x(), delta.y());
        }

        QGraphicsPixmapItem::mouseMoveEvent(event);
    }
} // !namespace discnet::sim::ui