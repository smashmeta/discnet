/*
 *
 */

#include <QLabel>
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>
#include "ui/widgets/ToolBoxItem.h"


namespace discnet::sim::ui
{
   ToolBoxItem::ToolBoxItem(const ToolBoxItemType type, QWidget *parent)
       : QWidget(parent), m_type(type)
   {
       setMinimumSize(138, 128);
       switch (m_type)
       {
           case ToolBoxItemType::Node:
               m_icon = QPixmap(":/images/node.png");
               break;
           case ToolBoxItemType::Router:
               m_icon = QPixmap(":/images/router.png");
               break;
           default:
               m_icon = QPixmap(":/images/unknown.png");
       }
       auto label = new QLabel("", this);
       label->setPixmap(m_icon);
       label->setScaledContents(true);
       label->setGeometry(QRect({10, 0}, m_icon.size()));
   }

   void ToolBoxItem::mousePressEvent(QMouseEvent *event)
   {
        if (event->buttons() & Qt::LeftButton)
        {
            QByteArray itemData;
            QDataStream dataStream(&itemData, QIODevice::WriteOnly);
            dataStream << static_cast<int>(m_type);
            QMimeData *mimeData = new QMimeData;
            mimeData->setData("application/qt-discnet-tool-item", itemData);
            QDrag *drag = new QDrag(this);
            drag->setMimeData(mimeData);
            drag->setPixmap(m_icon);
            drag->setHotSpot(QPoint(m_icon.size().width()/2, m_icon.size().height()/2));
            drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
        }
   }
} // !namespace discnet::sim::ui