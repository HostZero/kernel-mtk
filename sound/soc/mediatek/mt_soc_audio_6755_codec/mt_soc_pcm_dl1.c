/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   mt_soc_pcm_afe.c
 *
 * Project:
 * --------
 *    Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio dl1 data1 playback
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"
#include "AudDrv_Kernel.h"
#include "mt_soc_afe_control.h"
#include "mt_soc_digital_type.h"
#include "mt_soc_pcm_common.h"
#include "AudDrv_Gpio.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/mutex.h>
/*#include <mach/irqs.h>*/
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
/*#include <mach/mt_reg_base.h>*/
#include <asm/div64.h>
/*
#include <linux/aee.h>
#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>
#include <mach/mt_gpio.h>*/
/*#include <mach/mt_typedefs.h>*/
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
/* #include <asm/mach-types.h> */

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

static unsigned int pin_extspkamp, pin_extspkamp_2, pin_vowclk, pin_audclk, pin_audmiso,
	pin_audmosi, pin_i2s1clk, pin_i2s1dat, pin_i2s1mclk, pin_i2s1ws, pin_rcvspkswitch,
	pin_hpswitchtoground;
static unsigned int pin_mode_audclk, pin_mode_audmosi, pin_mode_audmiso, pin_mode_vowclk,
	pin_mode_extspkamp, pin_mode_extspkamp_2, pin_mode_i2s1clk, pin_mode_i2s1dat, pin_mode_i2s1mclk,
	pin_mode_i2s1ws, pin_mode_rcvspkswitch, pin_mode_hpswitchtoground;
static unsigned int if_config1, if_config2, if_config3, if_config4, if_config5, if_config6,
	if_config7, if_config8, if_config9, if_config10, if_config11, if_config12;
#endif

static AFE_MEM_CONTROL_T *pMemControl;
static int mPlaybackSramState;
static struct snd_dma_buffer *Dl1_Playback_dma_buf;

static DEFINE_SPINLOCK(auddrv_DLCtl_lock);

static struct device *mDev;

/*
 *    function implementation
 */

/*void StartAudioPcmHardware(void);*/
/*void StopAudioPcmHardware(void);*/
static int mtk_soc_dl1_probe(struct platform_device *pdev);
static int mtk_soc_pcm_dl1_close(struct snd_pcm_substream *substream);
static int mtk_asoc_pcm_dl1_new(struct snd_soc_pcm_runtime *rtd);
static int mtk_asoc_dl1_probe(struct snd_soc_platform *platform);

static bool mPrepareDone;

#define USE_RATE        (SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000)
#define USE_RATE_MIN        8000
#define USE_RATE_MAX        192000
#define USE_CHANNELS_MIN     1
#define USE_CHANNELS_MAX    2
#define USE_PERIODS_MIN     512
#define USE_PERIODS_MAX     8192

static struct snd_pcm_hardware mtk_pcm_dl1_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_RESUME | SNDRV_PCM_INFO_MMAP_VALID),
	.formats = SND_SOC_ADV_MT_FMTS,
	.rates = SOC_HIGH_USE_RATE,
	.rate_min = SOC_HIGH_USE_RATE_MIN,
	.rate_max = SOC_HIGH_USE_RATE_MAX,
	.channels_min = SOC_NORMAL_USE_CHANNELS_MIN,
	.channels_max = SOC_NORMAL_USE_CHANNELS_MAX,
	.buffer_bytes_max = Dl1_MAX_BUFFER_SIZE,
	.period_bytes_max = Dl1_MAX_PERIOD_SIZE,
	.periods_min = SOC_NORMAL_USE_PERIODS_MIN,
	.periods_max = SOC_NORMAL_USE_PERIODS_MAX,
	.fifo_size = 0,
};

static int mtk_pcm_dl1_stop(struct snd_pcm_substream *substream)
{
	pr_warn("%s\n", __func__);
#ifdef AUDIO_FPGA_EARLYPORTING
	Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0000, 0xffff);	/* bit0, Turn off down-link */
	Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0040, 0x0040);	/* down-link power down */
#endif

	//SetIrqEnable(Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE, false);
	irq_remove_user(substream, Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE);
	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL1, false);

	/* here start digital part */
	SetConnection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I05,
		      Soc_Aud_InterConnectionOutput_O03);
	SetConnection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I06,
		      Soc_Aud_InterConnectionOutput_O04);

	ClearMemBlock(Soc_Aud_Digital_Block_MEM_DL1);
	return 0;
}

