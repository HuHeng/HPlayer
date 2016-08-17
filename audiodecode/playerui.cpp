#include "playerui.h"
#include <QMouseEvent>

/*a slider can response to click*/
ClickedSlider::ClickedSlider(QWidget * parent):QSlider(parent)
{
    setOrientation(Qt::Horizontal);
}

void ClickedSlider::mousePressEvent(QMouseEvent* e)
{
    QSlider::mousePressEvent(e);
    //qDebug()<<"x:"<<e->pos().x()<<"  y:"<<e->pos().y();
    double pos = (maximum()-minimum()) * ((double)e->pos().x()/width()) + minimum();
    //qDebug()<<"max: "<<maximum()<<" min:"<<minimum()<<" width:"<<width()<<" pos:"<<pos;
    setValue(pos);
}
