#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "safequeue.h"

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
	SafeQueue<AVPacket, 25> audioPacketQ; 
	SafeQueue<AVFrame*, 10> audioFrameQ;
	/* ffmpeg struct */
	AVFormatContext* formatCtx;
	AVCodecContext* codecCtx;
	int audioIndex;
};

class Demuxer
{
public:
	Demuxer(AudioWorks* audioWorks);
	~Demuxer();
	/*run thread, read packet and put it in the packet queue. */
	void start();
	void stop();
	/*thread function*/
	void run();
	/*init formatCtx*/
	void init();
private:
	std::thread* demux;
	AudioWorks* aw;
};
