/*
 *
 */

#pragma once

#include <QGraphicsPixmapItem>

namespace discnet::sim::ui
{
    class RouterItem : public QGraphicsPixmapItem 
    {
    public:
        RouterItem(const std::string& name, QGraphicsItem *parent = nullptr);
    
    protected:
        std::string m_name;
    };
} // !namespace discnet::sim::ui