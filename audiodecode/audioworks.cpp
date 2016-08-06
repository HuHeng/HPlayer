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
}

int AudioWorks::openStream(char* filename)
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
	ret = avcodec_open2(codecCtx, codec, NULL);
	if(ret < 0){
		std::cout<<"open codecCtx failed!"<<std::endl;
		return -1;
	}
	return 0;
}


Demuxer::Demuxer(AudioWorks* audioWorks): aw(audioWorks)
{
	
}
Demuxer::~Demuxer()
{
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
		if((ret == AVERROR_EOF || avio_feof(aw->formatCtx->pb)) && !aw->eof){
			pkt.size = 0;
			pkt.data = NULL;
			aw->audioPacketQ.push(pkt);
			aw->eof = true;
			//FIXME: maybe stop
			break;
		}
		/*push the audio packet*/
		if(pkt.stream_index == aw->audioIndex){
			aw->audioPacketQ.push(pkt);	
		} else{
			av_packet_unref(&pkt);
		}
	}
}

AudioDecoder::AudioDecoder(AudioWorks* audioWorks):aw(audioWorks)
{

}

void AudioDecoder::run()
{
	AVFrame* frame = av_frame_alloc();
	//get a pkt and decode it
	for(;;){
		AVPacket pkt;
		aw->audioPacketQ.pop(pkt);
		int flush = 0;
		//flush the left frame
		if(pkt.size == 0 && pkt.data == NULL)
			flush = 1;
		
		//get a frame and push
		int gotFrame;
		do{
			int ret = avcodec_decode_audio4(aw->codecCtx, frame, &gotFrame, &pkt);
			if(ret < 0){
				std::cout<<"decode audio error!"<<std::endl;	
				break;
			}
			if(gotFrame){
				AVFrame* qFrame = av_frame_alloc();
				av_frame_move_ref(qFrame, frame);
				aw->audioFrameQ.push(qFrame);
			}
			pkt.data += ret;
			pkt.size -= ret;
		} while(pkt.size > 0 || (gotFrame && flush));
	}
	av_frame_unref(frame);
}


AudioBuffer::AudioBuffer(AudioWorks* AudioWorks):data(NULL),capacity(0),size(0),index(0),aw(AudioWorks),swrCtx(NULL)
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
   int out_count = (int64_t)(frame->nb_samples) *4* aw->codecCtx->sample_rate / frame->sample_rate + 256;
    int out_size  = av_samples_get_buffer_size(NULL, frame->channels, out_count, AV_SAMPLE_FMT_S16, 0);
    int len2;
    if (out_size < 0) {
        std::cout<<"av_samples_get_buffer_size() failed\n";
        return;
    }
    //should return if realloc failed


    len2 = swr_convert(swrCtx, out, out_count, in, frame->nb_samples);
    std::cout<<len2<<"\n";
    if (len2 < 0) {
        std::cout<<"swr_convert() failed\n";
        return;
    }
    index = 0;
    size = len2*2;
    /* if a frame has been decoded, output it */
                FILE* outfile = fopen("/home/huheng/test.pcm", "ab+");
                int data_size = av_get_bytes_per_sample((AVSampleFormat)frame->format);
                if (data_size < 0) {

                    fprintf(stderr, "Failed to calculate data size\n");
                    exit(1);
                }
                //for (int i=0; i<frame->nb_samples; i++)
                fwrite(frame->extended_data[0], 1, 2*frame->nb_samples, outfile);
                fclose(outfile);
/*
    for(int i=0; i < frame->nb_samples; ++i){
        memcpy(data+i*4, in[0] + i*2, 2);
        memcpy(data+i*4+2, in[1] + i*2, 2);
    }
    index = 0;
    size = 4*frame->nb_samples;
*/
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
