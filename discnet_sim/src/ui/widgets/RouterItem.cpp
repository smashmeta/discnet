/*
 *
 */

#include "ui/widgets/RouterItem.h"
#include <QFont>

namespace discnet::sim::ui
{
    RouterItem::RouterItem(const std::string& name, QGraphicsItem *parent)
        : QGraphicsPixmapItem(parent), m_name(name)
    {
        setPixmap(QPixmap(":/images/router.png"));
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);

        auto* name_label = new QGraphicsTextItem(name.c_str(), this);
        name_label->setPos(10, 0);
        name_label->setDefaultTextColor(Qt::blue);
        QFont font = name_label->font();
        font.setPointSize(20);
        name_label->setFont(font);
    }
} // !namespace discnet::sim::ui