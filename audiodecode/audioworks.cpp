#include <iostream>
#include <thread>
#include <QIODevice>
#include "audioworks.h"
#include <stdio.h>
#include <string.h>



AudioWorks::AudioWorks():
	volume(1.0),
	pos(0),
	filename(NULL),
	eof(false),
	abortRequest(false),
	formatCtx(NULL),
	codecCtx(NULL)
{

}

AudioWorks::~AudioWorks()
{
	//todo: delete object and free memory
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);
}

int AudioWorks::init(const char* filename)
{
	int ret = 0;
    av_register_all();
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
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "refcounted_frames", "1", 0);

    ret = avcodec_open2(codecCtx, codec, &opts);
	if(ret < 0){
		std::cout<<"open codecCtx failed!"<<std::endl;
		return -1;
	}
    /*TODO:init play state*/

    return 0;
}

Demuxer::Demuxer(std::shared_ptr<AudioWorks> audioWorks): aw(audioWorks)
{
	
}
Demuxer::~Demuxer()
{
}

void Demuxer::run()
{
	for(;;)
	{
        if(aw->abortRequest == 1)
            break;
        auto sharedPkt = std::make_shared<Packet>();
        //av_init_packet(&pkt);
        AVPacket* ppkt = &sharedPkt->pkt;
        ppkt->size = 0;
        ppkt->data = NULL;
        int ret = av_read_frame(aw->formatCtx, ppkt);
		//end of stream, put null packet flush the remaining frames
		if((ret == AVERROR_EOF || avio_feof(aw->formatCtx->pb)) && !aw->eof){
            ppkt->size = 0;
            ppkt->data = NULL;
            aw->audioPacketQ.push(sharedPkt);
			aw->eof = true;
			//FIXME: maybe stop
			break;
		}
		/*push the audio packet*/
        if(ppkt->stream_index == aw->audioIndex){
            aw->audioPacketQ.push(sharedPkt);
        }
	}
}

AudioDecoder::AudioDecoder(std::shared_ptr<AudioWorks> audioWorks):aw(audioWorks)
{

}
AudioDecoder::~AudioDecoder()
{

}

void AudioDecoder::run()
{

	//get a pkt and decode it
    //AVFrame* frame = av_frame_alloc();
    auto sharedFrame = std::make_shared<Frame>();
    for(;;){
        if(aw->abortRequest == 1)
            break;
        std::shared_ptr<Packet> sharedPacket;
        aw->audioPacketQ.pop(sharedPacket);
        AVPacket* ppkt = &sharedPacket->pkt;
		int flush = 0;
		//flush the left frame
        if(ppkt->size == 0 && ppkt->data == NULL)
			flush = 1;	

		//get a frame and push
        int gotFrame = 1;
        AVFrame* frame = sharedFrame->frame;
		do{
            int ret = avcodec_decode_audio4(aw->codecCtx, frame, &gotFrame, ppkt);
			if(ret < 0){
				std::cout<<"decode audio error!"<<std::endl;	
				break;
			}
			if(gotFrame){
                //AVFrame* qFrame = av_frame_alloc();
                auto qFrame = std::make_shared<Frame>();
                av_frame_move_ref(qFrame->frame, frame);
                aw->audioFrameQ.push(qFrame);
			}
            ppkt->data += ret;
            ppkt->size -= ret;
        } while((ppkt->size > 0 || (gotFrame && flush)) && aw->abortRequest != 1);
	}
}


AudioBuffer::AudioBuffer(std::shared_ptr<AudioWorks> audioWorks):data(NULL),capacity(0),size(0),index(0),aw(audioWorks),swrCtx(NULL)
{

}

AudioBuffer::~AudioBuffer(){
    if(swrCtx)
        swr_free(&swrCtx);
    if(data)
        free(data);
}

//maybe work
void AudioBuffer::readAVFrame(AVFrame* frame)
{
        swr_free(&swrCtx);
        swrCtx = swr_alloc_set_opts(NULL,
                                aw->codecCtx->channel_layout, AV_SAMPLE_FMT_S16, aw->codecCtx->sample_rate,
                                frame->channel_layout, (AVSampleFormat)frame->format, frame->sample_rate,
                                0, NULL);
        swr_init(swrCtx);

    const uint8_t **in = (const uint8_t **)frame->extended_data;
    uint8_t **out = &data;

    if(capacity < 4*frame->nb_samples){
        data = (uint8_t*)realloc(data, 4*frame->nb_samples);
        capacity = 4*frame->nb_samples;
    }
    /*
   int out_count = (int64_t)(frame->nb_samples) *4* aw->codecCtx->sample_rate / frame->sample_rate + 256;
    int out_size  = av_samples_get_buffer_size(NULL, frame->channels, out_count, AV_SAMPLE_FMT_S16, 0);
    int len2;
    if (out_size < 0) {
        std::cout<<"av_samples_get_buffer_size() failed\n";
        return;
    }
    len2 = swr_convert(swrCtx, out, out_count, in, frame->nb_samples);
    std::cout<<len2<<"\n";
    if (len2 < 0) {
        std::cout<<"swr_convert() failed\n";
        return;
    }
    index = 0;
    size = len2*2;
    */
    /* if a frame has been decoded, output it */


    for(int i=0; i < frame->nb_samples; ++i){
        memcpy(data+i*4, in[0] + i*2, 2);
        memcpy(data+i*4+2, in[1] + i*2, 2);
    }
    index = 0;
    size = 4*frame->nb_samples;

}
int AudioBuffer::getSize()
{
    return size;
}

void AudioBuffer::writeData(QIODevice *audioDevice, int len)
{
    if(size < len){
        std::cout<<"no enough size\n";
        return;
    }
    audioDevice->write((const char*)data + index, len);
    size -= len;
    index += len;
    if(size == 0)
        index = 0;
}
