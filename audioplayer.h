#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QMainWindow>
#include <QSlider>
#include "audioworks.h"

class QFile;
class QAudioOutput;
class QAudioFormat;
class QTimer;
class ClickedSlider;
class QLineEdit;
class QWidget;
class QPushButton;

class AudioPlayer : public QMainWindow
{
    Q_OBJECT

public:
    enum State{
        StopedState,
        PlayingState,
        PauseingState
    };
    enum {
        MaxVolume = 100
    };
    AudioPlayer(QWidget* parent = 0);
    ~AudioPlayer();   
    void stop();

    virtual void keyPressEvent(QKeyEvent* e);
    virtual void closeEvent(QCloseEvent* e);
	void openAudioOutput();

public slots:
    //init audioworks, start demuxer and decoder thread
    bool openAudioFile();

    //stop demux and decode threads, destroy audioworks
    void closeAudioFile();

 //   void seek();
    void setVolume(int volume);
    void playPause();
    void pause();
    void resume();
//    void suspend();
 //   void resume();

private slots:
	void eventLoop();

private:
	/*audio play device*/
    AudioOutput *audioOutput;
    std::shared_ptr<AudioDecoder> audioDecoder;
    std::shared_ptr<Demuxer> demuxer;

    QTimer* sendTimer;
	
    std::shared_ptr<AudioWorks> aw;
    State playerState;

    /*ui*/
    ClickedSlider* progressSlider;
    QLineEdit* durationEdit;
    ClickedSlider* volumeSlider;
    QPushButton* openButton;
    QPushButton* stopButton;
    QPushButton* playButton;
    QWidget* videoWidget;
};

#endif // AUDIOPLAYER_H
