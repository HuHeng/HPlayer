#include "playerui.h"
#include <QMouseEvent>
#include <QtWidgets>

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

AspectRatioPixmapLabel::AspectRatioPixmapLabel(QWidget *parent) :
    QLabel(parent)
{
    this->setMinimumSize(1,1);
    setScaledContents(false);
}

void AspectRatioPixmapLabel::setPixmap ( const QPixmap & p)
{
    pix = p;
    QLabel::setPixmap(scaledPixmap());
}

int AspectRatioPixmapLabel::heightForWidth( int width ) const
{
    return pix.isNull() ? this->height() : ((qreal)pix.height()*width)/pix.width();
}

QSize AspectRatioPixmapLabel::sizeHint() const
{
    int w = this->width();
   // int h = this->height();
    /*
    if(pix.isNull())
        return QSize(w,h);
    int pw = pix.width();
    int ph = pix.height();
    if((qreal)w/h > (qreal)pw/ph)
        return QSize(pw/ph*h ,h);
        */
    return QSize( w, heightForWidth(w) );
}

QPixmap AspectRatioPixmapLabel::scaledPixmap() const
{
    return pix.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void AspectRatioPixmapLabel::resizeEvent(QResizeEvent * e)
{
    if(!pix.isNull())
        QLabel::setPixmap(scaledPixmap());
}

VideoWidget::VideoWidget(QWidget* parent): QWidget(parent)
{
    piclabel = new AspectRatioPixmapLabel(parent);
    piclabel->setAlignment(Qt::AlignCenter);
    QPalette pal;
    pal.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);
}

void VideoWidget::display(const QPixmap & pix)
{
    piclabel->setPixmap(pix);
    QBoxLayout* layout = new QHBoxLayout;
    this->setLayout(layout);
    layout->addWidget(piclabel);
}

