/********************************************************
  Copyright (C), 2006-2016, Aurine
  File name:	logic_audio_ao.c
  Author:		罗发禄
  Version:		V2.0
  Date: 		2015-03-14
  Description:	音频AO播放程序
  
	History:				 
		1.Date: 
		Author: 
		Modification: 
*********************************************************/
#include <sys/soundcard.h>

#include "logic_audio_ao.h"
#include "logic_audio.h"
#include "logic_media.h"
#include "logic_media_core.h"
#include "audio_dev.h"

#define	BLOCKTIME         		10     //每个缓冲数据块的时长（毫秒）

#define _VOICE_SCALE_			0

#define _AUDIO_AO_FILE_SAVE_	0
#if _AUDIO_AO_FILE_SAVE_
#define PCM_AO_AUDIO			0
#define ALAW_AO_AUDIO			1
#define PCM_AO_AUDIO_FILE		CFG_PUBLIC_DRIVE"/ao_spk.pcm"
#define ALAW_AO_AUDIO_FILE		CFG_PUBLIC_DRIVE"/ao.alaw"

static FILE * aoDPCMFIle = NULL;
static FILE * aoDALAWFIle = NULL;

/*************************************************
  Function:		g711_ao_file_close
  Description: 	
  Input: 		
  Output:		
  Return:		 
  Others:
*************************************************/
static void g711_ao_file_close(uint8 mode)
{
	switch (mode)
	{
		case PCM_AO_AUDIO:
			if (aoDPCMFIle)
			{
				fclose(aoDPCMFIle);
				aoDPCMFIle = NULL;
			}
			break;
			
		case ALAW_AO_AUDIO:
			if (aoDALAWFIle)
			{
				fclose(aoDALAWFIle);
				aoDALAWFIle = NULL;
			}
			break;
			
		default:
			break;
	}
}

/*************************************************
  Function:		g711_ao_file_open
  Description: 	
  Input: 		
  Output:		
  Return:		 
  Others:
*************************************************/
static void g711_ao_file_open(uint8 mode)
{
	g711_ao_file_close(mode);
	switch (mode)
	{
		case PCM_AO_AUDIO:
			aoDPCMFIle = fopen(PCM_AO_AUDIO_FILE, "wb");
			break;
			
		case ALAW_AO_AUDIO:
			aoDALAWFIle = fopen(ALAW_AO_AUDIO_FILE, "wb");
			break;
			
		default:
			break;
	}
}

/*************************************************
  Function:		g711_ao_file_save
  Description: 	
  Input: 		
  Output:		
  Return:		 
  Others:
*************************************************/
static void g711_ao_file_save(uint8 mode, uint8 *data, int len)
{
	switch (mode)
	{
		case PCM_AO_AUDIO:
			if (aoDPCMFIle != NULL)
			{
				fwrite(data, len, 1, aoDPCMFIle);
				fflush(aoDPCMFIle);
			}
			break;
			
		case ALAW_AO_AUDIO:
			if (aoDALAWFIle != NULL)
			{
				fwrite(data, len, 1, aoDALAWFIle);
				fflush(aoDALAWFIle);
			}
			break;
			
		default:
			break;
	}
}
#endif
#if (_AEC_TYPE_ == _SW_AEC_)
//by zxf
//static struct timeval timestart;
//static struct timeval timestop;
//static long usec;
void ace_echo_playback_aec( short * play);
void ace_echo_playback_aec2(short * play, int samplecount);
#endif

/*************************************************
  Function:		audio_play_start
  Description:	启动音频播放
  Input: 		
  Output:
  Return:		 
  Others:
*************************************************/
static int audio_play_start( MSMediaDesc * arg)
{
	AudioAoState * data = (AudioAoState*)arg->private;
	int  Volume = 70;//100;
	int  StreamID = 0;
	int  Channels = data->channelnum;
	int  Bitwidth = 16;
	int  SampleRate = data->Samplerate;

	return Alsa_Play_Start(Channels, Bitwidth, SampleRate, BLOCKTIME);
}

