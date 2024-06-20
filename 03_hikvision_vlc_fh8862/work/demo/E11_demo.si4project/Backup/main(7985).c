#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include "fh_system_mpi.h"
#include "fh_vpu_mpi.h"
#include "fh_venc_mpi.h"
#include "FHAdv_OSD_mpi.h"
#include "fh_vb_mpipara.h"
#include "fh_vb_mpi.h"

#include "libdmc.h"
#include "libdmc_pes.h"
#include "libdmc_record_raw.h"
#include "font_array.h"
#include "sensor.h"

#include "vicap/fh_vicap_mpi.h"


#define CHECK_RET(state, error_code)																	\
	if (state)																						\
	{																								\
		printf("[%s]:%d line [%s] return 0x%x ERROR\n",__FILE__,__LINE__ , __func__, error_code);	\
		return error_code;																			\
	}

#define ALIGN_UP(addr, edge)   ((addr + edge - 1) & ~(edge - 1)) /* 鏁版嵁缁撴瀯瀵归綈瀹氫箟 */
#define ALIGN_BACK(addr, edge) ((edge) * (((addr) / (edge))))
#define ISP_PROC "/proc/driver/isp"
#define VPU_PROC "/proc/driver/vpu"
#define BGM_PROC "/proc/driver/bgm"
#define ENC_PROC "/proc/driver/enc"
#define JPEG_PROC "/proc/driver/jpeg"
#define TRACE_PROC "/proc/driver/trace"

#define WR_PROC_DEV(device,cmd)  do{ \
    int _tmp_fd; \
    _tmp_fd = open(device, O_RDWR, 0); \
    if(_tmp_fd >= 0) { \
        write(_tmp_fd, cmd, sizeof(cmd)); \
        close(_tmp_fd); \
    } \
}while(0)


static int g_sig_stop = 0;
static void sample_vlcview_handle_sig(int signo)
{
    g_sig_stop = 1;
}


struct isp_sensor_if sensor_func;

#define ISP_W0	3864
#define ISP_H0	2192
#define ISP_W	3840
#define ISP_H	2160
#define ISP_F	30	//图像帧率


static int g_get_stream_stop = 0;
static int g_get_stream_running = 0;

#define GROUP_ID 0 