static snd_pcm_uframes_t mtk_pcm_pointer(struct snd_pcm_substream *substream)
{
	kal_int32 HW_memory_index = 0;
	kal_int32 HW_Cur_ReadIdx = 0;
	kal_uint32 Frameidx = 0;
	kal_int32 Afe_consumed_bytes = 0;
	AFE_BLOCK_T *Afe_Block = &pMemControl->rBlock;
	/* struct snd_pcm_runtime *runtime = substream->runtime; */
	PRINTK_AUD_DL1(" %s Afe_Block->u4DMAReadIdx = 0x%x\n", __func__, Afe_Block->u4DMAReadIdx);

	Auddrv_Dl1_Spinlock_lock();

	/* get total bytes to copy */
	/* Frameidx = audio_bytes_to_frame(substream , Afe_Block->u4DMAReadIdx); */
	/* return Frameidx; */

	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL1) == true) {
		HW_Cur_ReadIdx = Afe_Get_Reg(AFE_DL1_CUR);
		if (HW_Cur_ReadIdx == 0) {
			PRINTK_AUDDRV("[Auddrv] HW_Cur_ReadIdx ==0\n");
			HW_Cur_ReadIdx = Afe_Block->pucPhysBufAddr;
		}

		HW_memory_index = (HW_Cur_ReadIdx - Afe_Block->pucPhysBufAddr);
		if (HW_memory_index >= Afe_Block->u4DMAReadIdx) {
			Afe_consumed_bytes = HW_memory_index - Afe_Block->u4DMAReadIdx;
		} else {
			Afe_consumed_bytes =
			    Afe_Block->u4BufferSize + HW_memory_index - Afe_Block->u4DMAReadIdx;
		}

		Afe_consumed_bytes = Align64ByteSize(Afe_consumed_bytes);

		Afe_Block->u4DataRemained -= Afe_consumed_bytes;
		Afe_Block->u4DMAReadIdx += Afe_consumed_bytes;
		Afe_Block->u4DMAReadIdx %= Afe_Block->u4BufferSize;
		PRINTK_AUD_DL1
		    ("[Auddrv] HW_Cur_ReadIdx =0x%x HW_memory_index = 0x%x Afe_consumed_bytes  = 0x%x\n",
		     HW_Cur_ReadIdx, HW_memory_index, Afe_consumed_bytes);
		Auddrv_Dl1_Spinlock_unlock();

		return audio_bytes_to_frame(substream, Afe_Block->u4DMAReadIdx);
	}

	Frameidx = audio_bytes_to_frame(substream, Afe_Block->u4DMAReadIdx);
	Auddrv_Dl1_Spinlock_unlock();
	return Frameidx;
}

static void SetDL1Buffer(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *hw_params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	AFE_BLOCK_T *pblock = &pMemControl->rBlock;

	pblock->pucPhysBufAddr = runtime->dma_addr;
	pblock->pucVirtBufAddr = runtime->dma_area;
	pblock->u4BufferSize = runtime->dma_bytes;
	pblock->u4SampleNumMask = 0x001f;	/* 32 byte align */
	pblock->u4WriteIdx = 0;
	pblock->u4DMAReadIdx = 0;
	pblock->u4DataRemained = 0;
	pblock->u4fsyncflag = false;
	pblock->uResetFlag = true;
	pr_warn("SetDL1Buffer u4BufferSize = %d pucVirtBufAddr = %p pucPhysBufAddr = 0x%x\n",
	       pblock->u4BufferSize, pblock->pucVirtBufAddr, pblock->pucPhysBufAddr);
	/* set dram address top hardware */
	Afe_Set_Reg(AFE_DL1_BASE, pblock->pucPhysBufAddr, 0xffffffff);
	Afe_Set_Reg(AFE_DL1_END, pblock->pucPhysBufAddr + (pblock->u4BufferSize - 1), 0xffffffff);
	memset_io((void *)pblock->pucVirtBufAddr, 0, pblock->u4BufferSize);

}

static int mtk_pcm_dl1_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *hw_params)
{
	/* struct snd_dma_buffer *dma_buf = &substream->dma_buffer; */
	int ret = 0;

	PRINTK_AUDDRV("mtk_pcm_dl1_params\n");

	/* runtime->dma_bytes has to be set manually to allow mmap */
	substream->runtime->dma_bytes = params_buffer_bytes(hw_params);

	if (mPlaybackSramState == SRAM_STATE_PLAYBACKFULL) {
		/* substream->runtime->dma_bytes = AFE_INTERNAL_SRAM_SIZE; */
		substream->runtime->dma_area = (unsigned char *)Get_Afe_SramBase_Pointer();
		substream->runtime->dma_addr = AFE_INTERNAL_SRAM_PHY_BASE;
		SetHighAddr(Soc_Aud_Digital_Block_MEM_DL1, false);
		AudDrv_Allocate_DL1_Buffer(mDev, substream->runtime->dma_bytes);
	} else {
		substream->runtime->dma_bytes = params_buffer_bytes(hw_params);
		substream->runtime->dma_area = Dl1_Playback_dma_buf->area;
		substream->runtime->dma_addr = Dl1_Playback_dma_buf->addr;
		SetHighAddr(Soc_Aud_Digital_Block_MEM_DL1, true);
		SetDL1Buffer(substream, hw_params);
	}

	PRINTK_AUDDRV("dma_bytes = %zu dma_area = %p dma_addr = 0x%lx\n",
		      substream->runtime->dma_bytes, substream->runtime->dma_area,
		      (long)substream->runtime->dma_addr);
	return ret;
}

static int mtk_pcm_dl1_hw_free(struct snd_pcm_substream *substream)
{
	PRINTK_AUDDRV("mtk_pcm_dl1_hw_free\n");
	return 0;
}


static struct snd_pcm_hw_constraint_list constraints_sample_rates = {
	.count = ARRAY_SIZE(soc_high_supported_sample_rates),
	.list = soc_high_supported_sample_rates,
	.mask = 0,
};

