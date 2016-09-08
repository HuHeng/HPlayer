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
class VideoWidget;

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

    //init audioworks
	void openAudioOutput();
    void refreshVideo(qint64 pos);

public slots:
    //init audioworks, start demuxer and decoder thread
    bool openAudioFile();

    //stop demux and decode threads, destroy audioworks
    void closeAudioFile();

    void setPosEdit(int);

    void setVolume(int volume);
    void playPause();
    void pause();
    void resume();
    void seek(int pos);

private slots:
	void eventLoop();

private:
	/*audio play device*/
    AudioOutput *audioOutput;
    std::shared_ptr<AudioDecoder> audioDecoder;
    std::shared_ptr<VideoDecoder> videoDecoder;
    std::shared_ptr<Demuxer> demuxer;

    QTimer* sendTimer;
	
    std::shared_ptr<AudioWorks> aw;
    State playerState;
    int serial;
    /*ui and control*/
    ClickedSlider* progressSlider;
    QLineEdit* durationEdit;
    ClickedSlider* volumeSlider;
    QPushButton* openButton;
    QPushButton* stopButton;
    QPushButton* playButton;
    VideoWidget* videoWidget;
};

#endif // AUDIOPLAYER_H
