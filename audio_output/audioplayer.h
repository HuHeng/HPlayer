#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QWidget>
#include <QSlider>

class QFile;
class QAudioOutput;
class QAudioFormat;
class QTimer;

class ClickedSlider : public QSlider
{
    Q_OBJECT
public:
    ClickedSlider(QWidget* parent = 0);
    ~ClickedSlider(){}
protected:
    virtual void mousePressEvent(QMouseEvent* e);

};


class AudioPlayer : public QWidget
{
    Q_OBJECT

public:
    AudioPlayer(QWidget* parent = 0);
    ~AudioPlayer();
    bool openAudioFile();
    void play();
    void stop();

    virtual void keyPressEvent(QKeyEvent* e);

public slots:
 //   void seek();
 //   void setVolum(int volum);
//    void suspend();
 //   void resume();
private slots:
    void writeData();

private:
    QFile *audioFile;
    QAudioOutput *audioOutput;
    QIODevice* audioDevice;

    QTimer* sendTimer;
    /*ui*/
    ClickedSlider* progressSlider;
    ClickedSlider* volumSlider;

    struct AudioWork{
        int currentPos;
        int seekbytes;
    };
};


#endif // AUDIOPLAYER_H
