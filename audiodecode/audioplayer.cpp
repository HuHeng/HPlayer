#include <QAudioOutput>
#include <QAudioFormat>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QMouseEvent>

#include "audioplayer.h"

const int T_VAL = 20; //millsec
const double volumeDelta = 0.1;

/*a slider can response to click*/
ClickedSlider::ClickedSlider(QWidget * parent):QSlider(parent)
{
    setOrientation(Qt::Horizontal);
}

void ClickedSlider::mousePressEvent(QMouseEvent* e)
{
    QSlider::mousePressEvent(e);
    qDebug()<<"x:"<<e->pos().x()<<"  y:"<<e->pos().y();
    double pos = (maximum()-minimum()) * ((double)e->pos().x()/width()) + minimum();
    qDebug()<<"max: "<<maximum()<<" min:"<<minimum()<<" width:"<<width()<<" pos:"<<pos;
    setValue(pos);
}

AudioPlayer::AudioPlayer(QWidget* parent)
    : QWidget(parent),
    audioFile(NULL),
    audioOutput(NULL),
    sendTimer(NULL)
{
    /*creat controls and setup ui*/
  //  audioOutput = new QAudioOutput;
}

AudioPlayer::~AudioPlayer()
{
    if(audioOutput)
        audioOutput->stop();
    if(audioFile)
        delete audioFile;
    if(audioOutput)
        delete audioOutput;
    if(sendTimer)
        delete sendTimer;

}

bool AudioPlayer::openAudioFile()
{
    audioFile = new QFile("/home/huheng/andy.pcm");
    bool ret = audioFile->open(QIODevice::ReadOnly);
    if(!ret){
        qDebug("open file: %s, falied!", qPrintable(audioFile->fileName()));
        return ret;
    }
    audioFile->seek(0);
    return ret;
}

/*
open stream init, create demuxer and decoder, init them
start the thread, doplay, go to eventloop
*/

void AudioPlayer::play()
{
    QAudioFormat audioFormat;
    audioFormat.setSampleRate(44100);
    audioFormat.setChannelCount(2);
    audioFormat.setSampleSize(16);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setSampleType(QAudioFormat::SignedInt);

    audioOutput = new QAudioOutput(audioFormat, 0);
    audioDevice = audioOutput->start();

    /*write data to audio device when the timer emit timeout*/
    sendTimer = new QTimer();
    connect(sendTimer, SIGNAL(timeout()), this, SLOT(writeData()));
    sendTimer->start(T_VAL);
}

void AudioPlayer::writeData()
{
    //48000*2*2/1000*30
    int freeBytes = audioOutput->bytesFree();
 //   qDebug()<<"freeBytes:"<<freeBytes;
    if(freeBytes <= 0)
        return;
    //send freeBytes to io device
    //FIXME: should alloc a iocontext, then the buffer alloc once
    char* buffer = (char*)malloc(freeBytes);
    qint64 readbytes = audioFile->read(buffer, freeBytes);
    if(readbytes <= 0){
        //the end
        audioFile->close();
        free(buffer);
        sendTimer->stop();
     //   audioOutput->stop();
        return;
    }
//    qDebug()<<"offset"<<freeBytes;
//    qDebug()<<"pos"<<audioFile->pos();
    audioDevice->write(buffer, readbytes);
    free(buffer);
}

void AudioPlayer::keyPressEvent(QKeyEvent *e)
{
    /*
     * audio volume and audio state should be stored in a context
       thus prevented unnecessay getting value;
    */
    qreal v;
    bool pause;
    switch(e->key()){
    case Qt::Key_Down:
        v = audioOutput->volume();
        qDebug()<<"current volume:"<<v;
        v -= volumeDelta;
        v = v > 0 ? v : 0;
        audioOutput->setVolume(v);
        qDebug()<<"set volume:"<<v;
        break;
    case Qt::Key_Up:
        v = audioOutput->volume();
        qDebug()<<"current volume:"<<v;
        v += volumeDelta;
        v = v > 1 ? 1 : v;
        audioOutput->setVolume(v);
        qDebug()<<"set volume:"<<v;
        break;
    case Qt::Key_Space:
        pause = audioOutput->state() == QAudio::SuspendedState ? true : false;
        if(pause)
            audioOutput->resume();
        else
            audioOutput->suspend();
        break;
    }
}
