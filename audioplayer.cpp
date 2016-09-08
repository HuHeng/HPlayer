#include <QAudioOutput>
#include <QAudioFormat>
#include <QDebug>

#include <QTimer>
#include <QTime>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QCommonStyle>

#include <iostream>
#include "audioplayer.h"
#include "playerui.h"

const int T_VAL = 10; //millsec
const double volumeDelta = 10;

AudioPlayer::AudioPlayer(QWidget* parent)
    : QMainWindow(parent),
    audioOutput(NULL),
    sendTimer(NULL),
    playerState(StopedState),
    serial(0)
{
    /*creat controls and setup ui*/

    //progress layout
    QGridLayout *progressLayout = new QGridLayout;
    progressSlider = new ClickedSlider;
    durationEdit = new QLineEdit;
    durationEdit->setReadOnly(true);
    connect(progressSlider, SIGNAL(valueChanged(int)), this, SLOT(setPosEdit(int)));
    progressLayout->addWidget(progressSlider,0,0);
    progressLayout->addWidget(durationEdit,0,1);
    progressLayout->setColumnStretch(0,4);
    progressLayout->setColumnStretch(1,1);

    //control layout
    openButton = new QPushButton(tr("open..."));
    connect(openButton, SIGNAL(clicked()), this, SLOT(openAudioFile()));

    stopButton = new QPushButton(tr("stop"));
    connect(stopButton, SIGNAL(clicked()), this, SLOT(closeAudioFile()));

    playButton = new QPushButton();
    playButton->setIcon(QCommonStyle().standardIcon(QStyle::SP_MediaPlay));
    connect(playButton, SIGNAL(clicked()), this, SLOT(playPause()));

    volumeSlider = new ClickedSlider;
    volumeSlider->setMinimum(0);
    volumeSlider->setMaximum(MaxVolume);
    connect(volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));

    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setMargin(0);
    controlLayout->addWidget(openButton);
    controlLayout->addWidget(stopButton);   
    controlLayout->addWidget(playButton);
    controlLayout->addWidget(volumeSlider);
    controlLayout->addStretch(1);

    //set center widget layout
    QBoxLayout *layout = new QVBoxLayout;
    videoWidget = new VideoWidget();
   // QPalette pal;
    //pal.setColor(QPalette::Background, Qt::black);
    //videoWidget->setAutoFillBackground(true);
    //videoWidget->setPalette(pal);
    layout->addWidget(videoWidget);
    layout->addLayout(progressLayout);
    layout->addLayout(controlLayout);

    QWidget* centerW = new QWidget;
    centerW->setLayout(layout);
    this->setCentralWidget(centerW);
    this->resize(640,480);
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
    //open file dialog
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Movie"),QDir::homePath());

    if (fileName.isEmpty()) {
        return false;
    }
    if(playerState != StopedState)
    {
        closeAudioFile();
    }
	/*open stream and init format Ctx and Codec Ctx*/
    //used shared_ptr
    aw = std::make_shared<AudioWorks>();

    if(aw->init(fileName.toStdString().c_str()) < 0)
        return false;
    volumeSlider->setValue(aw->volume);
    progressSlider->setMinimum(0);
    progressSlider->setMaximum(aw->formatCtx->duration/AV_TIME_BASE);
    qDebug()<<"duration: "<<aw->formatCtx->duration/AV_TIME_BASE;
    QTime duration(0,0);
    duration = duration.addSecs(aw->formatCtx->duration/AV_TIME_BASE);
    progressSlider->setValue(0);
    durationEdit->setText(QTime(0,0).toString() + "/" + duration.toString());
    qDebug()<<duration;
	//create demuxer and decoder
    demuxer = std::make_shared<Demuxer>(aw);
    audioDecoder = std::make_shared<AudioDecoder>(aw);
    videoDecoder = std::make_shared<VideoDecoder>(aw);
	//set QAudioFormat
	openAudioOutput();

    demuxer->start();
    audioDecoder->start();
    videoDecoder->start();
	//start event loop
	sendTimer = new QTimer();
    connect(sendTimer, SIGNAL(timeout()), this, SLOT(eventLoop()));
    sendTimer->start(T_VAL);
    playerState = PlayingState;
}

void AudioPlayer::closeAudioFile()
{
    if(playerState == StopedState)
        return;
    playerState = StopedState;
    aw->abortRequest = 1;
    aw->audioFrameQ.abort();
    aw->audioPacketQ.abort();
    aw->videoFrameQ.abort();
    aw->videoPacketQ.abort();

    demuxer->join();
    audioDecoder->join();
    videoDecoder->join();
    if(sendTimer)
        sendTimer->stop();
    progressSlider->setValue(0);
}

void AudioPlayer::openAudioOutput()
{

    QAudioFormat audioFormat;
    audioFormat.setSampleRate(aw->audioCodecCtx->sample_rate);
    audioFormat.setChannelCount(aw->audioCodecCtx->channels);
    audioFormat.setCodec("audio/pcm");
	/* the system endian*/
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
	/*the audio format*/
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    audioFormat.setSampleSize(16);

    audioOutput = new AudioOutput(audioFormat, aw);
    audioOutput->init();

}