/*
*   Name: start_isp
*            ISP流程初始化及运行
*
*   Parameters:
*
*            None
*
*   Return:
*            0(正确), 非0(错误)
*   Note:
*       无
*/
int start_isp()
{
	int ret;
	int vimod = 0;//0:online 1:offline
	int vomod = 1;//1:to vpu 2:to ddr

	ISP_MEM_INIT stMemInit = {0};
	ISP_VI_ATTR_S vi_attr = {0};

	ISP_PARAM_CONFIG stIsp_para_cfg;
	unsigned int param_addr, param_size;
	char *isp_param_buff;
	Sensor_Init_t initConf = {0};
	
	FH_VICAP_DEV_ATTR_S stViDev = {0};
	FH_VICAP_VI_ATTR_S stViAttr = {0};

	FILE *param_file;

		
	//sensor复位
	isp_sensor_reset();

	/**ISP memory 分配*/
	stMemInit.enOfflineWorkMode   = vimod;
	stMemInit.enIspOutMode		  = vomod;
	stMemInit.enIspOutFmt		  = 1;//422 8bit
	stMemInit.stPicConf.u32Width  = ISP_W;
	stMemInit.stPicConf.u32Height = ISP_H;
	ret = API_ISP_MemInit(0, &stMemInit);
	CHECK_RET(ret != 0, ret);

	//VI属性配置
	vi_attr.u16InputHeight = ISP_H;
	vi_attr.u16InputWidth  = ISP_W;
	vi_attr.u16PicHeight   = ISP_H;
	vi_attr.u16PicWidth    = ISP_W;
	vi_attr.u16FrameRate = 30;
	vi_attr.enBayerType	= BAYER_GBRG;
	ret = API_ISP_SetViAttr(0, &vi_attr);
	CHECK_RET(ret != 0, ret);

	//isp回调注册，用于sensor控制和AE控制
	sensor_func.init					= sensor_init_imx415;
	sensor_func.set_sns_fmt 			= sensor_set_fmt_imx415;
	sensor_func.set_sns_reg 			= sensor_write_reg;
	sensor_func.get_sns_reg 			= sensor_read_reg;
	sensor_func.set_exposure_ratio		= sensor_set_exposure_ratio_imx415;
	sensor_func.get_exposure_ratio		= sensor_get_exposure_ratio_imx415;
	sensor_func.get_sensor_attribute	= sensor_get_attribute_imx415;
	sensor_func.set_flipmirror			= sensor_set_mirror_flip_imx415;
	sensor_func.get_sns_ae_default		= GetAEDefault;
	sensor_func.get_sns_ae_info 		= GetAEInfo;
	sensor_func.set_sns_gain			= SetGain;
	sensor_func.set_sns_intt			= SetIntt;
	ret = API_ISP_SensorRegCb(0, 0, &sensor_func);
	CHECK_RET(ret != 0, ret);
	
	//sensor初始化
	ret = API_ISP_SensorInit(0, &initConf);
	CHECK_RET(ret != 0, ret);
	
	//ISP初始化
	ret = API_ISP_Init(0);
	CHECK_RET(ret != 0, ret);
	
	//初始化vi设备
	stViDev.enWorkMode = vimod;
	stViDev.stSize.u16Width  = ISP_W;
	stViDev.stSize.u16Height = ISP_H;
	ret = FH_VICAP_InitViDev(0, &stViDev);
	CHECK_RET(ret != 0, ret);
	
	//配置vi attr
	stViAttr.enWorkMode = vimod;
	stViAttr.stInSize.u16Width		= ISP_W0;
	stViAttr.stInSize.u16Height 		= ISP_H0;
	stViAttr.stCropSize.bCutEnable		= 1;
	stViAttr.stCropSize.stRect.u16Width 	= ISP_W;
	stViAttr.stCropSize.stRect.u16Height	= ISP_H;
	ret = FH_VICAP_SetViAttr(0, &stViAttr);
	CHECK_RET(ret != 0, ret);

	if(vimod == 1)
	{
		FH_BIND_INFO src,dst;
		src.obj_id = FH_OBJ_VICAP;
		src.dev_id = 0;
		src.chn_id = 0;
		dst.obj_id = FH_OBJ_ISP;
		dst.dev_id = 0;
		dst.chn_id = 0;
		FH_SYS_Bind(src,dst);
	}
	
	//获取isp参数文件加载地址和大小
	ret = API_ISP_GetBinAddr(0, &stIsp_para_cfg);
	param_size = stIsp_para_cfg.u32BinSize;
	CHECK_RET(ret != 0, ret);
	
	
	//加载isp参数文件
	isp_param_buff = (char*)malloc(param_size);
	param_file = fopen(SENSOR_PARAM, "rb");
	if(NULL == param_file)
	{
		free(isp_param_buff);
		
		printf("open file failed!\n");
		return -1;
	}
	
	if(param_size != fread(isp_param_buff, 1, param_size, param_file))
	{
		free(isp_param_buff);
		fclose(param_file);
		
		printf("open file failed!\n");
		return -1;
	}
	ret = API_ISP_LoadIspParam(0, isp_param_buff);
	CHECK_RET(ret != 0, ret);
	free(isp_param_buff);
	fclose(param_file);

	//运行ISP流水
	ret = isp_server_run();
	CHECK_RET(ret != 0, ret);

	return 0;
}

/*
*   Name: isp_set_param
*            配置图像属性
*
*   Parameters:
*
*            0:ae, 1:awb, 2:色彩, 3:亮度, 4:降噪, 5:镜像
*
*   Return:
*            None
*   Note:
*       无
*/
int isp_set_param(int key, int param)
{
	int ret;

	printf("isp set key %d, val %d\n", key, param);

	switch(key)
	{
		case ISP_AE: //AE使能 范围[0,1]
			ret = isp_set_ae(param);
			break;	
		case ISP_AWB: //AWB使能 范围[0,1]
			ret = isp_set_awb(param);
			break;
		case ISP_COLOR: //色彩 范围[1,255]
			ret = isp_set_saturation(param);
			break;
		case ISP_BRIGHT: //亮度 范围[0,255]
			ret = isp_set_bright(param);
			break;
		case ISP_NR: //降噪 范围[0,1]
			ret = isp_set_nr(param);
			break;
		case ISP_MF: //镜像 范围[0,3]
			ret = isp_set_mirrorflip(param);
			break;
		default:
			printf("Error: not support the key %d\n", key);
			break;
	}
	CHECK_RET(ret != 0, ret);

	return ret;
}


