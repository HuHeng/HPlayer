#ifndef AUDIOWORKS_H
#define AUDIOWORKS_H

extern "C"{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}
#include "safequeue.h"
#include <memory>
#include <QAudioOutput>
#include <QAudioFormat>

class QIODevice;

/*wrap AVPacket*/
struct Packet
{
    AVPacket pkt;
    int serial;
    Packet():serial(0){
        av_init_packet(&pkt);
    }
    ~Packet(){
        av_packet_unref(&pkt);
    }
};

/*wrap AVFrame*/
struct Frame
{
    AVFrame* frame;
    int serial;
    Frame():serial(0){
        frame = av_frame_alloc();
    }
    ~Frame(){
        av_frame_unref(frame);
        av_frame_free(&frame);
    }
};



class AudioWorks
{
public:
	AudioWorks();
	virtual ~AudioWorks();
    int init(const char*);
	/*-----------------------------*/
	/* this class may store status */
    /* range: 0-100 corresponding to the volumeSlider value*/
    int volume;
	char* filename;

    int serial;/*serial plus 1 after stream seek*/
    bool pause;
	bool eof; /*end of file*/
	bool abortRequest;

    /*clock and pos issue*/
    qint64 pos;
    qint64 basePos;/*base pos of the current serial*/
    qint64 seekPos;
    qint64 bypastSerialsProcessUsec;

    SafeQueue<std::shared_ptr<Packet>, 100> audioPacketQ;
    SafeQueue<std::shared_ptr<Frame>, 30> audioFrameQ;
    SafeQueue<std::shared_ptr<Packet>, 30> videoPacketQ;
    SafeQueue<std::shared_ptr<Frame>, 10> videoFrameQ;

    /* ffmpeg struct */
	AVFormatContext* formatCtx;
    AVCodecContext* audioCodecCtx;
    AVCodecContext* videoCodecCtx;

	int audioIndex;
    int videoIndex;
};


class ThreadObj
{
public:
    ThreadObj() : pThread(NULL){}
    ~ThreadObj(){
    }
	virtual void run() = 0;
	void start(){
        pThread = std::make_shared<std::thread>(std::mem_fn(&ThreadObj::run), this);
	}
	void join(){
        if(pThread->joinable())
            pThread->join();
	}
private:
    std::shared_ptr<std::thread> pThread;
};

class Demuxer: public ThreadObj
{
public:
    Demuxer(std::shared_ptr<AudioWorks> audioWorks);
	~Demuxer();
	/*thread function*/
	virtual void run();
private:
    std::shared_ptr<AudioWorks> aw;
};

class AudioDecoder: public ThreadObj
{
public:
    AudioDecoder(std::shared_ptr<AudioWorks> audioWorks);
	~AudioDecoder();
	virtual void run();
private:
    std::shared_ptr<AudioWorks> aw;
};

class VideoDecoder: public ThreadObj
{
public:
    VideoDecoder(std::shared_ptr<AudioWorks> audioWorks);
    ~VideoDecoder();
    virtual void run();
private:
    std::shared_ptr<AudioWorks> aw;
};

class AudioOutput : public QAudioOutput
{
public:
    AudioOutput(QAudioFormat audioFormat, std::shared_ptr<AudioWorks> audioWorks);
    virtual ~AudioOutput();
    void init();
    //read a avframe to data buffer
    void readAVFrame(AVFrame* frame);
    //write len of audio data to audio device
    void write();
    void writeData(int len);
    qint64 currentSerialPlayedUsec();
private:
    QIODevice* audioDevice;
    std::shared_ptr<AudioWorks> aw;
    SwrContext* swrCtx;

    int serial;
    qint64 bypastSerialsProcessedUsec;

    //data buffer
    uint8_t* data;
    int capacity;
    int size;
    int index;
};

#endif
