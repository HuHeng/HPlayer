extern "C"{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
}
#include "safequeue.h"

class QIODevice;

class AudioWorks
{
public:
	AudioWorks();
	virtual ~AudioWorks();
	void init();
	int openStream(char* filename);
	/*-----------------------------*/
	/* this class may store status */
	/* status */
	/* range: 0-1.0 */
	double volume;
	double pos;
	char* filename;
	bool eof; /*end of file*/
	bool abortRequest;
    SafeQueue<AVPacket, 100> audioPacketQ;
    SafeQueue<AVFrame*, 100> audioFrameQ;
	/* ffmpeg struct */
	AVFormatContext* formatCtx;
	AVCodecContext* codecCtx;

	int audioIndex;
};


class ThreadObj
{
public:
	virtual void run() = 0;
	void start(){
		pThread = new std::thread(std::mem_fn(&ThreadObj::run), this);
	}
	void join(){
		pThread->join();
		delete pThread;
	}
private:
	std::thread* pThread;
};

class Demuxer: public ThreadObj
{
public:
	Demuxer(AudioWorks* audioWorks);
	~Demuxer();
	/*thread function*/
	virtual void run();
private:
	AudioWorks* aw;
};

class AudioDecoder: public ThreadObj
{
public:
    AudioDecoder(AudioWorks* audioWorks);
	~AudioDecoder();
	virtual void run();
private:
	AudioWorks* aw;
};

class AudioBuffer{
public:
    AudioBuffer(AudioWorks* AudioWorks);
    ~AudioBuffer();
	void readAVFrame(AVFrame* frame);
    void writeData(QIODevice* audioDevice, int len);
    int getSize();
    uint8_t* data;
private:
  //  QIODevice* audioOutput;
    AudioWorks* aw;
	SwrContext* swrCtx;

	int capacity;
	int size;
	int index;
};