int sample_dmc_init(FH_CHAR *dst_ip, FH_UINT32 port ,FH_SINT32 max_channel_no)
{
    dmc_init();

    if (dst_ip != NULL && *dst_ip != 0)
    {
        dmc_pes_subscribe(max_channel_no, dst_ip, port);
    }

    return 0;
}

FH_VOID *sample_common_get_stream_proc(FH_VOID *arg)
{
    FH_SINT32 ret, i;
    FH_SINT32 end_flag;
    FH_SINT32 subtype;
    FH_VENC_STREAM stream;
    FH_SINT32 *stop = (FH_SINT32 *)arg;

    while (*stop == 0)
    {
        WR_PROC_DEV(TRACE_PROC, "timing_GetStream_START");

    	/*阻塞模式下,获取一帧H264或者H265数据*/
        ret = FH_VENC_GetStream_Block(FH_STREAM_ALL & (~(FH_STREAM_JPEG)), &stream);
        WR_PROC_DEV(TRACE_PROC, "timing_EncBlkFinish_xxx");

        if (ret != 0)
        {
            printf("Error(%d - %x): FH_VENC_GetStream_Block(FH_STREAM_ALL & (~(FH_STREAM_JPEG))) failed!\n", ret, ret);
            continue;
        }
		
		/*获取到一帧H264数据,按照下面的方式处理*/
        if (stream.stmtype == FH_STREAM_H264)
        {
            subtype = stream.h264_stream.frame_type == FH_FRAME_I ? DMC_MEDIA_SUBTYPE_IFRAME : DMC_MEDIA_SUBTYPE_PFRAME;
            for (i = 0; i < stream.h264_stream.nalu_cnt; i++)
            {
            	end_flag = (i == (stream.h264_stream.nalu_cnt - 1)) ? 1 : 0;
                dmc_input(stream.chan,
			    		  DMC_MEDIA_TYPE_H264,
			    		  subtype,
			    		  stream.h264_stream.time_stamp,
		    		      stream.h264_stream.nalu[i].start,
		    		      stream.h264_stream.nalu[i].length,
		    		      end_flag);
            }
        }

		/*获取到一帧H265数据,按照下面的方式处理*/
        else if (stream.stmtype == FH_STREAM_H265)
        {
            subtype = stream.h265_stream.frame_type == FH_FRAME_I ? DMC_MEDIA_SUBTYPE_IFRAME : DMC_MEDIA_SUBTYPE_PFRAME;
            for (i = 0; i < stream.h265_stream.nalu_cnt; i++)
			{
            	end_flag = (i == (stream.h265_stream.nalu_cnt - 1)) ? 1 : 0;
                dmc_input(stream.chan,
			    		  DMC_MEDIA_TYPE_H265,
			    		  subtype,
			    		  stream.h265_stream.time_stamp,
		    		      stream.h265_stream.nalu[i].start,
		    		      stream.h265_stream.nalu[i].length,
		    		      end_flag);
			}
        }

        /*获取到一帧MJPEG数据,按照下面的方式处理*/
        else if (stream.stmtype == FH_STREAM_MJPEG)
        {
            dmc_input(stream.chan,
                      DMC_MEDIA_TYPE_MJPEG,
                      0,
                      0,
                      stream.mjpeg_stream.start,
                      stream.mjpeg_stream.length,
                      1);
        }

		/*必须和FH_VENC_GetStream配套调用,以释放码流资源*/
        ret = FH_VENC_ReleaseStream(&stream);
        if(ret)
        {
            printf("Error(%d - %x): FH_VENC_ReleaseStream failed for chan(%d)!\n", ret, ret, stream.chan);
        }
        WR_PROC_DEV(TRACE_PROC, "timing_GetStream_END");
    }

    *stop = 0;
    return NULL;
}

