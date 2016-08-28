#include <QAudioOutput>
#include <QAudioFormat>
#include <QDebug>

#include <QTimer>
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
    playerState(StopedState)
{
    /*creat controls and setup ui*/

    //progress layout
    QGridLayout *progressLayout = new QGridLayout;
    progressSlider = new ClickedSlider;
    durationEdit = new QLineEdit;
    durationEdit->setReadOnly(true);

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
    videoWidget = new QWidget;
    QPalette pal;
    pal.setColor(QPalette::Background, Qt::black);
    videoWidget->setAutoFillBackground(true);
    videoWidget->setPalette(pal);
    layout->addWidget(videoWidget);
    layout->addLayout(progressLayout);
    layout->addLayout(controlLayout);

    QWidget* centerW = new QWidget;
    centerW->setLayout(layout);
    this->setCentralWidget(centerW);
    this->resize(320,240);
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
	//create demuxer and decoder
    demuxer = std::make_shared<Demuxer>(aw);
    audioDecoder = std::make_shared<AudioDecoder>(aw);

	//set QAudioFormat
	openAudioOutput();

    demuxer->start();
    audioDecoder->start();

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
    demuxer->join();
    audioDecoder->join();
    if(sendTimer)
        sendTimer->stop();
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
    //write may not be blocked
    if(playerState == PauseingState)
        return;
    audioOutput->write();

}

void AudioPlayer::closeEvent(QCloseEvent* event)
{
    closeAudioFile();
    event->accept();
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