static int mtk_pcm_dl1_open(struct snd_pcm_substream *substream)
{
	int ret = 0;

	struct snd_pcm_runtime *runtime = substream->runtime;

	PRINTK_AUDDRV("mtk_pcm_dl1_open\n");

	AfeControlSramLock();
#ifdef AUDIO_FPGA_EARLYPORTING
	SetSramState(SRAM_STATE_PLAYBACKDRAM);
#endif
	if (GetSramState() == SRAM_STATE_FREE) {
		mtk_pcm_dl1_hardware.buffer_bytes_max = GetPLaybackSramFullSize();
		mPlaybackSramState = SRAM_STATE_PLAYBACKFULL;
		SetSramState(mPlaybackSramState);
	} else {
		mtk_pcm_dl1_hardware.buffer_bytes_max = GetPLaybackDramSize();
		mPlaybackSramState = SRAM_STATE_PLAYBACKDRAM;
	}
	AfeControlSramUnLock();
	if (mPlaybackSramState == SRAM_STATE_PLAYBACKDRAM)
		AudDrv_Emi_Clk_On();

	pr_warn("mtk_pcm_dl1_hardware.buffer_bytes_max = %zu mPlaybackSramState = %d\n",
	       mtk_pcm_dl1_hardware.buffer_bytes_max, mPlaybackSramState);
	runtime->hw = mtk_pcm_dl1_hardware;

	AudDrv_Clk_On();
	memcpy((void *)(&(runtime->hw)), (void *)&mtk_pcm_dl1_hardware,
	       sizeof(struct snd_pcm_hardware));
	pMemControl = Get_Mem_ControlT(Soc_Aud_Digital_Block_MEM_DL1);

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &constraints_sample_rates);

	if (ret < 0)
		pr_err("snd_pcm_hw_constraint_integer failed\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		pr_warn("SNDRV_PCM_STREAM_PLAYBACK mtkalsa_dl1playback_constraints\n");

	if (ret < 0) {
		pr_err("ret < 0 mtk_soc_pcm_dl1_close\n");
		mtk_soc_pcm_dl1_close(substream);
		return ret;
	}
	/* PRINTK_AUDDRV("mtk_pcm_dl1_open return\n"); */
	return 0;
}

static int mtk_soc_pcm_dl1_close(struct snd_pcm_substream *substream)
{
	pr_warn("%s\n", __func__);

	if (mPrepareDone == true) {
		/* stop DAC output */
		SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, false);
		if (GetI2SDacEnable() == false)
			SetI2SDacEnable(false);

		RemoveMemifSubStream(Soc_Aud_Digital_Block_MEM_DL1, substream);

		EnableAfe(false);
		mPrepareDone = false;
	}

	if (mPlaybackSramState == SRAM_STATE_PLAYBACKDRAM)
		AudDrv_Emi_Clk_Off();

	AfeControlSramLock();
	ClearSramState(mPlaybackSramState);
	mPlaybackSramState = GetSramState();
	AfeControlSramUnLock();
	AudDrv_Clk_Off();
	return 0;
}

static int mtk_pcm_prepare(struct snd_pcm_substream *substream)
{
	bool mI2SWLen;
	struct snd_pcm_runtime *runtime = substream->runtime;

	if (mPrepareDone == false) {
		pr_warn
		    ("%s format = %d SNDRV_PCM_FORMAT_S32_LE = %d SNDRV_PCM_FORMAT_U32_LE = %d\n",
		     __func__, runtime->format, SNDRV_PCM_FORMAT_S32_LE, SNDRV_PCM_FORMAT_U32_LE);
		SetMemifSubStream(Soc_Aud_Digital_Block_MEM_DL1, substream);

		if (runtime->format == SNDRV_PCM_FORMAT_S32_LE
		    || runtime->format == SNDRV_PCM_FORMAT_U32_LE) {
			SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL1,
						     AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA);
			SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_24BIT,
						  Soc_Aud_InterConnectionOutput_O03);
			SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_24BIT,
						  Soc_Aud_InterConnectionOutput_O04);
			mI2SWLen = Soc_Aud_I2S_WLEN_WLEN_32BITS;
		} else {
			SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL1,
						     AFE_WLEN_16_BIT);
			SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_16BIT,
						  Soc_Aud_InterConnectionOutput_O03);
			SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_16BIT,
						  Soc_Aud_InterConnectionOutput_O04);
			mI2SWLen = Soc_Aud_I2S_WLEN_WLEN_16BITS;
		}

		SetSampleRate(Soc_Aud_Digital_Block_MEM_I2S, runtime->rate);

		/* start I2S DAC out */
		if (GetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC) == false) {
			SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, true);
			SetI2SDacOut(substream->runtime->rate, false, mI2SWLen);
			SetI2SDacEnable(true);
		} else {
			SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, true);
		}
		/* here to set interrupt_distributor */
	//	SetIrqMcuCounter(Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE, runtime->period_size);
	//	SetIrqMcuSampleRate(Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE, runtime->rate);

		EnableAfe(true);
		mPrepareDone = true;
	}
	return 0;
}


static int mtk_pcm_dl1_start(struct snd_pcm_substream *substream)
{

#ifdef AUDIO_FPGA_EARLYPORTING
	uint32 Reg_value;
#endif

	struct snd_pcm_runtime *runtime = substream->runtime;

	pr_warn("%s\n", __func__);
	/* here start digital part */

	SetConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I05,
		      Soc_Aud_InterConnectionOutput_O03);
	SetConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I06,
		      Soc_Aud_InterConnectionOutput_O04);

//	SetIrqEnable(Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE, true);

	/* here to set interrupt */
	irq_add_user(substream,
		     Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE,
		     substream->runtime->rate,
	     substream->runtime->period_size);

	SetSampleRate(Soc_Aud_Digital_Block_MEM_DL1, runtime->rate);
	SetChannels(Soc_Aud_Digital_Block_MEM_DL1, runtime->channels);
	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL1, true);

	EnableAfe(true);

