/*
 *
 */

#include "models/RouteModel.h"

namespace discnet::sim::models
{
    RouteModel::RouteModel(QObject* parent)
        : QAbstractTableModel(parent)
    {
        // nothing for now
    }

    QVariant RouteModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
                case 0:
                    return "NodeId";
                case 1:
                    return "Online";
                case 2: 
                    return "Time";
            }
        }

        return QVariant();
    }

    int RouteModel::rowCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return static_cast<int>(m_data.size());
    }

    int RouteModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return 3;
    }

    QVariant RouteModel::data(const QModelIndex& index, int role) const
    {
        int row = index.row();
        int column = index.column();

        if (m_data.size() > row)
        {
            auto entry = std::next(m_data.begin(), row);

            switch (role)
            {
                case Qt::EditRole:
                case Qt::DisplayRole:
                case Qt::ToolTipRole:
                {
                    switch (column)
                    {
                        case 0:
                            return  ""; // discnet::to_string(entry.m_identifier).c_str();
                        case 1:
                            return ""; // entry.m_status.m_online;
                        case 2: 
                            return ""; // std::format("{}", entry.m_last_discovery).c_str();

                    }
                }
            }
        }

        return QVariant();
    }

    Qt::ItemFlags RouteModel::flags(const QModelIndex& index) const
    {
        return QAbstractTableModel::flags(index);
    }

    std::vector<discnet::route_t> RouteModel::entries() const
    {
        return m_data;
    }

    void RouteModel::update(const discnet::route_t& entry)
    {
        for (auto& existing : m_data)
        {
            if (existing.m_identifier == entry.m_identifier)
            {

            }
        }
    }

    bool RouteModel::remove([[maybe_unused]] const discnet::route_identifier_t& identifier)
    {
        // todo: implement
        return false;
    }
}