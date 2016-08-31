#include <iostream>
#include <thread>
#include <QIODevice>
#include "audioworks.h"
#include <string.h>



AudioWorks::AudioWorks():
    volume(80),
	pos(0),
    lastPos(0),
    bypastSerialsProcessUsec(0),
	filename(NULL),
	eof(false),
	abortRequest(false),
	formatCtx(NULL),
    audioCodecCtx(NULL)
{

}

AudioWorks::~AudioWorks()
{
	//todo: delete object and free memory
    avcodec_free_context(&audioCodecCtx);
    avcodec_free_context(&videoCodecCtx);
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
    /*alloc audioCodecCtx and open it*/
    audioCodecCtx = avcodec_alloc_context3(NULL);
    if(audioCodecCtx == NULL){
		std::cout<<"alloc AVCodecContext failed!"<<std::endl;
		return -1;
	}
    ret = avcodec_parameters_to_context(audioCodecCtx, formatCtx->streams[audioStreamIndex]->codecpar);
	/*find the decode type*/
    AVCodec* codec = avcodec_find_decoder(audioCodecCtx->codec_id);
	if(codec == NULL){
		std::cout<<"find decoder failed!"<<std::endl;
		return -1;
	}
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "refcounted_frames", "1", 0);

    ret = avcodec_open2(audioCodecCtx, codec, &opts);
	if(ret < 0){
        std::cout<<"open audioCodecCtx failed!"<<std::endl;
		return -1;
	}

    /*find video stream index*/
    int videoStreamIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(videoStreamIndex < 0){
        std::cout<<"can't find video stream"<<std::endl;
        return videoStreamIndex;
    }
    videoIndex = videoStreamIndex;
    /*alloc audioCodecCtx and open it*/
    videoCodecCtx = avcodec_alloc_context3(NULL);
    if(videoCodecCtx == NULL){
        std::cout<<"alloc AVCodecContext failed!"<<std::endl;
        return -1;
    }
    ret = avcodec_parameters_to_context(videoCodecCtx, formatCtx->streams[videoStreamIndex]->codecpar);
    /*find the decode type*/
    AVCodec* vcodec = avcodec_find_decoder(videoCodecCtx->codec_id);
    if(vcodec == NULL){
        std::cout<<"find decoder failed!"<<std::endl;
        return -1;
    }
    AVDictionary *vopts = NULL;
    av_dict_set(&vopts, "refcounted_frames", "1", 0);

    ret = avcodec_open2(videoCodecCtx, vcodec, &vopts);
    if(ret < 0){
        std::cout<<"open videoCodecCtx failed!"<<std::endl;
        return -1;
    }
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
            aw->videoPacketQ.push(sharedPkt);
			aw->eof = true;
			//FIXME: maybe stop
			break;
		}
		/*push the audio packet*/
        if(ppkt->stream_index == aw->audioIndex){
            aw->audioPacketQ.push(sharedPkt);
        } else if(ppkt->stream_index == aw->videoIndex){
            aw->videoPacketQ.push(sharedPkt);
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
            int ret = avcodec_decode_audio4(aw->audioCodecCtx, frame, &gotFrame, ppkt);
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

VideoDecoder::VideoDecoder(std::shared_ptr<AudioWorks> audioWorks):aw(audioWorks)
{

}
VideoDecoder::~VideoDecoder()
{

}

void VideoDecoder::run()
{
    auto sharedFrame = std::make_shared<Frame>();
    for(;;){
        if(aw->abortRequest == 1)
            break;
        std::shared_ptr<Packet> sharedPacket;
        aw->videoPacketQ.pop(sharedPacket);
        AVPacket* ppkt = &sharedPacket->pkt;
        int flush = 0;
        //flush the left frame
        if(ppkt->size == 0 && ppkt->data == NULL)
            flush = 1;

        //get a frame and push
        int gotFrame = 1;
        AVFrame* frame = sharedFrame->frame;
        do{
            int ret = avcodec_decode_video2(aw->audioCodecCtx, frame, &gotFrame, ppkt);
            if(ret < 0){
                std::cout<<"decode video error!"<<std::endl;
                break;
            }
            if(gotFrame){
                //AVFrame* qFrame = av_frame_alloc();
                auto qFrame = std::make_shared<Frame>();
                av_frame_move_ref(qFrame->frame, frame);
                aw->videoFrameQ.push(qFrame);
            }
            ppkt->data += ret;
            ppkt->size -= ret;
        } while((ppkt->size > 0 || (gotFrame && flush)) && aw->abortRequest != 1);
    }

}


AudioOutput::AudioOutput(QAudioFormat audioFormat, std::shared_ptr<AudioWorks> audioWorks) :
    QAudioOutput(audioFormat),
    data(NULL),capacity(0),size(0),index(0),
    aw(audioWorks),
    swrCtx(NULL),
    audioDevice(NULL)
{

}

AudioOutput::~AudioOutput(){
    if(swrCtx)
        swr_free(&swrCtx);
    if(data)
        free(data);
}

void AudioOutput::init()
{
    //qaudiooutput->start
    audioDevice = start();
}


void AudioOutput::readAVFrame(AVFrame* frame)
{
    if(!swrCtx){
        swrCtx = swr_alloc_set_opts(NULL,
                                    aw->audioCodecCtx->channel_layout, AV_SAMPLE_FMT_S16, aw->audioCodecCtx->sample_rate,
                                    frame->channel_layout, (AVSampleFormat)frame->format, frame->sample_rate,
                                    0, NULL);
        swr_init(swrCtx);
    }
    const uint8_t **in = (const uint8_t **)frame->extended_data;
    uint8_t **out = &data;
    int bufLen = aw->audioCodecCtx->channels * frame->nb_samples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    if(capacity < bufLen){
        data = (uint8_t*)realloc(data, bufLen);
        capacity = bufLen;
    }

   int out_count = (int64_t)(frame->nb_samples) *4* aw->audioCodecCtx->sample_rate / frame->sample_rate + 256;
   /*
   int out_size  = av_samples_get_buffer_size(NULL, frame->channels, out_count, AV_SAMPLE_FMT_S16, 0);
    int len2;
    if (out_size < 0) {
        std::cout<<"av_samples_get_buffer_size() failed\n";
        return;
    }
    */

    int len2 = swr_convert(swrCtx, out, out_count, in, frame->nb_samples);
    //std::cout<<len2<<"\n";
    if (len2 < 0) {
        std::cout<<"swr_convert() failed\n";
        return;
    }
    index = 0;
    size = aw->audioCodecCtx->channels * len2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

}

void AudioOutput::writeData(int len)
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

void AudioOutput::write()
{
    int freeBytes = bytesFree();
    if(freeBytes <= 0)
        return;
    /*first, send the remain data of audio buffer*/
    int remain = size;
    //write remain data
    if(remain < freeBytes && remain > 0){
        writeData(remain);
        freeBytes -= remain;
    }
    /*second, convert the avframe to the audio buffer and send to the device*/
    while(freeBytes > 0 && aw->audioFrameQ.size() > 0){
        //convert AVframe data to audio buffer
        std::shared_ptr<Frame> sharedFrame;
        aw->audioFrameQ.pop(sharedFrame);
        readAVFrame(sharedFrame->frame);
        if(freeBytes <= size){
            writeData(freeBytes);
            freeBytes = 0;
        } else{
            int len = size;
            writeData(len);
            freeBytes -= len;
        }
    }
}