#ifdef AUDIO_FPGA_EARLYPORTING	/* ccc early porting, copy from TurnOnDacPower() and ADC_LOOP_DAC_Func() */
	pr_warn("%s pcm loopback_b\n", __func__);

	Ana_Set_Reg(TOP_CLKSQ, 0x0001, 0x0001);	/* Enable CLKSQ 26MHz */
	Ana_Set_Reg(TOP_CLKSQ_SET, 0x0001, 0x0001);	/* Turn on 26MHz source clock */
	Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x7000, 0x7000);

	Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);	/* power on clock */

	Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffffffff);
	Ana_Set_Reg(AFUNC_AUD_CON0, 0xc3a1, 0xffffffff);	/* sdm audio fifo clock power on */
	Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffffffff);	/* sdm power on */
	Ana_Set_Reg(AFUNC_AUD_CON2, 0x000b, 0xffffffff);	/* sdm fifo enable */
	Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001e, 0xffffffff);	/* set attenuation gain */
	Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffffffff);	/* [0] afe enable */

	/* Ana_Set_Reg(AFE_UL_SRC0_CON0_H, 0x0000 , 0x0010); // UL1 */
	/* Ana_Set_Reg(AFE_UL_SRC0_CON0_L, 0x0001, 0xffff);   //power on uplink */
	/* Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0, 0x0380, 0xffff); //MTKIF */
	/* Ana_Set_Reg(AFE_DL_SRC2_CON0_H, 0x0800, 0xffff);   //DL */
	/* Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0001, 0xffff); //DL */
	switch (runtime->rate) {
	case 8000:
		Reg_value = 0;
		break;
	case 11025:
		Reg_value = 1;
		break;
	case 12000:
		Reg_value = 2;
		break;
	case 16000:
		Reg_value = 3;
		break;
	case 22050:
		Reg_value = 4;
		break;
	case 24000:
		Reg_value = 5;
		break;
	case 32000:
		Reg_value = 6;
		break;
	case 44100:
		Reg_value = 7;
		break;
	case 48000:
		Reg_value = 8;
		break;
	default:
		Reg_value = 7;
		pr_warn("WRONG: ApplyDLNewIFFrequency with frequency = %d\n", runtime->rate);
	}

	Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0, (Reg_value << 12 | 0x330), 0xffff);
	Ana_Set_Reg(AFE_DL_SRC2_CON0_H, (Reg_value << 12 | 0x300), 0xffff);

	/* Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0 , 0 << 12 | 0x330 , 0xffffffff); //8k sample rate */
	/* Ana_Set_Reg(AFE_DL_SRC2_CON0_H , 0 << 12 | 0x300 , 0xffffffff);//8k sample rate */
	Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0001, 0xffffffff);	/* turn off mute function and turn on dl */
	Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffffffff);	/* set DL in normal path, not from sine gen table */
	Afe_Set_Reg(FPGA_CFG1, 0x1, 0xffff);	/* must set in FPGA platform for PMIC digital loopback */

	pr_warn("%s pcm loopback_e\n", __func__);
#endif


	return 0;
}

static int mtk_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	PRINTK_AUDDRV("mtk_pcm_trigger cmd = %d\n", cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		return mtk_pcm_dl1_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return mtk_pcm_dl1_stop(substream);
	}
	return -EINVAL;
}

