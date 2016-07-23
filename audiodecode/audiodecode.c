extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
}

#include <iostream>

class AudioWorks
{
	/* this class may store status */
	/* status */
	/* range: 0-1.0 */
	double volume;
	double pos;

	std::queue<AVPacket*> audioPacketQ; 
	std::queue<AVFrame*> audioFrameQ;
	
};

class Demuxer
{
public:
	/*run thread, read packet and put it in the packet queue. */
	void start();
	/*init formatCtx*/
	void init();
private:
	std::thread::id demuxId;
	AVFormatContext* formatCtx;
};


/* what does a decoder do? */
class Decoder 
{
private:
	AVCodecContext* codecCtx;
};


int main()
{
}
