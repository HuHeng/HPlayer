#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
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
	SafeQueue<AVPacket, 25> audioPacketQ; 
	SafeQueue<AVFrame*, 10> audioFrameQ;
	/* ffmpeg struct */
	AVFormatContext* formatCtx;
	AVCodecContext* codecCtx;
	int audioIndex;
};

class AudioBuffer{
public:
private:
	SwrContext* swrCtx;
	char* data;
	int capacity;
	int size;
	int index;
	AudioBuffer():data(NULL),capacity(0),size(0),index(0){}
	~AudioBuffer(){
		if(data)
			free(data);
	}
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