static int mtk_pcm_copy(struct snd_pcm_substream *substream,
			int channel, snd_pcm_uframes_t pos,
			void __user *dst, snd_pcm_uframes_t count)
{
	AFE_BLOCK_T *Afe_Block = NULL;
	int copy_size = 0, Afe_WriteIdx_tmp;
	unsigned long flags;
	/* struct snd_pcm_runtime *runtime = substream->runtime; */
	char *data_w_ptr = (char *)dst;

	PRINTK_AUD_DL1("mtk_pcm_copy pos = %lu count = %lu\n ", pos, count);
	/* get total bytes to copy */
	count = audio_frame_to_bytes(substream, count);

	/* check which memif nned to be write */
	Afe_Block = &pMemControl->rBlock;

	PRINTK_AUD_DL1("AudDrv_write WriteIdx=0x%x, ReadIdx=0x%x, DataRemained=0x%x\n",
		       Afe_Block->u4WriteIdx, Afe_Block->u4DMAReadIdx, Afe_Block->u4DataRemained);

	if (Afe_Block->u4BufferSize == 0) {
		pr_err("AudDrv_write: u4BufferSize=0 Error");
		return 0;
	}

	AudDrv_checkDLISRStatus();

	spin_lock_irqsave(&auddrv_DLCtl_lock, flags);
	copy_size = Afe_Block->u4BufferSize - Afe_Block->u4DataRemained;	/* free space of the buffer */
	spin_unlock_irqrestore(&auddrv_DLCtl_lock, flags);
	if (count <= copy_size) {
		if (copy_size < 0)
			copy_size = 0;
		else
			copy_size = count;
	}

	copy_size = Align64ByteSize(copy_size);
	PRINTK_AUD_DL1("copy_size=0x%x, count=0x%x\n", copy_size, (unsigned int)count);

	if (copy_size != 0) {
		spin_lock_irqsave(&auddrv_DLCtl_lock, flags);
		Afe_WriteIdx_tmp = Afe_Block->u4WriteIdx;
		spin_unlock_irqrestore(&auddrv_DLCtl_lock, flags);

		/* copy once */
		if (Afe_WriteIdx_tmp + copy_size < Afe_Block->u4BufferSize) {

			if (!access_ok(VERIFY_READ, data_w_ptr, copy_size)) {
				PRINTK_AUDDRV("AudDrv_write 0ptr invalid data_w_ptr=%p, size=%d",
					      data_w_ptr, copy_size);
				PRINTK_AUDDRV("AudDrv_write u4BufferSize=%d, u4DataRemained=%d",
					      Afe_Block->u4BufferSize, Afe_Block->u4DataRemained);
			} else {
				PRINTK_AUD_DL1
				    ("memcpy VirtBufAddr+Afe_WriteIdx= %p,data_w_ptr = %p copy_size = 0x%x\n",
				     Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp, data_w_ptr,
				     copy_size);
				if (copy_from_user
				    ((Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp), data_w_ptr,
				     copy_size)) {
					PRINTK_AUDDRV("AudDrv_write Fail copy from user\n");
					return -1;
				}
			}

			spin_lock_irqsave(&auddrv_DLCtl_lock, flags);
			Afe_Block->u4DataRemained += copy_size;
			Afe_Block->u4WriteIdx = Afe_WriteIdx_tmp + copy_size;
			Afe_Block->u4WriteIdx %= Afe_Block->u4BufferSize;
			spin_unlock_irqrestore(&auddrv_DLCtl_lock, flags);
			data_w_ptr += copy_size;
			count -= copy_size;

			PRINTK_AUD_DL1
			    ("AudDrv_write finish1, copy:%x, WriteIdx:%x,ReadIdx=%x,Remained:%x, count=%d \r\n",
			     copy_size, Afe_Block->u4WriteIdx, Afe_Block->u4DMAReadIdx,
			     Afe_Block->u4DataRemained, (int)count);

		} else {
		/* copy twice */
			kal_uint32 size_1 = 0, size_2 = 0;

			size_1 = Align64ByteSize((Afe_Block->u4BufferSize - Afe_WriteIdx_tmp));
			size_2 = Align64ByteSize((copy_size - size_1));
			PRINTK_AUD_DL1("size_1=0x%x, size_2=0x%x\n", size_1, size_2);
			if (!access_ok(VERIFY_READ, data_w_ptr, size_1)) {
				pr_err("AudDrv_write 1ptr invalid data_w_ptr=%p, size_1=%d",
				       data_w_ptr, size_1);
				pr_err("AudDrv_write u4BufferSize=%d, u4DataRemained=%d",
				       Afe_Block->u4BufferSize, Afe_Block->u4DataRemained);
			} else {

				PRINTK_AUD_DL1
				    ("mcmcpy Afe_Block->pucVirtBufAddr+Afe_WriteIdx= %p data_w_ptr = %p size_1 = %x\n",
				     Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp, data_w_ptr,
				     size_1);
				if ((copy_from_user
				     ((Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp), data_w_ptr,
				      (unsigned int)size_1))) {
					PRINTK_AUDDRV("AudDrv_write Fail 1 copy from user");
					return -1;
				}
			}
			spin_lock_irqsave(&auddrv_DLCtl_lock, flags);
			Afe_Block->u4DataRemained += size_1;
			Afe_Block->u4WriteIdx = Afe_WriteIdx_tmp + size_1;
			Afe_Block->u4WriteIdx %= Afe_Block->u4BufferSize;
			Afe_WriteIdx_tmp = Afe_Block->u4WriteIdx;
			spin_unlock_irqrestore(&auddrv_DLCtl_lock, flags);

			if (!access_ok(VERIFY_READ, data_w_ptr + size_1, size_2)) {
				PRINTK_AUDDRV
				    ("AudDrv_write 2ptr invalid data_w_ptr=%p, size_1=%d, size_2=%d",
				     data_w_ptr, size_1, size_2);
				PRINTK_AUDDRV("AudDrv_write u4BufferSize=%d, u4DataRemained=%d",
					      Afe_Block->u4BufferSize, Afe_Block->u4DataRemained);
			} else {

				PRINTK_AUD_DL1
				    ("mcmcpy VirtBufAddr+Afe_WriteIdx= %p,data_w_ptr+size_1 = %p size_2 = %x\n",
				     Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp,
				     data_w_ptr + size_1, (unsigned int)size_2);
				if ((copy_from_user
				     ((Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp),
				      (data_w_ptr + size_1), size_2))) {
					PRINTK_AUDDRV("AudDrv_write Fail 2  copy from user");
					return -1;
				}
			}
			spin_lock_irqsave(&auddrv_DLCtl_lock, flags);

			Afe_Block->u4DataRemained += size_2;
			Afe_Block->u4WriteIdx = Afe_WriteIdx_tmp + size_2;
			Afe_Block->u4WriteIdx %= Afe_Block->u4BufferSize;
			spin_unlock_irqrestore(&auddrv_DLCtl_lock, flags);
			count -= copy_size;
			data_w_ptr += copy_size;

			PRINTK_AUD_DL1
			    ("AudDrv_write finish2, copy size:%x, WriteIdx:%x,ReadIdx=%x DataRemained:%x \r\n",
			     copy_size, Afe_Block->u4WriteIdx, Afe_Block->u4DMAReadIdx,
			     Afe_Block->u4DataRemained);
		}
	}
	return 0;
}

static int mtk_pcm_silence(struct snd_pcm_substream *substream,
			   int channel, snd_pcm_uframes_t pos, snd_pcm_uframes_t count)
{
	PRINTK_AUDDRV("%s\n", __func__);
	return 0;		/* do nothing */
}

static void *dummy_page[2];

static struct page *mtk_pcm_page(struct snd_pcm_substream *substream, unsigned long offset)
{
	PRINTK_AUDDRV("%s\n", __func__);
	return virt_to_page(dummy_page[substream->stream]);	/* the same page */
}

