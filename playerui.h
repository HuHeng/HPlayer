#ifndef PLAYERUI_H
#define PLAYERUI_H
#include <QSlider>
class ClickedSlider : public QSlider
{
    Q_OBJECT
public:
    ClickedSlider(QWidget* parent = 0);
    ~ClickedSlider(){}

protected:
    virtual void mousePressEvent(QMouseEvent* e);

};

#endif // PLAYERUI_H