/*************************************************
  Function:		ms_ao_audio_init
  Description: 	音波AO初始化
  Input: 		
  Output:		
  Return:		 
  Others:
*************************************************/
static int ms_ao_audio_init(struct _MSMediaDesc * f)
{
	if (NULL == f->private)
	{
		AudioAoState * data = (AudioAoState*)malloc(sizeof(AudioAoState));
		
		memset(data, 0, sizeof(AudioAoState));
		data->enPayloadType= PT_G711A;
		data->Samplerate = AUDIO_SAMPLE_RATE_8000;
   		data->Bitwidth = AUDIO_BIT_WIDTH_16;       	/*standard 16bit little endian format, support this format only*/
		data->channelnum = 1;
		data->aoDev = 0;
		data->AParam.IsPack = HI_TRUE;
		data->AParam.PackNum = AUDIO_NUM;
		data->AParam.SpkValue = 95;
		f->private= data;
		f->mcount = 0;
	}

	return 0;
}

/*************************************************
  Function:		ms_ao_audio_uninit
  Description:	音频AO反初始化
  Input: 		
  Output:		
  Return:		 
  Others:
*************************************************/
static int ms_ao_audio_uninit(struct _MSMediaDesc * f)
{
	if (f->mcount == 0)
	{
		ms_free(f->private);
		return 0;
	}
	
	return -1;
}

/*************************************************
  Function:		ms_audio_ao_open
  Description: 	打开音频AO设备
  Input: 		
  Output:		
  Return:		 
  Others:
*************************************************/
static int ms_audio_ao_open(struct _MSMediaDesc * f, void * arg)
{
	int ret = -1;
	AudioAoState * data = (AudioAoState*)f->private;
	
	ms_media_lock(f);
	if (f->mcount == 0)
	{
		#if _AUDIO_AO_FILE_SAVE_
	    g711_ao_file_open(PCM_AO_AUDIO);
		#endif	
		data->AoEnable = HI_TRUE;
		
		ret = audio_play_start(f);
		if (HI_SUCCESS != ret)
		{
			goto error0;
		}
		
		f->mcount++;
		if (f->preprocess)
		{
			f->preprocess(f);
		} 
	}
	else
	{
		f->mcount++;
	}
	ms_media_unlock(f);
	return ret;

error0:	
	
	ms_media_unlock(f);
	return ret;
}


/*************************************************
  Function:		ms_audio_ao_proc
  Description:	发送音频数据到声卡
  Input:		无	
  Output:		无
  Return:		无
  Others:
*************************************************/
void ms_audio_ao_proc(struct _MSMediaDesc * f, mblk_t * arg)
{	
	int status;
    AudioAoState * s = (AudioAoState*)f->private;
	if (s->AoEnable == HI_TRUE)
	{
		#if _AUDIO_AO_FILE_SAVE_
		g711_ao_file_save(PCM_AO_AUDIO, (uint8 *)arg->address, arg->len);
		#endif

		#if _VOICE_SCALE_
		pcm_audio_mult((unsigned char*)arg->address, arg->len, 16, 0.1);
		#endif

		Alsa_Play_Func(arg->address, arg->len, &status);
		//log_printf("audio len [%d]\n", arg->len);
		#if (_AEC_TYPE_ == _SW_AEC_)
		//by zxf
		//gettimeofday(&timestart, NULL);	
		//gettimeofday(&timestop, NULL);
		//usec = (timestop.tv_sec * 1000000 +timestop.tv_usec) - (timestart.tv_sec * 1000000 +timestart.tv_usec);
		//printf("usec=====%ld\n",usec);
		//timestop = timestart;
		//printf("alsa_write_process len==%d\n",arg->len);
		//ace_echo_playback_aec(arg->address);
		ace_echo_playback_aec2((short *)(arg->address), arg->len/sizeof(short));
		//gettimeofday(&timestart, NULL);	
		//gettimeofday(&timestop, NULL);
		/*usec = (timestop.tv_sec * 1000000 +timestop.tv_usec) - (timestart.tv_sec * 1000000 +timestart.tv_usec);
		if (usec > 100 * 1000)
		{
			printf("usec=====%ld\n",usec);
		}
		timestart = timestop;*/
		#endif
	}
}

