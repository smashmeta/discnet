/*
 *
 */

#include <atomic>
#include <random>
#include <QUuid>
#include <discnet/typedefs.hpp>
#include "ui/widgets/NodeItem.h"
#include "ui/widgets/AdapterItem.h"
#include "ui/widgets/SimulatorScene.h"


namespace discnet::sim::ui
{
    NodeItem::NodeItem(const uint16_t node_id, SimulatorScene* scene, QGraphicsItem *parent)
        : QGraphicsPixmapItem(parent), m_node_id(node_id), m_scene(scene)
    {
        static std::atomic<int> s_sequence_number = 0;
        m_internal_id = s_sequence_number.fetch_add(1, std::memory_order_relaxed);

        setPixmap(QPixmap(":/images/node.png"));
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        m_dialog = new NodeDialog(node_id);
        m_adapterDialog = new AdapterDialog(m_internal_id);

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

    void NodeItem::add_adapter(AdapterItem* adapter)
    {
        m_adapters.push_back(adapter);
    }

    void NodeItem::remove_adapter(AdapterItem* adapter)
    {
        auto existing = std::find(m_adapters.begin(), m_adapters.end(), adapter);
        if (existing != m_adapters.end())
        {
            m_adapters.erase(existing);
        }
    }

    std::vector<AdapterItem*> NodeItem::adapters()
    {
        return m_adapters;
    }

    uint32_t NodeItem::internal_id() const
    {
        return m_internal_id;
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

    nlohmann::json NodeItem::serialize() const
    {
        nlohmann::json position = {
            { "x", scenePos().x() },
            { "y", scenePos().y() }
        };
        
        nlohmann::json result = {
            { "internal_id", m_internal_id },
            { "node_id", m_node_id },
            { "position", position }
        };

        return result;
    }

    NodeItem* NodeItem::deserialize(const nlohmann::json& json, SimulatorScene* scene)
    {
        NodeItem* result = nullptr;
        auto node_id = json["node_id"].get<uint16_t>();
        auto x = json["position"]["x"].get<double>();
        auto y = json["position"]["y"].get<double>();
        result = new NodeItem(node_id, scene);
        result->setPos(QPointF(x, y));
        
        return result;
    }
} // !namespace discnet::sim::ui