static int sampe_set_venc_cfg(int chan, int enc_w, int enc_h)
{
	FH_VENC_CHN_CAP cfg_vencmem;

    cfg_vencmem.support_type       = FH_NORMAL_H264|FH_NORMAL_H265;
    cfg_vencmem.max_size.u32Width  = ISP_W;
    cfg_vencmem.max_size.u32Height = ISP_H;
	
    int ret = FH_VENC_CreateChn(chan, &cfg_vencmem);
	if (ret != 0)
	{
	   return ret;
	}
	
	FH_VENC_CHN_CONFIG cfg_param;

	cfg_param.chn_attr.enc_type 					 = FH_NORMAL_H264;
	cfg_param.chn_attr.h264_attr.profile			 = H264_PROFILE_MAIN;
	cfg_param.chn_attr.h264_attr.i_frame_intterval	 = 50;
	cfg_param.chn_attr.h264_attr.size.u32Width		 = enc_w;
	cfg_param.chn_attr.h264_attr.size.u32Height 	 = enc_h;

	cfg_param.rc_attr.rc_type						  = FH_RC_H264_VBR;
	cfg_param.rc_attr.h264_vbr.bitrate				  = 8*1024*1024;
	cfg_param.rc_attr.h264_vbr.init_qp				  = 35;
	cfg_param.rc_attr.h264_vbr.ImaxQP				  = 42;
	cfg_param.rc_attr.h264_vbr.IminQP				  = 28;
	cfg_param.rc_attr.h264_vbr.PmaxQP				  = 42;
	cfg_param.rc_attr.h264_vbr.PminQP				  = 28;
	cfg_param.rc_attr.h264_vbr.FrameRate.frame_count   = 25;
	cfg_param.rc_attr.h264_vbr.FrameRate.frame_time	  = 1;
	cfg_param.rc_attr.h264_vbr.maxrate_percent		  = 200;
	cfg_param.rc_attr.h264_vbr.IFrmMaxBits			  = 0;
	cfg_param.rc_attr.h264_vbr.IP_QPDelta			  = 3;
	cfg_param.rc_attr.h264_vbr.I_BitProp 			  = 5;
	cfg_param.rc_attr.h264_vbr.P_BitProp 			  = 1;
	cfg_param.rc_attr.h264_vbr.fluctuate_level		  = 0;

	return FH_VENC_SetChnAttr(chan, &cfg_param);
}

static int sampe_set_jpeg_cfg(int chan, int enc_w, int enc_h, int quality)
{
	sleep(1);
	
	static int jpeg_cnt = 0;
	static int jpeg_init = 0;
	int ret = 0;

	if(jpeg_init == 0)
	{
		FH_VENC_CHN_CAP cfg_vencmem;

		cfg_vencmem.support_type       = FH_JPEG;
		cfg_vencmem.max_size.u32Width  = ISP_W;
		cfg_vencmem.max_size.u32Height = ISP_H;

		ret = FH_VENC_CreateChn(chan, &cfg_vencmem);
		if (ret != 0)
		{
			return ret;
		}
		jpeg_init = 1;
	}
	FH_VENC_CHN_CONFIG cfg_param = {0};

	cfg_param.chn_attr.enc_type = FH_JPEG;
	cfg_param.chn_attr.jpeg_attr.encode_speed = 4;
	cfg_param.chn_attr.jpeg_attr.qp = quality;

	ret = FH_VENC_SetChnAttr(chan, &cfg_param);
	CHECK_RET(ret != 0, ret);

	FH_BIND_INFO src,dst;
	src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = 0;
    src.chn_id = 0;

    dst.obj_id = FH_OBJ_JPEG;
    dst.dev_id = 0;
    dst.chn_id = chan;
	
    ret = FH_SYS_Bind(src, dst);
	CHECK_RET(ret != 0, ret);

	FH_VENC_STREAM jpeg_stream;
	ret = FH_VENC_GetStream_Block(FH_STREAM_JPEG, &jpeg_stream);
	CHECK_RET(ret != 0, ret);

	if (jpeg_stream.stmtype == FH_STREAM_JPEG)
	{
		char jpeg_path[50]  = {0};
		snprintf(jpeg_path, sizeof(jpeg_path), "/home/jpeg_%d.jpg",jpeg_cnt);
		FILE *jpeg_file = fopen(jpeg_path,"w+");
		if(jpeg_file)
		{
			fwrite(jpeg_stream.jpeg_stream.start, sizeof(char), jpeg_stream.jpeg_stream.length, jpeg_file);
			fclose(jpeg_file);
			printf("get jpeg file %d\n",jpeg_stream.jpeg_stream.length);
		}
		jpeg_cnt++;
	}
	ret = FH_VENC_ReleaseStream(&jpeg_stream);
	CHECK_RET(ret != 0, ret);

	return 0;
}

