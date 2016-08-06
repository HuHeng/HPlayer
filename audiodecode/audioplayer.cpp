#include <QAudioOutput>
#include <QAudioFormat>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QMouseEvent>

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

/*
 * open stream init, create demuxer and decoder, init them
 * start the thread, doplay, go to eventloop
*/
bool AudioPlayer::openAudioFile()
{
	/*open stream and init format Ctx and Codec Ctx*/
	aw = new AudioWorks();
	//test mp3
	aw->openStream("/home/huheng/andy.mp3");
	//create demuxer and decoder
	demuxer = new Demuxer(aw);
    audioDecoder = new AudioDecoder(aw);
    audioBuffer = new AudioBuffer(aw);
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
		AVFrame* frame;
		aw->audioFrameQ.pop(frame);
        audioBuffer->readAVFrame(frame);
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
	case Qt::Key_Q:
		/*quit*/
		aw->abortRequest = 1;
		break;
    }
}
