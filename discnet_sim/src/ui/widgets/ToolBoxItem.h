/* 
 *
 */

 #pragma once

 #include <QWidget>
 #include <QPixmap>

namespace discnet::sim::ui
{
    enum class ToolBoxItemType
    {
        Node,
        Router
    };

    class ToolBoxItem : public QWidget
    {
    public:
        explicit ToolBoxItem(const ToolBoxItemType type, QWidget *parent = nullptr);
    protected:
        // void dragEnterEvent(QDragEnterEvent *event) override;
        // void dragMoveEvent(QDragMoveEvent *event) override;
        // void dropEvent(QDropEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
    protected:
        ToolBoxItemType m_type;
        QPixmap m_icon;
    };
} // !namespace discnet::sim::ui