int sample_set_osd()
{
	int ret;
    int graph_ctrl = 0;
	
    graph_ctrl |= FHT_OSD_GRAPH_CTRL_TOSD_AFTER_VP;

    /* 初始化OSD */
    ret = FHAdv_Osd_Init(0,FHT_OSD_DEBUG_LEVEL_ERROR, graph_ctrl, 0, 0);
    if (ret != FH_SUCCESS)
    {
        printf("FHAdv_Osd_Init failed with %x\n", ret);
        return ret;
    }

	/* 加载asc字库 */
    FHT_OSD_FontLib_t font_lib;

	font_lib.pLibData = asc16;
	font_lib.libSize  = sizeof(asc16);
	ret = FHAdv_Osd_LoadFontLib(FHEN_FONT_TYPE_ASC, &font_lib);
	if (ret != 0)
	{
	    printf("Error: Load ASC font lib failed, ret=%d\n", ret);
	    return ret;
	}

    /* 加载gb2312字库 */
    font_lib.pLibData = gb2312;
	font_lib.libSize  = sizeof(gb2312);
	ret = FHAdv_Osd_LoadFontLib(FHEN_FONT_TYPE_CHINESE, &font_lib);
	if (ret != 0)
	{
	    printf("Error: Load CHINESE font lib failed, ret=%d\n", ret);
	    return ret;
	}

	FHT_OSD_CONFIG_t osd_cfg;
    FHT_OSD_Layer_Config_t  pOsdLayerInfo[1];
    FHT_OSD_TextLine_t text_line_cfg[1];
    FH_CHAR text_data[128]; /*it should be enough*/
    FH_SINT32 user_defined_time = 0;

	memset(&osd_cfg, 0, sizeof(osd_cfg));
    memset(&pOsdLayerInfo[0], 0, sizeof(FHT_OSD_Layer_Config_t));
    memset(&text_line_cfg[0], 0, sizeof(FHT_OSD_TextLine_t));
    memset(&text_data, 0, sizeof(text_data));

	/* 不旋转 */
    osd_cfg.osdRotate        = 0;
    osd_cfg.pOsdLayerInfo = &pOsdLayerInfo[0];
    /* 设置text行块个数 */
    osd_cfg.nOsdLayerNum     = 1; /*我们的demo中只演示了一个行块*/

    pOsdLayerInfo[0].layerStartX = 0;
    pOsdLayerInfo[0].layerStartY = 0;
    /* pOsdLayerInfo[0].layerMaxWidth = 640; */ /*根据需求配置，如果设置则以设置的最大值配置内存，如果缺省则以实际幅面大小申请内存*/
    /* pOsdLayerInfo[0].layerMaxHeight = 480; */
    /* 设置字符大小,像素单位 */
    pOsdLayerInfo[0].osdSize     = 32;

    /* 设置字符颜色为白色 */
    pOsdLayerInfo[0].normalColor.fAlpha = 255;
    pOsdLayerInfo[0].normalColor.fRed   = 255;
    pOsdLayerInfo[0].normalColor.fGreen = 255;
    pOsdLayerInfo[0].normalColor.fBlue  = 255;

    /* 设置字符反色颜色为黑色 */
    pOsdLayerInfo[0].invertColor.fAlpha = 255;
    pOsdLayerInfo[0].invertColor.fRed   = 0;
    pOsdLayerInfo[0].invertColor.fGreen = 0;
    pOsdLayerInfo[0].invertColor.fBlue  = 0;

    /* 设置字符钩边颜色为黑色 */
    pOsdLayerInfo[0].edgeColor.fAlpha = 255;
    pOsdLayerInfo[0].edgeColor.fRed   = 0;
    pOsdLayerInfo[0].edgeColor.fGreen = 0;
    pOsdLayerInfo[0].edgeColor.fBlue  = 0;

    /* 不显示背景 */
    pOsdLayerInfo[0].bkgColor.fAlpha = 0;

    /* 钩边像素为1 */
    pOsdLayerInfo[0].edgePixel        = 1;

    /* 反色控制 */
    pOsdLayerInfo[0].osdInvertEnable  = FH_OSD_INVERT_DISABLE; /*disable反色功能*/
    // pOsdLayerInfo[0].osdInvertEnable  = FH_OSD_INVERT_BY_CHAR; /*以字符为单位进行反色控制*/
    /*osd_cfg.osdInvertEnable  = FH_OSD_INVERT_BY_LINE;*/ /*以行块为单位进行反色控制*/
    pOsdLayerInfo[0].osdInvertThreshold.high_level = 180;
    pOsdLayerInfo[0].osdInvertThreshold.low_level  = 160;
    pOsdLayerInfo[0].layerFlag = FH_OSD_LAYER_USE_TWO_BUF;
    pOsdLayerInfo[0].layerId = 0;
	
	ret = FHAdv_Osd_Ex_SetText(0, 0, &osd_cfg);
    if (ret != FH_SUCCESS)
    {
		printf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
        return ret;
    }
	text_line_cfg[0].textInfo = text_data;

	FH_CHAR user_tag_data[] = {
        0xe4, 0x0d+0, /*FHT_OSD_USER1,此处用于显示当前码率*/
        0x0a,               /*换行*/
        0xe4, 0x01, /*FHT_OSD_YEAR4, 4位年,比如2019*/
        '-',
        0xe4, 0x03, /*FHT_OSD_MONTH2, 2位月份,取值01～12*/
        '-',
        0xe4, 0x04, /*FHT_OSD_DAY, 2位日期,取值01～31*/
        0x20,       /*空格*/
        0xe4, 0x07, /*FHT_OSD_HOUR24, 24时制小时,取值00～23*/
        ':',
        0xe4, 0x09, /*FHT_OSD_MINUTE, 2位分钟,取值00～59*/
        ':',
        0xe4, 0x0a, /*FHT_OSD_SECOND, 2位秒,取值00～59*/
        0,          /*null terminated string*/
    };

    sprintf(text_line_cfg[0].textInfo, "Channel-%d ", 0);
    strcat(text_line_cfg[0].textInfo, user_tag_data);
	text_line_cfg[0].textEnable    = 1;                          /* 使能自定义text */
    text_line_cfg[0].timeOsdEnable = 0;                          /* 去使能时间显示 */
    text_line_cfg[0].textLineWidth = (32/2) * 36;/* 每行最多显示36个像素宽度为16的字符 */
    text_line_cfg[0].linePositionX = 0;                          /* 左上角起始点宽度像素位置 */
    text_line_cfg[0].linePositionY = 0;                          /* 左上角起始点高度像素位置 */

    text_line_cfg[0].lineId = 0;
    text_line_cfg[0].enable = 1;

	ret = FHAdv_Osd_SetTextLine(0, 0, pOsdLayerInfo[0].layerId, &text_line_cfg[0]);
	if (ret != FH_SUCCESS)
	{
		printf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
		return ret;
	}

	return 0;
}

