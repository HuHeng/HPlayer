#ifndef AUDIOWORKS_H
#define AUDIOWORKS_H

extern "C"{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
}
#include "safequeue.h"
#include <memory>

class QIODevice;

/*wrap AVPacket*/
struct Packet
{
    AVPacket pkt;
    Packet(){
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
    Frame(){
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
    int init(char*);
	/*-----------------------------*/
	/* this class may store status */
	/* status */
	/* range: 0-1.0 */
	double volume;
	double pos;
	char* filename;
	bool eof; /*end of file*/
	bool abortRequest;
    SafeQueue<std::shared_ptr<Packet>, 100> audioPacketQ;
    SafeQueue<std::shared_ptr<Frame>, 100> audioFrameQ;
	/* ffmpeg struct */
	AVFormatContext* formatCtx;
	AVCodecContext* codecCtx;

	int audioIndex;
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

class AudioBuffer{
public:
    AudioBuffer(std::shared_ptr<AudioWorks> AudioWorks);
    ~AudioBuffer();
	void readAVFrame(AVFrame* frame);
    void writeData(QIODevice* audioDevice, int len);
    int getSize();
    uint8_t* data;
private:
  //  QIODevice* audioOutput;
    std::shared_ptr<AudioWorks> aw;
	SwrContext* swrCtx;

	int capacity;
	int size;
	int index;
};
/*
class AudioOutput
{
public:
    //write audio data to audio device
    writeAudioData(std::shared_ptr<AudioWorks> aw);
private:
    QIODevice audioDevice;
    QAudioOutput audioOutput;
};
*/


#endif
