#include <QAudioOutput>
#include <QAudioFormat>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QMouseEvent>
#include <iostream>
#include "audioplayer.h"

const int T_VAL = 10; //millsec
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
    if(audioOutput)
        delete audioOutput;
    if(sendTimer)
        delete sendTimer;

}

/*
 * open stream init, create demuxer and decoder, init them
 * start the thread, doplay, go to eventloop
*/
bool AudioPlayer::openAudioFile()
{
	/*open stream and init format Ctx and Codec Ctx*/
    //used shared_ptr
    aw = std::make_shared<AudioWorks>();
	//test mp3
    if(aw->init("/home/huheng/andy.mp3") < 0)
        return false;

	//create demuxer and decoder
    demuxer = std::make_shared<Demuxer>(aw);
    audioDecoder = std::make_shared<AudioDecoder>(aw);
    audioBuffer = std::make_shared<AudioBuffer>(aw);
	//set QAudioFormat
	openAudioOutput();

    demuxer->start();
    audioDecoder->start();

	//start event loop
	sendTimer = new QTimer();
    connect(sendTimer, SIGNAL(timeout()), this, SLOT(eventLoop()));
    sendTimer->start(T_VAL);

}

void AudioPlayer::openAudioOutput()
{

    QAudioFormat audioFormat;
    audioFormat.setSampleRate(aw->codecCtx->sample_rate);
    audioFormat.setChannelCount(aw->codecCtx->channels);
    audioFormat.setCodec("audio/pcm");
	/* the system endian*/
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
	/*the audio format*/
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    audioFormat.setSampleSize(16);

    audioOutput = new QAudioOutput(audioFormat, 0);
    audioDevice = audioOutput->start();

}


void AudioPlayer::eventLoop()
{
    if(aw->abortRequest){
        demuxer->join();
        audioDecoder->join();
        sendTimer->stop();
        std::cout<<"q: "<<std::this_thread::get_id()<<std::endl;
    }
    /*about 10ms per loop*/
    int freeBytes = audioOutput->bytesFree();
	if(freeBytes <= 0)
		return;
	/*first, send the remain data of audio buffer*/
    int remain = audioBuffer->getSize();
    //write remain data
    if(remain < freeBytes && remain > 0){
        audioBuffer->writeData(audioDevice, remain);
        freeBytes -= remain;
	}
	/*second, convert the avframe to the audio buffer and send to the device*/
	while(freeBytes > 0 && aw->audioFrameQ.size() > 0){
		//convert AVframe data to audio buffer 
        std::shared_ptr<Frame> sharedFrame;
        aw->audioFrameQ.pop(sharedFrame);
        audioBuffer->readAVFrame(sharedFrame->frame);
        if(freeBytes <= audioBuffer->getSize()){
            audioBuffer->writeData(audioDevice, freeBytes);
            freeBytes = 0;
        } else{
            int len = audioBuffer->getSize();
            audioBuffer->writeData(audioDevice, len);
            freeBytes -= len;
        }
    }

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
        std::cout<<std::this_thread::get_id()<<std::endl;
        pause = audioOutput->state() == QAudio::SuspendedState ? true : false;
        if(pause)
            audioOutput->resume();
        else
            audioOutput->suspend();
        break;
	case Qt::Key_Q:
		/*quit*/
		aw->abortRequest = 1;
        aw->audioFrameQ.abort();
        aw->audioPacketQ.abort();
        demuxer->join();
        audioDecoder->join();
        sendTimer->stop();
		break;
    }
}