FH_VOID sample_common_media_driver_config(FH_VOID)
{
	VB_CONF_S stVbConf;
    FH_SINT32 ret;

    FH_VB_Exit();

    memset(&stVbConf, 0, sizeof(VB_CONF_S));
    stVbConf.u32MaxPoolCnt = 4;
    stVbConf.astCommPool[0].u32BlkSize = 3840 * 2160 * 3;
    stVbConf.astCommPool[0].u32BlkCnt = 4;
    stVbConf.astCommPool[1].u32BlkSize = 1920 * 1080 * 3;
    stVbConf.astCommPool[1].u32BlkCnt = 4;
    stVbConf.astCommPool[2].u32BlkSize = 1280 * 720 * 3;
    stVbConf.astCommPool[2].u32BlkCnt = 4;
    stVbConf.astCommPool[3].u32BlkSize = 768 * 448 * 3;
    stVbConf.astCommPool[3].u32BlkCnt = 4;

    ret = FH_VB_SetConf(&stVbConf);
    if (ret)
    {
        printf("[FH_VB_SetConf] failed with:%x\n", ret);
    }

    ret = FH_VB_Init();
    if (ret)
    {
        printf("[FH_VB_Init] failed with:%x\n", ret);
    }
	
	WR_PROC_DEV(ENC_PROC, "allchnstm_0_12000000_32");
    WR_PROC_DEV(ENC_PROC, "stm_12000000_32");
    WR_PROC_DEV(JPEG_PROC, "frmsize_1_3000000_3000000");
    WR_PROC_DEV(JPEG_PROC, "jpgstm_12000000_1");
    WR_PROC_DEV(JPEG_PROC, "mjpgstm_12000000_1");
	
}

