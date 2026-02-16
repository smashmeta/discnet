/*
 *
 */

#pragma once

#include <QAbstractTableModel>
#include <discnet/route.hpp>

namespace discnet::sim::models
{
    class RouteModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        RouteModel(QObject* parent = nullptr);

        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        
        std::vector<discnet::route_t> entries() const;
        void update(const discnet::route_t& entry);
        bool remove(const discnet::route_identifier_t& identifier);
    private:
        std::vector<discnet::route_t> m_data;
    };
} // !namespace discnet::sim::ui