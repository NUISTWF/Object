#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>


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

#define ALIGN_UP(addr, edge)   ((addr + edge - 1) & ~(edge - 1)) /* 数据结构对齐定义 */
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
#define ISP_F	30	//ͼ��֡��


static int g_get_stream_stop = 0;
static int g_get_stream_running = 0;

#define GROUP_ID 0 

int stream_tag;
int stream_key1 = 0;
int stream_key2 = 0;
char *kernel_buf;
int recordTime = 0;
double bitrate0;
double bitrate1;
/*
*   Name: start_isp
*            ISP���̳�ʼ��������
*
*   Parameters:
*
*            None
*
*   Return:
*            0(��ȷ), ��0(����)
*   Note:
*       ��
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

		
	//sensor��λ
	isp_sensor_reset();

	/**ISP memory ����*/
	stMemInit.enOfflineWorkMode   = vimod;
	stMemInit.enIspOutMode		  = vomod;
	stMemInit.enIspOutFmt		  = 1;//422 8bit
	stMemInit.stPicConf.u32Width  = ISP_W;
	stMemInit.stPicConf.u32Height = ISP_H;
	ret = API_ISP_MemInit(0, &stMemInit);
	CHECK_RET(ret != 0, ret);

	//VI��������
	vi_attr.u16InputHeight = ISP_H;
	vi_attr.u16InputWidth  = ISP_W;
	vi_attr.u16PicHeight   = ISP_H;
	vi_attr.u16PicWidth    = ISP_W;
	vi_attr.u16FrameRate = 30;
	vi_attr.enBayerType	= BAYER_GBRG;
	ret = API_ISP_SetViAttr(0, &vi_attr);
	CHECK_RET(ret != 0, ret);

	//isp�ص�ע�ᣬ����sensor���ƺ�AE����
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
	
	//sensor��ʼ��
	ret = API_ISP_SensorInit(0, &initConf);
	CHECK_RET(ret != 0, ret);
	
	//ISP��ʼ��
	ret = API_ISP_Init(0);
	CHECK_RET(ret != 0, ret);
	
	//��ʼ��vi�豸
	stViDev.enWorkMode = vimod;
	stViDev.stSize.u16Width  = ISP_W;
	stViDev.stSize.u16Height = ISP_H;
	ret = FH_VICAP_InitViDev(0, &stViDev);
	CHECK_RET(ret != 0, ret);
	
	//����vi attr
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
	
	//��ȡisp�����ļ����ص�ַ�ʹ�С
	ret = API_ISP_GetBinAddr(0, &stIsp_para_cfg);
	param_size = stIsp_para_cfg.u32BinSize;
	CHECK_RET(ret != 0, ret);
	
	
	//����isp�����ļ�
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

	//����ISP��ˮ
	ret = isp_server_run();
	CHECK_RET(ret != 0, ret);

	return 0;
}