int main(int argc, char *argv[])
{
    int  ret;
    char *dst_ip;
    unsigned int port;

    /*signal处理, CTRL+C退出*/
    signal(SIGINT,  sample_vlcview_handle_sig);
    signal(SIGQUIT, sample_vlcview_handle_sig);
    signal(SIGKILL, sample_vlcview_handle_sig);
    signal(SIGTERM, sample_vlcview_handle_sig);

    dst_ip = argc > 1 ? argv[1] : NULL;
    port   = argc > 2 ? strtol(argv[2], NULL, 0) : 1234;

	printf("demo_main driver_config\n");

	sample_common_media_driver_config();

	ret = FH_SYS_Init(); 
	CHECK_RET(ret != 0, ret);

	printf("start_isp\n");
	
	start_isp();
	printf("start_isp success\n");
	
	FH_VPU_SET_GRP_INFO grp_info;
	grp_info.vi_max_size.u32Width = ISP_W;
	grp_info.vi_max_size.u32Height = ISP_H;
	grp_info.ycmean_en = 1;
	grp_info.ycmean_ds = 16;

    ret = FH_VPSS_CreateGrp(GROUP_ID, &grp_info);
   CHECK_RET(ret != 0, ret);
	
	FH_VPU_SIZE vi_pic;
	vi_pic.vi_size.u32Width  = ISP_W;
	vi_pic.vi_size.u32Height = ISP_H;
	vi_pic.crop_area.crop_en = 0;
	vi_pic.crop_area.vpu_crop_area.u32X = 0;
	vi_pic.crop_area.vpu_crop_area.u32Y = 0;
	vi_pic.crop_area.vpu_crop_area.u32Width = 0;
	vi_pic.crop_area.vpu_crop_area.u32Height = 0;

	ret = FH_VPSS_SetViAttr(GROUP_ID,&vi_pic);
	CHECK_RET(ret != 0, ret);

	ret = FH_VPSS_Enable(GROUP_ID, VPU_MODE_ISP);
	CHECK_RET(ret != 0, ret)

	FH_VPU_CHN_INFO chn_info = {0};
	chn_info.bgm_enable = 1;
	chn_info.cpy_enable = 1;
	chn_info.sad_enable = 1;
	chn_info.bgm_ds = 8;
	chn_info.chn_max_size.u32Width = ISP_W;
    chn_info.chn_max_size.u32Height = ISP_H;
	chn_info.out_mode = VPU_VOMODE_SCAN;
	chn_info.support_mode = 1<<chn_info.out_mode;
	chn_info.bufnum = 3;
	chn_info.max_stride = 0;
	ret = FH_VPSS_CreateChn(GROUP_ID, 0, &chn_info);
	CHECK_RET(ret != 0, ret);

    FH_VPU_CHN_CONFIG chn_attr;
    chn_attr.vpu_chn_size.u32Width  = 3840;
    chn_attr.vpu_chn_size.u32Height = 2160;
	chn_attr.crop_area.crop_en = 0;
    chn_attr.crop_area.vpu_crop_area.u32X = 0;
    chn_attr.crop_area.vpu_crop_area.u32Y = 0;
    chn_attr.crop_area.vpu_crop_area.u32Width = 0;
    chn_attr.crop_area.vpu_crop_area.u32Height = 0;
    chn_attr.offset = 0;
    chn_attr.depth = 1;
    chn_attr.stride = 0;
    ret = FH_VPSS_SetChnAttr(GROUP_ID, 0, &chn_attr);  //配置VPU的图像宽高，可放大缩小
    CHECK_RET(ret != 0, ret);

    ret = FH_VPSS_SetVOMode(GROUP_ID, 0, VPU_VOMODE_SCAN);
    CHECK_RET(ret != 0, ret);

	ret = FH_VPSS_OpenChn(GROUP_ID, 0);
	CHECK_RET(ret != 0, ret);

	ret = sampe_set_venc_cfg(0, 3840, 2160);//配置编码的宽高，需要与绑定的VPU的宽高一致
	CHECK_RET(ret != 0, ret);

	FH_BIND_INFO src, dst;
	src.obj_id = FH_OBJ_ISP;
    src.dev_id = 0;
    src.chn_id = 0;
    dst.obj_id = FH_OBJ_VPU_VI;
    dst.dev_id = 0;
    dst.chn_id = 0;
    ret = FH_SYS_Bind(src, dst);
	CHECK_RET(ret != 0, ret);
	
	ret = FH_VENC_StartRecvPic(0);
	CHECK_RET(ret != 0, ret);
	
    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = GROUP_ID;
    src.chn_id = 0;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = 0;
	
    ret = FH_SYS_Bind(src, dst);
	CHECK_RET(ret != 0, ret);

	sample_dmc_init(dst_ip, port, 1);
	
	pthread_attr_t attr;
	pthread_t thread_stream;
	
	if(!g_get_stream_running)
	{
		g_get_stream_running = 1;
		g_get_stream_stop = 0;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setstacksize(&attr, 3 * 1024);
		pthread_create(&thread_stream, &attr, sample_common_get_stream_proc, &g_get_stream_stop);
	}
#if 0	
	ret = sample_set_osd();
	CHECK_RET(ret != 0, ret);

#endif

	isp_set_param(ISP_AE, 1);  //配置自动曝光

	isp_set_param(ISP_AWB, 1); //配置自动白平衡

	isp_set_param(ISP_COLOR, 25); //配置图像饱和度

	isp_set_param(ISP_BRIGHT, 125); //配置亮度

	isp_set_param(ISP_NR, 1); //配置降噪

	isp_set_param(ISP_MF, 0); //配置镜像
#if 0
	FH_VPU_MASK stVpumaskinfo;  //配置马赛克
	memset(&stVpumaskinfo,0x0,sizeof(FH_VPU_MASK));

	stVpumaskinfo.mask_enable[0] = 1;
	stVpumaskinfo.area_value[0].u32X = 0;
	stVpumaskinfo.area_value[0].u32Y = 0;
	stVpumaskinfo.area_value[0].u32Width = 200; 
	stVpumaskinfo.area_value[0].u32Height = 200; 
	stVpumaskinfo.color = 0xffda059a;//黑色 0xff008080 白色 0xffff8080 黄色 0xffda059a 蓝色 0xff7abe29
	
	ret = FH_VPSS_SetMask(&stVpumaskinfo);
	CHECK_RET(ret != 0, ret);
#endif
#if 0
	ret = sampe_set_jpeg_cfg(1, 2560, 1440, 40);  //抓图
	CHECK_RET(ret != 0, ret);

	ret = sampe_set_jpeg_cfg(1, 2560, 1440, 80);
	CHECK_RET(ret != 0, ret);
#endif
    while (!g_sig_stop)
    {
        usleep(20000);
    }
	
	FH_VICAP_Exit(0);
	API_ISP_Exit(0);
	FH_SYS_Exit();
	
	return;

    return 0;
}