/*************************************************
  Function:		ms_audio_ao_close
  Description: 	关闭音频AO设备
  Input: 		
  Output:		
  Return:		 
  Others:
*************************************************/
static int ms_audio_ao_close(struct _MSMediaDesc * f, void * arg)
{
	uint32 ret = HI_FAILURE;
	AudioAoState *data=(AudioAoState*)f->private;

	ms_media_lock(f);
	if (f->mcount > 0)
	{
		//f->mcount--;
		f->mcount = 0;
		if (f->mcount == 0)
		{
			#if _AUDIO_AO_FILE_SAVE_
    		g711_ao_file_close(PCM_AO_AUDIO);
			#endif

			Alsa_Play_Close();
			
			if (f->postprocess)
			{
				f->postprocess(f);
			} 
			ret = HI_SUCCESS;
		}
	}
	ms_media_unlock(f);
	
	return ret;
}

/*************************************************
  Function:		ms_audio_ao_param
  Description: 	设置AO参数
  Input: 		
  Output:		
  Return:		 
  Others:
*************************************************/
static int ms_audio_ao_param(struct _MSMediaDesc * f, void * arg)
{
	AudioAoState * data = (AudioAoState*)f->private;
	AUDIO_PARAM * param = (AUDIO_PARAM*)arg;

	ms_media_lock(f);
	if (f->mcount == 0)
	{		
	   memcpy(&data->AParam, param, sizeof(AUDIO_PARAM));
	   ms_media_unlock(f);	
	   return HI_SUCCESS;
	}
	
	ms_media_unlock(f);	
	return HI_FAILURE;
}

/*************************************************
  Function:		ms_audio_ao_enable
  Description: 	使能AO设备
  Input: 		
  Output:		
  Return:		 
  Others:
*************************************************/
static int ms_audio_ao_enable(struct _MSMediaDesc * f, void * arg)
{
	AudioAoState * data = (AudioAoState*)f->private;
	HI_BOOL param = *((HI_BOOL*)arg);

	ms_media_lock(f);
	if (f->mcount > 0)
	{		
	   data->AoEnable = param;
	   ms_media_unlock(f);	
	   return HI_SUCCESS;
	}
	ms_media_unlock(f);
	
	return HI_FAILURE;
}

static MSMediaMethod methods[] =
{
	{MS_AUDIO_AO_OPEN,		ms_audio_ao_open},
	{MS_AUDIO_AO_CLOSE,		ms_audio_ao_close},
	{MS_AUDIO_AO_PARAM,		ms_audio_ao_param},
	{MS_AUDIO_AO_ENABLE,	ms_audio_ao_enable},
	{0,						NULL}
};

MSMediaDesc ms_audio_ao_desc =
{
	.id = MS_ALAW_AO_ID,
	.name = "MsPCMaO",
	.text = "Pcm Ao using N32926 Hw",
	.enc_fmt = "pcm",
	.ninputs = 1,
	.noutputs = 0,
	.outputs = NULL,
	.init = ms_ao_audio_init,
	.uninit = ms_ao_audio_uninit,
	.preprocess = NULL,
	.process = ms_audio_ao_proc,
	.postprocess = NULL,	
	.methods = methods,
	.mcount = 0,
	.porcessmode = PROCESS_WORK_BLOCK, 
	.sharebufsize = 1000,		// 1024*sharebufsize
	.sharebufblk = 200,
	.private = NULL,
};

