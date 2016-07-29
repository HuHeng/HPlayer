#include <iostream>
#include <thread>
#include <QDebug>
#include "audioworks.h"

AudioWorks::AudioWorks():
	volume(1.0),
	pos(0),
	filename(NULL),
	eof(false),
	formatCtx(NULL),
	codecCtx(NULL)
{

}

AudioWorks::~AudioWorks()
{
	//todo: delete object and free memory
}

int AudioWorks::openStream(char* filename)
{
	int ret = 0;
	
	/*open formatctx*/
	ret = avformat_open_input(&formatCtx, filename, NULL, NULL);
	if(ret < 0){
		std::cout<<"avformat open failed!"<<std::endl;
		return ret;
	}

	ret = avformat_find_stream_info(formatCtx, NULL); 
	if(ret < 0){
		std::cout<<"find stream info failed!"<<std::endl;	
		return ret;
	}
	
	/*find audio stream index*/
	int audioStreamIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0); 
	if(audioStreamIndex < 0){
		std::cout<<"can't find audio stream"<<std::endl;	
		return audioStreamIndex;
	}
	audioIndex = audioStreamIndex;
	/*alloc codecctx and open it*/
	codecCtx = avcodec_alloc_context3(NULL);	
	if(codecCtx == NULL){
		std::cout<<"alloc AVCodecContext failed!"<<std::endl;
		return -1;
	}
	ret = avcodec_parameters_to_context(codecCtx, formatCtx->streams[audioStreamIndex]->codecpar);
	/*find the decode type*/
	AVCodec* codec = avcodec_find_decoder(codecCtx->codec_id);
	if(codec == NULL){
		std::cout<<"find decoder failed!"<<std::endl;
		return -1;
	}
	ret = avcodec_open2(codecCtx, codec, NULL);
	if(ret < 0){
		std::cout<<"open codecCtx failed!"<<std::endl;
		return -1;
	}
	/*open formatCtx and codecCtx success*/
	return 0;
}


Demuxer::Demuxer(AudioWorks* audioWorks):demux(NULL), aw(audioWorks)
{
	
}
Demuxer::~Demuxer()
{
	if(demux)	
		delete demux;
}
void Demuxer::start()
{
	demux = new std::thread(std::mem_fn(&Demuxer::run), this);
}

void Demuxer::run()
{
	for(;;)
	{
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.size = 0;
		pkt.data = NULL;
		int ret = av_read_frame(aw->formatCtx, &pkt);
		//end of stream, put null packet flush the remaining frames
		if(ret < 0){
			if((ret == AVERROR_EOF || avio_feof(aw->formatCtx->pb)) && !aw->eof){
				pkt.size = 0;
				pkt.data = NULL;
				aw->audioPacketQ.push(pkt);
				aw->eof = true;
			}
		}
		/*push the audio packet*/
		if(pkt.stream_index == aw->audioIndex){
			aw->audioPacketQ.push(pkt);	
		} else{
			av_packet_unref(&pkt);
		}
	}
}

/* what does a decoder do? */
class Decoder 
{
public:
	virtual void run() = 0;
private:
	AVCodecContext* codecCtx;
};


int main()
{
}