/*
*   Name: isp_set_param
*            ����ͼ������
*
*   Parameters:
*
*            0:ae, 1:awb, 2:ɫ��, 3:����, 4:����, 5:����
*
*   Return:
*            None
*   Note:
*       ��
*/
int isp_set_param(int key, int param)
{
	int ret;

	printf("isp set key %d, val %d\n", key, param);

	switch(key)
	{
		case ISP_AE: //AEʹ�� ��Χ[0,1]
			ret = isp_set_ae(param);
			break;	
		case ISP_AWB: //AWBʹ�� ��Χ[0,1]
			ret = isp_set_awb(param);
			break;
		case ISP_COLOR: //ɫ�� ��Χ[1,255]
			ret = isp_set_saturation(param);
			break;
		case ISP_BRIGHT: //���� ��Χ[0,255]
			ret = isp_set_bright(param);
			break;
		case ISP_NR: //���� ��Χ[0,1]
			ret = isp_set_nr(param);
			break;
		case ISP_MF: //���� ��Χ[0,3]
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
        WR_PROC_DEV(TRACE_PROC, "timing_GetStream_START");	// 往第一个文件"/proc/driver/trace"内写入第二个的内容

    	/*����ģʽ��,��ȡһ֡H264����H265����*/
        ret = FH_VENC_GetStream_Block(FH_STREAM_ALL & (~(FH_STREAM_JPEG)), &stream);
        WR_PROC_DEV(TRACE_PROC, "timing_EncBlkFinish_xxx");

        if (ret != 0)
        {
            printf("Error(%d - %x): FH_VENC_GetStream_Block(FH_STREAM_ALL & (~(FH_STREAM_JPEG))) failed!\n", ret, ret);
            continue;
        }
		
		/*��ȡ��һ֡H264����,��������ķ�ʽ����*/
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

		/*��ȡ��һ֡H265����,��������ķ�ʽ����*/
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

        /*��ȡ��һ֡MJPEG����,��������ķ�ʽ����*/
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

		/*�����FH_VENC_GetStream���׵���,���ͷ�������Դ*/
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

// 通道0 1280 720
static int sampe_set_venc_cfg(int chan, int enc_w, int enc_h)
{
	FH_VENC_CHN_CAP cfg_vencmem;

    cfg_vencmem.support_type       = FH_NORMAL_H264|FH_NORMAL_H265;	// 编码类型：用的H264和H265的编码
    cfg_vencmem.max_size.u32Width  = ISP_W;	// 最大编码幅面，也就是分辨率
    cfg_vencmem.max_size.u32Height = ISP_H;
	
    int ret = FH_VENC_CreateChn(chan, &cfg_vencmem);	// VPU创建了双通道后，VENC也就创建了对应的通道
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
	if (chan == 0)
		cfg_param.rc_attr.h264_vbr.bitrate			  = 2*1024*1024;
	if (chan == 1)
		cfg_param.rc_attr.h264_vbr.bitrate			  = 1*1024*1024;
	cfg_param.rc_attr.h264_vbr.init_qp				  = 35;
	cfg_param.rc_attr.h264_vbr.ImaxQP				  = 42;
	cfg_param.rc_attr.h264_vbr.IminQP				  = 28;
	cfg_param.rc_attr.h264_vbr.PmaxQP				  = 42;
	cfg_param.rc_attr.h264_vbr.PminQP				  = 28;
	if (chan == 0)
		cfg_param.rc_attr.h264_vbr.FrameRate.frame_count   = 25;
	if (chan == 1)
		cfg_param.rc_attr.h264_vbr.FrameRate.frame_count   = 25;
	cfg_param.rc_attr.h264_vbr.FrameRate.frame_time	  = 1;
	cfg_param.rc_attr.h264_vbr.maxrate_percent		  = 200;
	cfg_param.rc_attr.h264_vbr.IFrmMaxBits			  = 0;
	cfg_param.rc_attr.h264_vbr.IP_QPDelta			  = 3;
	cfg_param.rc_attr.h264_vbr.I_BitProp 			  = 5;
	cfg_param.rc_attr.h264_vbr.P_BitProp 			  = 1;
	cfg_param.rc_attr.h264_vbr.fluctuate_level		  = 0;
	// 创建完通道后就设置编码通道属性
	return FH_VENC_SetChnAttr(chan, &cfg_param);	// 设置编码通道属性（通道号，编码通道属性）
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

	while(1)
	{
		ret = FH_VENC_GetStream_Timeout(FH_STREAM_JPEG, &jpeg_stream,1000);
		if(ret == 0)
		{
			break;
		}
	}
	
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

	ret = FH_SYS_UnBindbyDst(dst);
	CHECK_RET(ret != 0, ret);

	return 0;
}

int sample_set_osd()
{
	int ret;
    int graph_ctrl = 0;
	
    graph_ctrl |= FHT_OSD_GRAPH_CTRL_TOSD_AFTER_VP;

    /* ��ʼ��OSD */
    ret = FHAdv_Osd_Init(0,FHT_OSD_DEBUG_LEVEL_ERROR, graph_ctrl, 0, 0);
	//ret = FHAdv_Osd_Init(1,FHT_OSD_DEBUG_LEVEL_ERROR, graph_ctrl, 0, 0);
    if (ret != FH_SUCCESS)
    {
        printf("FHAdv_Osd_Init failed with %x\n", ret);
        return ret;
    }

	/* ����asc�ֿ� */
    FHT_OSD_FontLib_t font_lib;

	font_lib.pLibData = asc16;
	font_lib.libSize  = sizeof(asc16);
	ret = FHAdv_Osd_LoadFontLib(FHEN_FONT_TYPE_ASC, &font_lib);
	if (ret != 0)
	{
	    printf("Error: Load ASC font lib failed, ret=%d\n", ret);
	    return ret;
	}

    /* ����gb2312�ֿ� */
    font_lib.pLibData = gb2312;
	font_lib.libSize  = sizeof(gb2312);
	ret = FHAdv_Osd_LoadFontLib(FHEN_FONT_TYPE_CHINESE, &font_lib);
	if (ret != 0)
	{
	    printf("Error: Load CHINESE font lib failed, ret=%d\n", ret);
	    return ret;
	}

	FHT_OSD_CONFIG_t osd_cfg;
    FHT_OSD_Layer_Config_t  pOsdLayerInfo[4];
    FHT_OSD_TextLine_t text_line_cfg[4];
    FH_CHAR text_data[4][128]; /*it should be enough*/
    FH_SINT32 user_defined_time = 0;

	memset(&osd_cfg, 0, sizeof(osd_cfg));
    memset(&pOsdLayerInfo[0], 0, 4 * sizeof(FHT_OSD_Layer_Config_t));
	// 自动反色
	// memset(&pOsdLayerInfo[1], 0, 4 * sizeof(FHT_OSD_Layer_Config_t));
    memset(&text_line_cfg[0], 0, 4 * sizeof(FHT_OSD_TextLine_t));
    memset(&text_data, 0, sizeof(text_data));

	/* ����ת */
    osd_cfg.osdRotate        = 0;
    osd_cfg.pOsdLayerInfo = &pOsdLayerInfo[0];
    /* ����text�п���� */
    osd_cfg.nOsdLayerNum     = 1; /*���ǵ�demo��ֻ��ʾ��һ���п�*/

    pOsdLayerInfo[0].layerStartX = 0;
    pOsdLayerInfo[0].layerStartY = 0;
    /* pOsdLayerInfo[0].layerMaxWidth = 640; */ /*�����������ã���������������õ����ֵ�����ڴ棬���ȱʡ����ʵ�ʷ����С�����ڴ�*/
    /* pOsdLayerInfo[0].layerMaxHeight = 480; */
    /* �����ַ���С,���ص�λ */

	// 设置字符大小的
    pOsdLayerInfo[0].osdSize     = 48;

    /* �����ַ���ɫΪ��ɫ */
    pOsdLayerInfo[0].normalColor.fAlpha = 255;
    pOsdLayerInfo[0].normalColor.fRed   = 255;
    pOsdLayerInfo[0].normalColor.fGreen = 255;
    pOsdLayerInfo[0].normalColor.fBlue  = 255;

    /* �����ַ���ɫ��ɫΪ��ɫ */
    pOsdLayerInfo[0].invertColor.fAlpha = 255;
    pOsdLayerInfo[0].invertColor.fRed   = 0;
    pOsdLayerInfo[0].invertColor.fGreen = 0;
    pOsdLayerInfo[0].invertColor.fBlue  = 0;

    /* �����ַ�������ɫΪ��ɫ */
    pOsdLayerInfo[0].edgeColor.fAlpha = 255;
    pOsdLayerInfo[0].edgeColor.fRed   = 0;
    pOsdLayerInfo[0].edgeColor.fGreen = 0;
    pOsdLayerInfo[0].edgeColor.fBlue  = 0;

    /* ����ʾ���� */
    pOsdLayerInfo[0].bkgColor.fAlpha = 0;

    /* ��������Ϊ1 */
    pOsdLayerInfo[0].edgePixel        = 1;

    /* ��ɫ���� */
	//pOsdLayerInfo[0].osdInvertEnable  = FH_OSD_INVERT_DISABLE; /*disable��ɫ����*/
	// ===============改成自动反色===============
	pOsdLayerInfo[0].osdInvertEnable  = 1;

    pOsdLayerInfo[0].osdInvertThreshold.high_level = 180;
    pOsdLayerInfo[0].osdInvertThreshold.low_level  = 160;
    pOsdLayerInfo[0].layerFlag = FH_OSD_LAYER_USE_TWO_BUF;
    pOsdLayerInfo[0].layerId = 0;
	
	// 设置两个通道，第二个参数是通道号
	ret = FHAdv_Osd_Ex_SetText(0, 1, &osd_cfg);
	if (ret != FH_SUCCESS)
    {
		printf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
        return ret;
    }
	// ==================================================
	printf("=====================\n");
	ret = FHAdv_Osd_Ex_SetText(0, 0, &osd_cfg);
    if (ret != FH_SUCCESS)
    {
		printf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
        return ret;
    }
	text_line_cfg[0].textInfo = text_data[0];
	text_line_cfg[1].textInfo = text_data[1];
	text_line_cfg[2].textInfo = text_data[2];
	text_line_cfg[3].textInfo = text_data[3];

	FH_CHAR user_tag_data[] = {
        0xe4, 0x0d+0, /*FHT_OSD_USER1,�˴�������ʾ��ǰ����*/
        0x0a,               /*����*/
        0xe4, 0x01, /*FHT_OSD_YEAR4, 4λ��,����2019*/
        '-',
        0xe4, 0x03, /*FHT_OSD_MONTH2, 2λ�·�,ȡֵ01��12*/
        '-',
        0xe4, 0x04, /*FHT_OSD_DAY, 2λ����,ȡֵ01��31*/
        0x20,       /*�ո�*/
        0xe4, 0x07, /*FHT_OSD_HOUR24, 24ʱ��Сʱ,ȡֵ00��23*/
        ':',
        0xe4, 0x09, /*FHT_OSD_MINUTE, 2λ����,ȡֵ00��59*/
        ':',
        0xe4, 0x0a, /*FHT_OSD_SECOND, 2λ��,ȡֵ00��59*/
        0,          /*null terminated string*/
    };
#if 1
 	sprintf(text_line_cfg[0].textInfo, "Camera Channel - %d, 1280*720", 0);
	//sprintf(text_line_cfg[0].textInfo, "Camera Channel - %d", 1);
	text_line_cfg[0].textEnable    = 1;                          /* ʹ���Զ���text */
    text_line_cfg[0].timeOsdEnable = 0;                          /* ȥʹ��ʱ����ʾ */
    text_line_cfg[0].textLineWidth = (64/2) * 36;				/* ÿ�������ʾ36�����ؿ���Ϊ32���ַ� */
    text_line_cfg[0].linePositionX = 100;                          /* ���Ͻ���ʼ���������λ�� */
    text_line_cfg[0].linePositionY = 240;                          /* ���Ͻ���ʼ��߶�����λ�� */

    text_line_cfg[0].lineId = 0;
    text_line_cfg[0].enable = 1;

	ret = FHAdv_Osd_SetTextLine(0, 0, pOsdLayerInfo[0].layerId, &text_line_cfg[0]);
	// 第二个参数是vpu通道号
	sprintf(text_line_cfg[0].textInfo, "Camera Channel - %d, 768*448", 1);
	FHAdv_Osd_SetTextLine(0, 1, pOsdLayerInfo[0].layerId, &text_line_cfg[0]);
	if (ret != FH_SUCCESS)
	{
		printf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
		return ret;
	}
#endif	
#if 1
	/*
	char buffer[128];
	time_t stamp;
	struct tm *timeinfo;
	time(&stamp);
	timeinfo = localtime(&stamp);

	char time_str[50] = {0}; // 足够大的缓冲区来存储时间字符串  
    sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d",   
            timeinfo->tm_year + 1954, // 年份从1900年开始计算  
            timeinfo->tm_mon + 5,     // 月份从0开始，所以需要+1  
            timeinfo->tm_mday + 7,    // 日期  
            timeinfo->tm_hour,        // 小时  
            timeinfo->tm_min,         // 分钟  
            timeinfo->tm_sec);        // 秒  
  
    printf("Current time: %s\n", time_str);

	struct timeval tv1;
	gettimeofday(&tv1, NULL);
	printf("time = %ld\n", tv1.tv_sec);
	*/
	strcat(text_line_cfg[1].textInfo, user_tag_data);
	text_line_cfg[1].textEnable    = 0;                          /* 文字字符使能 */
    text_line_cfg[1].timeOsdEnable = 1;                          /* 时间显示使能 */
    text_line_cfg[1].timeOsdFlags = FHT_OSD_TIME_BIT_WEEK | FHT_OSD_TIME_BIT_AFTER;
	text_line_cfg[1].textLineWidth = (64/2) * 36;				/* ÿ�������ʾ36�����ؿ���Ϊ32���ַ� */
    text_line_cfg[1].linePositionX = 640;                          /* ���Ͻ���ʼ���������λ�� */
    text_line_cfg[1].linePositionY = 600;                          /* ���Ͻ���ʼ��߶�����λ�� */
	text_line_cfg[1].showFlag = FHT_OSD_TEXT_RIGHT_ALIGN;
    text_line_cfg[1].lineId = 1;
    text_line_cfg[1].enable = 1;

	
	// 搞个子进程不断地显示刷新时间 fork()
	//FHAdv_Osd_SetTextLine(0, 1, pOsdLayerInfo[0].layerId, &text_line_cfg[1]);
	ret = FHAdv_Osd_SetTextLine(0, 0, pOsdLayerInfo[0].layerId, &text_line_cfg[1]);
	if (ret != FH_SUCCESS)
	{
		printf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
		return ret;
	}
#endif
#if 1
 	sprintf(text_line_cfg[2].textInfo, "Bug destroyer");
	text_line_cfg[2].textEnable    = 1;                          /* 使锟斤拷锟皆讹拷锟斤拷text */
    text_line_cfg[2].timeOsdEnable = 0;                          /* 去使锟斤拷时锟斤拷锟斤拷示 */
    text_line_cfg[2].textLineWidth = (64/2) * 36;				/* 每锟斤拷锟斤拷锟斤拷锟绞?6锟斤拷锟斤拷锟截匡拷锟斤拷为32锟斤拷锟街凤拷 */
    text_line_cfg[2].linePositionX = 120;                          /* 锟斤拷锟较斤拷锟斤拷始锟斤拷锟斤拷锟斤拷锟斤拷锟轿伙拷锟?*/
    text_line_cfg[2].linePositionY = 180;                          /* 锟斤拷锟较斤拷锟斤拷始锟斤拷叨锟斤拷锟斤拷锟轿伙拷锟?*/
	text_line_cfg[2].showFlag = FHT_OSD_TEXT_TWINKLE;
    text_line_cfg[2].lineId = 2;
    text_line_cfg[2].enable = 1;

	ret = FHAdv_Osd_SetTextLine(0, 0, pOsdLayerInfo[0].layerId, &text_line_cfg[2]);
	if (ret != FH_SUCCESS)
	{
		printf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
		return ret;
	}
#endif
#if 1
 	sprintf(text_line_cfg[3].textInfo, "Hangzhou HIKVISION");
	text_line_cfg[3].textEnable    = 1;                          /* 使锟斤拷锟皆讹拷锟斤拷text */
    text_line_cfg[3].timeOsdEnable = 0;                          /* 去使锟斤拷时锟斤拷锟斤拷示 */
    text_line_cfg[3].textLineWidth = (64/2) * 36;				/* 每锟斤拷锟斤拷锟斤拷锟绞?6锟斤拷锟斤拷锟截匡拷锟斤拷为32锟斤拷锟街凤拷 */
    text_line_cfg[3].linePositionX = 120;                          /* 锟斤拷锟较斤拷锟斤拷始锟斤拷锟斤拷锟斤拷锟斤拷锟轿伙拷锟?*/
    text_line_cfg[3].linePositionY = 120;                          /* 锟斤拷锟较斤拷锟斤拷始锟斤拷叨锟斤拷锟斤拷锟轿伙拷锟?*/

    text_line_cfg[3].lineId = 3;
    text_line_cfg[3].enable = 1;

	ret = FHAdv_Osd_SetTextLine(0, 0, pOsdLayerInfo[0].layerId, &text_line_cfg[3]);
	if (ret != FH_SUCCESS)
	{
		printf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
		return ret;
	}
	sprintf(text_line_cfg[3].textInfo, "Hangzhou HIKVISION");
	FHAdv_Osd_SetTextLine(0, 1, pOsdLayerInfo[0].layerId, &text_line_cfg[3]);
#endif

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
	
	WR_PROC_DEV(ENC_PROC, "allchnstm_0_20000000_40");
    WR_PROC_DEV(ENC_PROC, "stm_20000000_40");
    WR_PROC_DEV(JPEG_PROC, "frmsize_1_3000000_3000000");
    WR_PROC_DEV(JPEG_PROC, "jpgstm_12000000_2");
    WR_PROC_DEV(JPEG_PROC, "mjpgstm_12000000_2");
	
}


// ===========================================自定义处理函数===========================================
// 启动暂停的线程
void stream_handler(int signo){
	printf("stream_switch\n");
	fflush(stdout);
	if(stream_tag == 1){
		stream_tag = 0;
	}
	else
		stream_tag = 1;
}

// 翻转视频的线程
void *thread_key2_function(void* arg)
{
	int fd = (int)arg;
	int val = 0;
 	while (1) {
		read(fd, &val, 4);
		printf("val = %d\n", val);
		isp_set_param(ISP_MF, val);
	}	
}

// 板子录视频的线程
void *thread_key3_function(void* arg)
{	
	int fd = (int)arg;
	int val = 0;
	while (1) {
		if (strcmp(kernel_buf, "open") == 0) {
			stream_key1 = 1;
			stream_key2 = 1;
			printf("===== start recording ! =====\n");
			strcpy(kernel_buf, "hello");
		}
		if ((stream_key1 == 2) && (stream_key2 == 2)) {
			val = 1;
			write(fd, &val, 4);
			stream_key1 = 0;
			stream_key2 = 0;
			printf("===== recording end! =====\n");
		}
	}
}

// 实时显示码率
void *thread_bitrate_function(void* arg)
{
	FH_CHN_STATUS chn_attr_get_0;
	FH_CHN_STATUS chn_attr_get_1;
	while (1) {
		FH_VENC_GetChnStatus(0, &chn_attr_get_0); // 通道状态信息
		bitrate0 = chn_attr_get_0.bps;
		bitrate0 = bitrate0 / 8388608;
		char bitrateBuffer[128] = {0};
		sprintf(bitrateBuffer, "bitrate:%.3fMB/s", bitrate0);
		//printf("%s\n", bitrateBuffer);

		FH_VENC_GetChnStatus(1, &chn_attr_get_1); // 通道状态信息
		bitrate1 = chn_attr_get_1.bps;
		bitrate1 = bitrate1 / 8388608;
		char bitrateBuffer1[128] = {0};
		sprintf(bitrateBuffer1, "bitrate:%.3fMB/s", bitrate1);
		//printf("%s\n", bitrateBuffer1);

		FHT_OSD_CONFIG_t osd_cfg;
    	FHT_OSD_Layer_Config_t  pOsdLayerInfo[4];
    	FHT_OSD_TextLine_t text_line_cfg[4];
    	FH_CHAR text_data[4][128];
    	FH_SINT32 user_defined_time = 0;

		memset(&osd_cfg, 0, sizeof(osd_cfg));
    	memset(&pOsdLayerInfo[0], 0, 4 * sizeof(FHT_OSD_Layer_Config_t));
    	memset(&text_line_cfg[0], 0, 4 * sizeof(FHT_OSD_TextLine_t));
    	memset(&text_data, 0, sizeof(text_data));

    	osd_cfg.osdRotate        = 0;
    	osd_cfg.pOsdLayerInfo = &pOsdLayerInfo[0];
    	osd_cfg.nOsdLayerNum     = 1;

		pOsdLayerInfo[0].layerStartX = 0;
		pOsdLayerInfo[0].layerStartY = 0;

		// 设置字符大小的
    	pOsdLayerInfo[0].osdSize     = 48;

		pOsdLayerInfo[0].normalColor.fAlpha = 255;
		pOsdLayerInfo[0].normalColor.fRed   = 255;
		pOsdLayerInfo[0].normalColor.fGreen = 255;
		pOsdLayerInfo[0].normalColor.fBlue  = 255;

		/* �����ַ���ɫ��ɫΪ��ɫ */
		pOsdLayerInfo[0].invertColor.fAlpha = 255;
		pOsdLayerInfo[0].invertColor.fRed   = 0;
		pOsdLayerInfo[0].invertColor.fGreen = 0;
		pOsdLayerInfo[0].invertColor.fBlue  = 0;

		/* �����ַ�������ɫΪ��ɫ */
		pOsdLayerInfo[0].edgeColor.fAlpha = 255;
		pOsdLayerInfo[0].edgeColor.fRed   = 0;
		pOsdLayerInfo[0].edgeColor.fGreen = 0;
		pOsdLayerInfo[0].edgeColor.fBlue  = 0;

		/* ����ʾ���� */
		pOsdLayerInfo[0].bkgColor.fAlpha = 0;

		/* ��������Ϊ1 */
		pOsdLayerInfo[0].edgePixel        = 1;

		/* ��ɫ���� */
		//pOsdLayerInfo[0].osdInvertEnable  = FH_OSD_INVERT_DISABLE; /*disable��ɫ����*/
		// ===============改成自动反色===============
		pOsdLayerInfo[0].osdInvertEnable  = 1;

		pOsdLayerInfo[0].osdInvertThreshold.high_level = 180;
		pOsdLayerInfo[0].osdInvertThreshold.low_level  = 160;
		pOsdLayerInfo[0].layerFlag = FH_OSD_LAYER_USE_TWO_BUF;
		pOsdLayerInfo[0].layerId = 0;
	
		// 设置两个通道，第二个参数是通道号
		int ret;
		ret = FHAdv_Osd_Ex_SetText(0, 1, &osd_cfg);
		if (ret != FH_SUCCESS)
			printf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
		// ==================================================
		
		ret = FHAdv_Osd_Ex_SetText(0, 0, &osd_cfg);
		if (ret != FH_SUCCESS)
			printf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
		text_line_cfg[0].textInfo = text_data[0];
		text_line_cfg[1].textInfo = text_data[1];
		text_line_cfg[2].textInfo = text_data[2];
		text_line_cfg[3].textInfo = text_data[3];


		sprintf(text_line_cfg[0].textInfo, bitrateBuffer);
		text_line_cfg[0].textEnable    = 1;   
		text_line_cfg[0].timeOsdEnable = 0;   
		text_line_cfg[0].textLineWidth = (64/2) * 36;
		text_line_cfg[0].linePositionX = 120; 
		text_line_cfg[0].linePositionY = 120;
		text_line_cfg[0].lineId = 3;
		text_line_cfg[0].enable = 1;

		ret = FHAdv_Osd_SetTextLine(0, 0, pOsdLayerInfo[0].layerId, &text_line_cfg[0]);


		sprintf(text_line_cfg[0].textInfo, bitrateBuffer1);
		FHAdv_Osd_SetTextLine(0, 1, pOsdLayerInfo[0].layerId, &text_line_cfg[0]);
		sleep(1);
		bitrate0 = 0;
	}
}

int main(int argc, char *argv[])
{
    int  ret;
    char *dst_ip;
    unsigned int port;

// ===========================================打开按键设备===========================================
	// 录制时间
	recordTime = 60;

	printf("You will record for %d seconds\n", recordTime);
	stream_tag = 1;
	int button_fd;
	int flags;
	button_fd = open("/dev/cam_ctrl", O_RDWR);
	if(button_fd < 0){
		perror("open:cam_ctrl failed\n");
	}
	fcntl(button_fd, F_SETOWN, getpid());
	flags = fcntl(button_fd, F_GETFL);
	fcntl(button_fd, F_SETFL,flags | FASYNC);
	signal(SIGIO, stream_handler);

	// 上电是0，不录取流，按键按下才开始录流
	stream_key1 = 0;
	stream_key2 = 0;
	kernel_buf = mmap(NULL, 1024*8, PROT_READ | PROT_WRITE, MAP_SHARED, button_fd, 0);
	if (kernel_buf == MAP_FAILED) {
		printf("can not mmap file /dev/cam_ctrl\n");
	}
	pthread_t tid3;
	pthread_create(&tid3, NULL, thread_key3_function, (void*)button_fd);





    /*signal����, CTRL+C�˳�*/
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
	// vpu 通道属性
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
	// 创建通道0
	ret = FH_VPSS_CreateChn(GROUP_ID, 0, &chn_info);
	CHECK_RET(ret != 0, ret);
	// 创建通道1
	ret = FH_VPSS_CreateChn(GROUP_ID, 1, &chn_info);
	CHECK_RET(ret != 0, ret);


	// 通道0是 1280*720
    FH_VPU_CHN_CONFIG chn_attr;
    chn_attr.vpu_chn_size.u32Width  = 1280;
    chn_attr.vpu_chn_size.u32Height = 720;
	chn_attr.crop_area.crop_en = 0;
    chn_attr.crop_area.vpu_crop_area.u32X = 0;
    chn_attr.crop_area.vpu_crop_area.u32Y = 0;
    chn_attr.crop_area.vpu_crop_area.u32Width = 0;
    chn_attr.crop_area.vpu_crop_area.u32Height = 0;
    chn_attr.offset = 0;
    chn_attr.depth = 1;
    chn_attr.stride = 0;
    ret = FH_VPSS_SetChnAttr(GROUP_ID, 0, &chn_attr);  //����VPU��ͼ����ߣ��ɷŴ���С
    CHECK_RET(ret != 0, ret);

    ret = FH_VPSS_SetVOMode(GROUP_ID, 0, VPU_VOMODE_SCAN);
    CHECK_RET(ret != 0, ret);

	ret = FH_VPSS_OpenChn(GROUP_ID, 0);
	CHECK_RET(ret != 0, ret);

	ret = sampe_set_venc_cfg(0, 1280, 720);
	CHECK_RET(ret != 0, ret);

	// 通道1是 768*448
	FH_VPU_CHN_CONFIG chn_attr_1;
    chn_attr_1.vpu_chn_size.u32Width  = 768;
    chn_attr_1.vpu_chn_size.u32Height = 448;
	chn_attr_1.crop_area.crop_en = 0;
    chn_attr_1.crop_area.vpu_crop_area.u32X = 0;
    chn_attr_1.crop_area.vpu_crop_area.u32Y = 0;
    chn_attr_1.crop_area.vpu_crop_area.u32Width = 0;
    chn_attr_1.crop_area.vpu_crop_area.u32Height = 0;
    chn_attr_1.offset = 0;
    chn_attr_1.depth = 1;
    chn_attr_1.stride = 0;
    ret = FH_VPSS_SetChnAttr(GROUP_ID, 1, &chn_attr_1);  //����VPU��ͼ����ߣ��ɷŴ���С
    CHECK_RET(ret != 0, ret);

    ret = FH_VPSS_SetVOMode(GROUP_ID, 1, VPU_VOMODE_SCAN); // ke yi she zhi shi pin xuan zhuan
    CHECK_RET(ret != 0, ret);

	ret = FH_VPSS_OpenChn(GROUP_ID, 1);
	CHECK_RET(ret != 0, ret);

	ret = sampe_set_venc_cfg(1, 768, 448);
	CHECK_RET(ret != 0, ret);

	// 获取通道属性，实时显示码率
	pthread_t tid_bitrate;
	pthread_create(&tid_bitrate, NULL, thread_bitrate_function, NULL);


	FH_BIND_INFO src, dst;
	src.obj_id = FH_OBJ_ISP;
    src.dev_id = 0;
    src.chn_id = 0;
    dst.obj_id = FH_OBJ_VPU_VI;
    dst.dev_id = 0;
    dst.chn_id = 0;
    ret = FH_SYS_Bind(src, dst);
	CHECK_RET(ret != 0, ret);

	// 设置两个通道，开启接收图像数据
	ret = FH_VENC_StartRecvPic(0);
	CHECK_RET(ret != 0, ret);
	ret = FH_VENC_StartRecvPic(1);
	CHECK_RET(ret != 0, ret);
	// 通道0
    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = GROUP_ID;
    src.chn_id = 0;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = 0;
	
    ret = FH_SYS_Bind(src, dst);
	CHECK_RET(ret != 0, ret);

	// 通道1
	src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = GROUP_ID;
    src.chn_id = 1;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = 1;
	
    ret = FH_SYS_Bind(src, dst);
	CHECK_RET(ret != 0, ret);
	// 初始化两个通道以及对应端口
	sample_dmc_init(dst_ip, port, 2); /////////////////////// max channel no 2


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
#if 1
	ret = sample_set_osd();
	CHECK_RET(ret != 0, ret);
#endif

	isp_set_param(ISP_AE, 1);  //�����Զ��ع�

	isp_set_param(ISP_AWB, 1); //�����Զ���ƽ��

	isp_set_param(ISP_COLOR, 75); //25����ͼ�񱥺Ͷ�

	isp_set_param(ISP_BRIGHT, 175); //125��������

	isp_set_param(ISP_NR, 1); //���ý���

	// 本来是0，修改为1，代表水平翻转；按键翻转该变量
	isp_set_param(ISP_MF, 0);
	pthread_t tid2;
	pthread_create(&tid2, NULL, thread_key2_function, (void*)button_fd);
#if 1
	/*// 马赛克 
	FH_VPU_MASK stVpumaskinfo1;  //����������
	memset(&stVpumaskinfo1,0x0,sizeof(FH_VPU_MASK));

	stVpumaskinfo1.mask_enable[0] = 1;
	stVpumaskinfo1.area_value[0].u32X = 0;
	stVpumaskinfo1.area_value[0].u32Y = 400;
	stVpumaskinfo1.area_value[0].u32Width = 300; 
	stVpumaskinfo1.area_value[0].u32Height = 300;
	stVpumaskinfo1.masaic.masaic_enable = 1;
	stVpumaskinfo1.masaic.masaic_size = 1;
	//stVpumaskinfo1.color = 0x000000ff;
	ret = FH_VPSS_SetMask(0, &stVpumaskinfo1);
	CHECK_RET(ret != 0, ret);*/


	// osd颜色板子
	FH_VPU_MASK stVpumaskinfo;  //����������
	memset(&stVpumaskinfo,0x0,sizeof(FH_VPU_MASK));

	stVpumaskinfo.mask_enable[0] = 1;
	stVpumaskinfo.area_value[0].u32X = 0;
	stVpumaskinfo.area_value[0].u32Y = 0;
	stVpumaskinfo.area_value[0].u32Width = 300; 
	stVpumaskinfo.area_value[0].u32Height = 300; 

	int j = 0;
	while (1) {
		if (j % 6 == 0)
			stVpumaskinfo.color = 0x000000ff;	// 红
		if (j % 6 == 1)
			stVpumaskinfo.color = 0x00ff0000;	// 绿
		if (j % 6 == 2)
			stVpumaskinfo.color = 0x0000ff00;	// 蓝
		if (j % 6 == 3)
			stVpumaskinfo.color = 0xffda059a;	// 黄
		if (j % 6 == 4)
			stVpumaskinfo.color = 0xff008080;	// 黑
		if (j % 6 == 5)
			stVpumaskinfo.color = 0xffff8080;	// 白
		j++;
		if (j == 50000)
			j = 0;
		sleep(1);
		ret = FH_VPSS_SetMask(0, &stVpumaskinfo);
		CHECK_RET(ret != 0, ret);
	}
	
	// 0xffda059a黄色；0xff008080黑色；0xffff8080白色；0xff7abe29蓝色；
	// 0x00ff0000绿色；0x0000ff00深蓝色；0x000000ff红色

#endif
#if 0
	sleep(2);
	ret = sampe_set_jpeg_cfg(1, 3840, 2160, 40);  //ץͼ
	CHECK_RET(ret != 0, ret);

	ret = sampe_set_jpeg_cfg(1, 3840, 2160, 80);
	CHECK_RET(ret != 0, ret);
#endif
    while (!g_sig_stop)
    {
        usleep(20000);
    }

    return 0;
}