static struct snd_pcm_ops mtk_afe_ops = {
	.open = mtk_pcm_dl1_open,
	.close = mtk_soc_pcm_dl1_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = mtk_pcm_dl1_params,
	.hw_free = mtk_pcm_dl1_hw_free,
	.prepare = mtk_pcm_prepare,
	.trigger = mtk_pcm_trigger,
	.pointer = mtk_pcm_pointer,
	.copy = mtk_pcm_copy,
	.silence = mtk_pcm_silence,
	.page = mtk_pcm_page,
};

static struct snd_soc_platform_driver mtk_soc_platform = {
	.ops = &mtk_afe_ops,
	.pcm_new = mtk_asoc_pcm_dl1_new,
	.probe = mtk_asoc_dl1_probe,
};


static int mtk_asoc_pcm_dl1_new(struct snd_soc_pcm_runtime *rtd)
{
	int ret = 0;

	PRINTK_AUDDRV("%s\n", __func__);
	return ret;
}


static int mtk_asoc_dl1_probe(struct snd_soc_platform *platform)
{
	PRINTK_AUDDRV("mtk_asoc_dl1_probe\n");
	/* allocate dram */
	AudDrv_Allocate_mem_Buffer(platform->dev, Soc_Aud_Digital_Block_MEM_DL1,
				   Dl1_MAX_BUFFER_SIZE);
	Dl1_Playback_dma_buf = Get_Mem_Buffer(Soc_Aud_Digital_Block_MEM_DL1);
	return 0;
}

static int mtk_afe_remove(struct platform_device *pdev)
{
	PRINTK_AUDDRV("%s\n", __func__);
#ifndef CONFIG_MTK_CLKMGR
	AudDrv_Clk_Deinit(&pdev->dev);
#endif
	snd_soc_unregister_platform(&pdev->dev);

	return 0;
}

#ifdef CONFIG_OF
/*extern void *AFE_BASE_ADDRESS;*/
u32 afe_irq_number = 0;
int AFE_BASE_PHY;

static const struct of_device_id mt_soc_pcm_dl1_of_ids[] = {
	{.compatible = "mediatek,mt_soc_pcm_dl1",},
	{}
};

static int Auddrv_Reg_map_new(void *dev)
{
	struct device *pdev = dev;

	if (!pdev->of_node) {
		pr_err("%s invalid of_node\n", __func__);
		return -ENODEV;
	}

	/*get afe irq num */
	afe_irq_number = irq_of_parse_and_map(pdev->of_node, 0);

	pr_warn("[ge_mt_soc_pcm_dl1] afe_irq_number=%d\n", afe_irq_number);

	if (!afe_irq_number) {
		pr_err("[ge_mt_soc_pcm_dl1] get afe_irq_number failed!!!\n");
		return -ENODEV;
	}

	if (pdev->of_node) {
		/* Setup IO addresses */
		AFE_BASE_ADDRESS = of_iomap(pdev->of_node, 0);
		pr_warn("[ge_mt_soc_pcm_dl1] AFE_BASE_ADDRESS=0x%p\n", AFE_BASE_ADDRESS);
	} else {
		pr_err("[mt_soc_pcm_dl1] node NULL, can't iomap AFE_BASE!!!\n");
		BUG();
		return -ENODEV;
	}

	if (pdev->of_node) {
		/* Setup IO addresses */
		of_property_read_u32(pdev->of_node, "reg", &AFE_BASE_PHY);
		pr_warn("[ge_mt_soc_pcm_dl1] AFE_BASE_PHY=0x%x\n", AFE_BASE_PHY);
	} else {
		pr_err("[mt_soc_pcm_dl1] node NULL, can't iomap AFE_BASE_PHY!!!\n");
		BUG();
		return -ENODEV;
	}


	return 0;
}

#ifdef CONFIG_MTK_LEGACY

