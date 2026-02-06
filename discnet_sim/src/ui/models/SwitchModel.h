/*
 *
 */

#pragma once

#include <QAbstractTableModel>

namespace discnet::sim::models
{
    enum class SwitchType
    {
        Fast,
        Medium,
        Slow
    };

    struct Switch
    {
        uint16_t m_identifier;
        std::string m_name;
        SwitchType m_type;
    };

    class SwitchModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        SwitchModel(QObject* parent = nullptr);

        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        
        std::vector<Switch> entries() const;
        void add(const Switch& entry);
        bool remove(const QModelIndex& index);
    private:
        std::vector<Switch> m_data;
    };
} // !namespace discnet::sim::ui