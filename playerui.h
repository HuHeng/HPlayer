#ifndef PLAYERUI_H
#define PLAYERUI_H
#include <QSlider>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>

class ClickedSlider : public QSlider
{
    Q_OBJECT
public:
    ClickedSlider(QWidget* parent = 0);
    ~ClickedSlider(){}

protected:
    virtual void mousePressEvent(QMouseEvent* e);

};

class AspectRatioPixmapLabel : public QLabel
{
    Q_OBJECT
public:
    explicit AspectRatioPixmapLabel(QWidget *parent = 0);
    virtual int heightForWidth( int width ) const;
    virtual QSize sizeHint() const;
    QPixmap scaledPixmap() const;
public slots:
    void setPixmap ( const QPixmap & );
    void resizeEvent(QResizeEvent *);
private:
    QPixmap pix;
};

class VideoWidget : public QWidget
{
public:
    VideoWidget(QWidget* parent = 0);
    void display(const QPixmap& );
private:
    AspectRatioPixmapLabel* piclabel;
};

#endif // PLAYERUI_H