static int Auddrv_OF_ParseGPIO(void *dev)
{
	/* struct device_node *node = NULL; */
	struct device *pdev = dev;

	if (!pdev->of_node) {
		pr_err("%s invalid of_node\n", __func__);
		return -ENODEV;
	}

	/* node = of_find_compatible_node(NULL, NULL, "mediatek,mt-soc-dl1-pcm"); */
	if (pdev->of_node) {
		if_config1 = 1;
		if_config2 = 1;
		if_config3 = 1;
		if_config4 = 1;
		if_config5 = 1;
		if_config6 = 1;
		if_config7 = 1;
		if_config8 = 1;
		if_config9 = 1;
		if_config10 = 1;
		if_config11 = 1;
		if_config12 = 1;

		if (of_property_read_u32_index(pdev->of_node, "audclk-gpio", 0, &pin_audclk)) {
			if_config1 = 0;
			pr_err("audclk-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index(pdev->of_node, "audclk-gpio", 1, &pin_mode_audclk)) {
			if_config1 = 0;
			pr_err("audclk-gpio get pin_mode fail!!!\n");
		}

		if (of_property_read_u32_index(pdev->of_node, "audmiso-gpio", 0, &pin_audmiso)) {
			if_config2 = 0;
			pr_err("audmiso-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index(pdev->of_node, "audmiso-gpio", 1, &pin_mode_audmiso)) {
			if_config2 = 0;
			pr_err("audmiso-gpio get pin_mode fail!!!\n");
		}

		if (of_property_read_u32_index(pdev->of_node, "audmosi-gpio", 0, &pin_audmosi)) {
			if_config3 = 0;
			pr_err("audmosi-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index(pdev->of_node, "audmosi-gpio", 1, &pin_mode_audmosi)) {
			if_config3 = 0;
			pr_err("audmosi-gpio get pin_mode fail!!!\n");
		}

		if (of_property_read_u32_index(pdev->of_node, "vowclk-gpio", 0, &pin_vowclk)) {
			if_config4 = 0;
			pr_err("vowclk-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index(pdev->of_node, "vowclk-gpio", 1, &pin_mode_vowclk)) {
			if_config4 = 0;
			pr_err("vowclk-gpio get pin_mode fail!!!\n");
		}

		if (of_property_read_u32_index(pdev->of_node, "extspkamp-gpio", 0, &pin_extspkamp)) {
			if_config5 = 0;
			pr_err("extspkamp-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index(pdev->of_node, "extspkamp-gpio", 1, &pin_mode_extspkamp)) {
			if_config5 = 0;
			pr_err("extspkamp-gpio get pin_mode fail!!!\n");
		}

		if (of_property_read_u32_index(pdev->of_node, "i2s1clk-gpio", 0, &pin_i2s1clk)) {
			if_config6 = 0;
			pr_err("i2s1clk-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index(pdev->of_node, "i2s1clk-gpio", 1, &pin_mode_i2s1clk)) {
			if_config6 = 0;
			pr_err("i2s1clk-gpio get pin_mode fail!!!\n");
		}

		if (of_property_read_u32_index(pdev->of_node, "i2s1dat-gpio", 0, &pin_i2s1dat)) {
			if_config7 = 0;
			pr_err("i2s1dat-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index(pdev->of_node, "i2s1dat-gpio", 1, &pin_mode_i2s1dat)) {
			if_config7 = 0;
			pr_err("i2s1dat-gpio get pin_mode fail!!!\n");
		}

		if (of_property_read_u32_index(pdev->of_node, "i2s1mclk-gpio", 0, &pin_i2s1mclk)) {
			if_config8 = 0;
			pr_err("i2s1mclk-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index(pdev->of_node, "i2s1mclk-gpio", 1, &pin_mode_i2s1mclk)) {
			if_config8 = 0;
			pr_err("i2s1mclk-gpio get pin_mode fail!!!\n");
		}

		if (of_property_read_u32_index(pdev->of_node, "i2s1ws-gpio", 0, &pin_i2s1ws)) {
			if_config9 = 0;
			pr_err("i2s1ws-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index(pdev->of_node, "i2s1ws-gpio", 1, &pin_mode_i2s1ws)) {
			if_config9 = 0;
			pr_err("i2s1ws-gpio get pin_mode fail!!!\n");
		}

		if (of_property_read_u32_index(pdev->of_node, "extspkamp_2-gpio", 0, &pin_extspkamp_2)) {
			if_config10 = 0;
			pr_err("extspkamp_2-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index(pdev->of_node, "extspkamp_2-gpio", 1, &pin_mode_extspkamp_2)) {
			if_config10 = 0;
			pr_err("extspkamp_2-gpio get pin_mode fail!!!\n");
		}

		if (of_property_read_u32_index(pdev->of_node, "rcvspkswitch-gpio", 0, &pin_rcvspkswitch)) {
			if_config11 = 0;
			pr_err("rcvspkswitch-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index
		    (pdev->of_node, "rcvspkswitch-gpio", 1, &pin_mode_rcvspkswitch)) {
			if_config11 = 0;
			pr_err("rcvspkswitch-gpio get pin_mode fail!!!\n");
		}

		if (of_property_read_u32_index(pdev->of_node, "hpswitchtoground-gpio", 0, &pin_hpswitchtoground)) {
			if_config12 = 0;
			pr_err("hpswitchtoground-gpio get pin fail!!!\n");
		}
		if (of_property_read_u32_index
		    (pdev->of_node, "hpswitchtoground-gpio", 1, &pin_mode_hpswitchtoground)) {
			if_config12 = 0;
			pr_err("hpswitchtoground-gpio get pin_mode fail!!!\n");
		}
		/* Disable hpswitchtoground if pin = 0 */
		if (pin_hpswitchtoground == 0)
			if_config12 = 0;

		pr_warn("Auddrv_OF_ParseGPIO pin_audclk=%d, pin_audmiso=%d, pin_audmosi=%d\n",
			pin_audclk, pin_audmiso, pin_audmosi);
		pr_warn("Auddrv_OF_ParseGPIO pin_vowclk=%d, pin_extspkamp=%d\n", pin_vowclk,
			pin_extspkamp);
		pr_warn
		    ("Auddrv_OF_ParseGPIO pin_i2s1clk=%d, pin_i2s1dat=%d, pin_i2s1mclk=%d, pin_i2s1ws=%d\n",
		     pin_i2s1clk, pin_i2s1dat, pin_i2s1mclk, pin_i2s1ws);
	} else {
		pr_err("Auddrv_OF_ParseGPIO node NULL!!!\n");
		return -ENODEV;
	}
	return 0;
}
#endif

int GetGPIO_Info(int type, int *pin, int *pinmode)
{
	switch (type) {
	case 1:		/* pin_audclk */
		if (if_config1 == 1) {
			*pin = pin_audclk | 0x80000000;
			*pinmode = pin_mode_audclk;
		} else {
			pr_err("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;
		}
		break;

	case 2:		/* pin_audmiso */
		if (if_config2 == 1) {
			*pin = pin_audmiso | 0x80000000;
			*pinmode = pin_mode_audmiso;
		} else {
			pr_err("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;
		}
		break;

	case 3:		/* pin_audmosi */
		if (if_config3 == 1) {
			*pin = pin_audmosi | 0x80000000;
			*pinmode = pin_mode_audmosi;
		} else {
			pr_err("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;
		}
		break;

	case 4:		/* pin_vowclk */
		if (if_config4 == 1) {
			*pin = pin_vowclk | 0x80000000;
			*pinmode = pin_mode_vowclk;
		} else {
			pr_err("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;
		}
		break;

	case 5:		/* pin_extspkamp */
		if (if_config5 == 1) {
			*pin = pin_extspkamp | 0x80000000;
			*pinmode = pin_mode_extspkamp;
		} else {
			pr_err("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;
		}
		break;

	case 6:		/* pin_i2s1clk */
		if (if_config6 == 1) {
			*pin = pin_i2s1clk | 0x80000000;
			*pinmode = pin_mode_i2s1clk;
		} else {
			pr_err("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;
		}
		break;

	case 7:		/* pin_i2s1dat */
		if (if_config7 == 1) {
			*pin = pin_i2s1dat | 0x80000000;
			*pinmode = pin_mode_i2s1dat;
		} else {
			pr_err("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;
		}
		break;

	case 8:		/* pin_i2s1mclk */
		if (if_config8 == 1) {
			*pin = pin_i2s1mclk | 0x80000000;
			*pinmode = pin_mode_i2s1mclk;
		} else {
			pr_err("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;
		}
		break;

	case 9:		/* pin_i2s1ws */
		if (if_config9 == 1) {
			*pin = pin_i2s1ws | 0x80000000;
			*pinmode = pin_mode_i2s1ws;
		} else {
			pr_err("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;
		}
		break;

	case 10:		/* pin_extspkamp_2 */
		if (if_config10 == 1) {
			*pin = pin_extspkamp_2 | 0x80000000;
			*pinmode = pin_mode_extspkamp_2;
		} else {
			pr_debug("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;
		}
		break;

	case 11:		/* pin_rcvspkswitch */
		if (if_config11 == 1) {
			*pin = pin_rcvspkswitch | 0x80000000;
			*pinmode = pin_mode_rcvspkswitch;
		} else {
			pr_debug("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;
		}
		break;

	case 12:		/* pin_hpswitchtoground */
		if (if_config12 == 1) {
			*pin = pin_hpswitchtoground | 0x80000000;
			*pinmode = pin_mode_hpswitchtoground;
		} else {
			pr_debug("GetGPIO_Info type %d fail!!!\n", type);
			*pin = -1;
			*pinmode = -1;

			return -1;
		}
		break;

	default:
		*pin = -1;
		*pinmode = -1;
		pr_err("Auddrv_OF_ParseGPIO invalid type=%d!!!\n", type);
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(GetGPIO_Info);

#endif

static void DL1GlobalVarInit(void)
{
	pMemControl = NULL;

	mPlaybackSramState = 0;

	Dl1_Playback_dma_buf = NULL;

	mDev = NULL;

	mPrepareDone = false;

}

static int mtk_soc_dl1_probe(struct platform_device *pdev)
{
	int ret = 0;

	PRINTK_AUDDRV("%s\n", __func__);

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);

	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node) {
		dev_set_name(&pdev->dev, "%s", MT_SOC_DL1_PCM);
	} else {
		pr_err("%s invalid of_node\n", __func__);
		return -ENODEV;
	}

	pr_warn("%s: dev name %s\n", __func__, dev_name(&pdev->dev));

	DL1GlobalVarInit();

#ifdef CONFIG_OF

#ifndef CONFIG_MTK_CLKMGR
	AudDrv_Clk_probe(&pdev->dev);
#endif
	ret = Auddrv_Reg_map_new(&pdev->dev);
	if (ret) {
		BUG();
		return -ENODEV;
	}
	ret = Register_Aud_Irq(NULL, afe_irq_number);
	if (ret) {
		BUG();
		return -ENODEV;
	}

#ifndef CONFIG_MTK_LEGACY
	AudDrv_GPIO_probe(&pdev->dev);
#else
	ret = Auddrv_OF_ParseGPIO(&pdev->dev);
	if (ret) {
		BUG();
		return -ENODEV;
	}
#endif

#else
	ret = Register_Aud_Irq(&pdev->dev, MT6755_AFE_MCU_IRQ_LINE);
#endif

	ret = InitAfeControl();

	mDev = &pdev->dev;

	return snd_soc_register_platform(&pdev->dev, &mtk_soc_platform);
}


static struct platform_driver mtk_afe_driver = {
	.driver = {
		   .name = MT_SOC_DL1_PCM,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = mt_soc_pcm_dl1_of_ids,
#endif
		   },
	.probe = mtk_soc_dl1_probe,
	.remove = mtk_afe_remove,
};

#ifndef CONFIG_OF
static struct platform_device *soc_mtkafe_dev;
#endif

static int __init mtk_soc_platform_init(void)
{
	int ret;

	PRINTK_AUDDRV("%s\n", __func__);

#ifndef CONFIG_OF

	soc_mtkafe_dev = platform_device_alloc(MT_SOC_DL1_PCM, -1);

	if (!soc_mtkafe_dev)
		return -ENOMEM;


	ret = platform_device_add(soc_mtkafe_dev);

	if (ret != 0) {
		platform_device_put(soc_mtkafe_dev);
		return ret;
	}
#endif
	ret = platform_driver_register(&mtk_afe_driver);
	return ret;

}
module_init(mtk_soc_platform_init);

static void __exit mtk_soc_platform_exit(void)
{
	PRINTK_AUDDRV("%s\n", __func__);

	platform_driver_unregister(&mtk_afe_driver);
}
module_exit(mtk_soc_platform_exit);

MODULE_DESCRIPTION("AFE PCM module platform driver");
MODULE_LICENSE("GPL");