/*
 * eventLoop about 10ms per time
 * write audio data and display the image with appropriate pts
*/
void AudioPlayer::eventLoop()
{    
    /*finished*/
    if(aw->eof && aw->audioFrameQ.size() == 0){
        closeAudioFile();
        return;
    }
    //write may not be blocked
    if(playerState == PauseingState)
        return;
    /*set progress value*/
    qint64 t = audioOutput->currentSerialPlayedUsec();
    //qDebug()<<"t: "<<t;
    aw->pos = aw->basePos + t;
    progressSlider->setValue(aw->pos/1000000);
    audioOutput->write();
    /*refresh video*/
    refreshVideo(aw->pos);

}

void AudioPlayer::refreshVideo(qint64 pos)
{
    for(;;){
        if(aw->videoFrameQ.size() == 0)
            return;
        std::shared_ptr<Frame> sharedFrame = aw->videoFrameQ.front();
        /*test the frame in pos*/
        qint64 startPos = sharedFrame->frame->pkt_pts - aw->formatCtx->streams[aw->videoIndex]->start_time;
        AVRational  timeBase = aw->formatCtx->streams[aw->videoIndex]->time_base;
        double spos = (double)(timeBase.num) * startPos / timeBase.den;
        double duration = (double)(timeBase.num) * sharedFrame->frame->pkt_duration / timeBase.den;
        double cpos = double(pos)/AV_TIME_BASE;
        if(spos <= cpos && cpos < spos + duration){

            aw->videoFrameQ.pop(sharedFrame);
            AVFrame* fr = sharedFrame->frame;
            SwsContext *convertCtx = sws_getContext(aw->videoCodecCtx->width,aw->videoCodecCtx->height
                                                                    ,aw->videoCodecCtx->pix_fmt,aw->videoCodecCtx->width,aw->videoCodecCtx->height
                                                                    ,AV_PIX_FMT_RGB32,SWS_BICUBIC,NULL,NULL,NULL);
            std::shared_ptr<Frame> frameRGB = std::make_shared<Frame>();
            AVFrame* pFrameRGB = frameRGB->frame;
            uint8_t *out_buffer;
            //分配AVFrame所需内存
            out_buffer = new uint8_t[avpicture_get_size(AV_PIX_FMT_RGB32, aw->videoCodecCtx->width, aw->videoCodecCtx->height)];
            avpicture_fill((AVPicture *)pFrameRGB, out_buffer, AV_PIX_FMT_RGB32, aw->videoCodecCtx->width, aw->videoCodecCtx->height);
            sws_scale(convertCtx,(const uint8_t*  const*)fr->data,fr->linesize,0
                      ,aw->videoCodecCtx->height,pFrameRGB->data,pFrameRGB->linesize);
            QImage img((uchar *)pFrameRGB->data[0],aw->videoCodecCtx->width,aw->videoCodecCtx->height,QImage::Format_RGB32);

            videoWidget->display(QPixmap::fromImage(img));
            sws_freeContext(convertCtx);
            delete[] out_buffer;
        } else if(cpos < spos){
            return;
        } else if(cpos >= spos){
            aw->videoFrameQ.pop(sharedFrame);
        }
    }

}

void AudioPlayer::closeEvent(QCloseEvent* event)
{
    closeAudioFile();
    event->accept();
}

void AudioPlayer::setPosEdit(int pos)
{
    QTime t(0,0);
    QTime tNow = t.addSecs(pos);
    QTime tDuration = t.addSecs(progressSlider->maximum());
    durationEdit->setText(tNow.toString() + "/" + tDuration.toString());
    //compare pos and aw->pos, decide seek or not
    int delta = pos - aw->pos/1000000;
    if(delta <= -2 || delta >= 2)
        seek(pos);
}

void AudioPlayer::setVolume(int volume)
{
    if(playerState == StopedState)
        return;
    aw->volume = volume;
    audioOutput->setVolume((qreal)volume/MaxVolume);
    volumeSlider->setValue(volume);
}

void AudioPlayer::playPause()
{
    if(playerState == PlayingState){
        pause();
    } else if(playerState == PauseingState){
        resume();
    }

}

void AudioPlayer::pause()
{
    playButton->setIcon(QCommonStyle().standardIcon(QStyle::SP_MediaPlay));
    playerState = PauseingState;
}

void AudioPlayer::resume()
{
    playButton->setIcon(QCommonStyle().standardIcon(QStyle::SP_MediaPause));
    playerState = PlayingState;
}

/*
 * stream seek in two cases
 * 1 click the progress slider, valuechange signal will emit
 * 2 press left or right key
*/
void AudioPlayer::seek(int pos)
{
    //aw->basePos = pos;
    aw->seekPos = pos * AV_TIME_BASE;
}

void AudioPlayer::keyPressEvent(QKeyEvent *e)
{
    /*
     * audio volume and audio state should be stored in a context
       thus prevented unnecessay getting value;
    */
    switch(e->key()){
    case Qt::Key_Down:
        aw->volume -= volumeDelta;
        if(aw->volume < 0)
            aw->volume = 0;
        setVolume(aw->volume);
        qDebug()<<"set volume:"<<aw->volume;
        break;
    case Qt::Key_Up:
        aw->volume += volumeDelta;
        if(aw->volume > MaxVolume)
            aw->volume = MaxVolume;
        setVolume(aw->volume);
        qDebug()<<"set volume:"<<aw->volume;
        break;
    case Qt::Key_Space:
        playPause();
        break;
	case Qt::Key_Q:
		/*quit*/
        closeAudioFile();
		break;
    }
}
