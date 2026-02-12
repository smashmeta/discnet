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

    adapter_t AdapterItem::adapter() const
    {
        return m_adapter;
    }

    nlohmann::json AdapterItem::serialize() const
    {
        nlohmann::json addresses = nlohmann::json::array();
        for (const auto& address : m_adapter.m_address_list)
        {
            addresses.push_back(
                { 
                    { "address", address.first.to_string() },
                    { "mask", address.second.to_string() }
                }
            );
        }

         nlohmann::json position = {
            { "x", scenePos().x() },
            { "y", scenePos().y() }
         };

        nlohmann::json result = {
            { "node_internal_id", m_node->internal_id() },
            { "guid", m_adapter.m_guid },
            { "mac", m_adapter.m_mac_address },
            { "index", m_adapter.m_index },
            { "name", m_adapter.m_name },
            { "description", m_adapter.m_description },
            { "loopback", m_adapter.m_loopback },
            { "enabled", m_adapter.m_enabled },
            { "addresses", addresses },
            { "gateway", m_adapter.m_gateway.to_string() },
            { "mtu", m_adapter.m_mtu },
            { "position", position }
        };

        return result;
    }

    AdapterItem* AdapterItem::deserialize(const nlohmann::json& json, NodeItem* node)
    {
        AdapterItem* result = nullptr;

        adapter_t adapter;
        adapter.m_guid = json["guid"].get<std::string>();
        adapter.m_mac_address = json["mac"].get<std::string>();
        adapter.m_index = json["index"].get<uint8_t>();
        adapter.m_name = json["name"].get<std::string>();
        adapter.m_description = json["description"].get<std::string>();
        adapter.m_loopback= json["loopback"].get<bool>();
        adapter.m_enabled= json["enabled"].get<bool>();

        for (const auto& address_json : json["addresses"])
        {
            auto address = boost::asio::ip::make_address_v4(address_json["address"].get<std::string>());
            auto mask = boost::asio::ip::make_address_v4(address_json["mask"].get<std::string>());
            adapter.m_address_list.push_back(std::make_pair(address, mask));
        }

        adapter.m_gateway = boost::asio::ip::make_address_v4(json["gateway"].get<std::string>());
        adapter.m_mtu = json["mtu"].get<uint64_t>();

        auto x = json["position"]["x"].get<double>();
        auto y = json["position"]["y"].get<double>();
        result = new AdapterItem(adapter, node);
        result->setPos(QPointF(x, y));
        node->add_adapter(result);
        
        return result;
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