/* Copyright (c) 2014-2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <reg.h>
#include <err.h>
#include <clock.h>
#include <clock_pll.h>
#include <clock_alpha_pll.h>
#include <clock_lib2.h>
#include <platform/clock.h>
#include <platform/iomap.h>


/* Mux source select values */
#define cxo_source_val    0
#define gpll0_source_val  1
#define gpll4_source_val  5
#define cxo_mm_source_val 0
#define mmpll0_mm_source_val 1
#define mmpll1_mm_source_val 2
#define mmpll3_mm_source_val 3
#define gpll0_mm_source_val 5
#define hdmipll_mm_source_val 1

struct clk_freq_tbl rcg_dummy_freq = F_END;


/* Clock Operations */
static struct clk_ops clk_ops_rst =
{
	.reset     = clock_lib2_reset_clk_reset,
};

static struct clk_ops clk_ops_branch =
{
	.enable     = clock_lib2_branch_clk_enable,
	.disable    = clock_lib2_branch_clk_disable,
	.set_rate   = clock_lib2_branch_set_rate,
	.reset      = clock_lib2_branch_clk_reset,
};

static struct clk_ops clk_ops_rcg_mnd =
{
	.enable     = clock_lib2_rcg_enable,
	.set_rate   = clock_lib2_rcg_set_rate,
};

static struct clk_ops clk_ops_rcg =
{
	.enable     = clock_lib2_rcg_enable,
	.set_rate   = clock_lib2_rcg_set_rate,
};

static struct clk_ops clk_ops_cxo =
{
	.enable     = cxo_clk_enable,
	.disable    = cxo_clk_disable,
};

static struct clk_ops clk_ops_pll_vote =
{
	.enable     = pll_vote_clk_enable,
	.disable    = pll_vote_clk_disable,
	.auto_off   = pll_vote_clk_disable,
	.is_enabled = pll_vote_clk_is_enabled,
};

static struct clk_ops clk_ops_vote =
{
	.enable     = clock_lib2_vote_clk_enable,
	.disable    = clock_lib2_vote_clk_disable,
};

static struct clk_ops clk_ops_fixed_alpha_pll =
{
	.enable = alpha_pll_enable,
	.disable = alpha_pll_disable,
};

/* Clock Sources */
static struct fixed_clk cxo_clk_src =
{
	.c = {
		.rate     = 19200000,
		.dbg_name = "cxo_clk_src",
		.ops      = &clk_ops_cxo,
	},
};

static struct pll_vote_clk gpll0_clk_src =
{
	.en_reg       = (void *) APCS_GPLL_ENA_VOTE,
	.en_mask      = BIT(0),
	.status_reg     = (void *) GPLL0_MODE,
	.status_mask    = BIT(30),
	.parent       = &cxo_clk_src.c,

	.c = {
		.rate     = 600000000,
		.dbg_name = "gpll0_clk_src",
		.ops      = &clk_ops_pll_vote,
	},
};

static struct pll_vote_clk gpll4_clk_src =
{
	.en_reg       = (void *) APCS_GPLL_ENA_VOTE,
	.en_mask      = BIT(4),
	.status_reg   = (void *) GPLL4_MODE,
	.status_mask  = BIT(30),
	.parent       = &cxo_clk_src.c,

	.c = {
		.rate     = 1600000000,
		.dbg_name = "gpll4_clk_src",
		.ops      = &clk_ops_pll_vote,
	},
};

static struct alpha_pll_masks pll_masks_p = {
	.lock_mask = BIT(31),
	.active_mask = BIT(30),
	.vco_mask = BM(21, 20) >> 20,
	.vco_shift = 20,
	.alpha_en_mask = BIT(24),
	.output_mask = 0xf,
};

static struct alpha_pll_vco_tbl mmpll_p_vco[] = {
	VCO(3,  250000000,  500000000),
	VCO(2,  500000000, 1000000000),
	VCO(1, 1000000000, 1500000000),
	VCO(0, 1500000000, 2000000000),
};

static struct alpha_pll_clk mmpll0_clk_src = {
	.masks = &pll_masks_p,
	.base = (uint32_t )MSM_MMSS_CLK_CTL_BASE,
	.offset = 0x0,
	.vco_tbl = mmpll_p_vco,
	.vco_num = ARRAY_SIZE(mmpll_p_vco),
	.fsm_reg_offset = 0x0100,
	.fsm_en_mask = BIT(0),
	.enable_config = 0x1,
	.parent = &cxo_clk_src.c,
	.inited = false,
	.c = {
		.rate = 800000000,
		.dbg_name = "mmpll0_clk_src",
		.ops = &clk_ops_fixed_alpha_pll,
	},
};

/* UART Clocks */
static struct clk_freq_tbl ftbl_gcc_blsp1_2_uart1_6_apps_clk[] =
{
	F( 3686400,  gpll0,    1,  96,  15625),
	F( 7372800,  gpll0,    1, 192,  15625),
	F(14745600,  gpll0,    1, 384,  15625),
	F(16000000,  gpll0,    5,   2,     15),
	F(19200000,    cxo,    1,   0,      0),
	F(24000000,  gpll0,    5,   1,      5),
	F(32000000,  gpll0,    1,   4,     75),
	F(40000000,  gpll0,   15,   0,      0),
	F(46400000,  gpll0,    1,  29,    375),
	F(48000000,  gpll0, 12.5,   0,      0),
	F(51200000,  gpll0,    1,  32,    375),
	F(56000000,  gpll0,    1,   7,     75),
	F(58982400,  gpll0,    1, 1536, 15625),
	F(60000000,  gpll0,   10,   0,      0),
	F(63160000,  gpll0,  9.5,   0,      0),
	F_END
};

static struct rcg_clk blsp2_uart2_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) BLSP2_UART2_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) BLSP2_UART2_APPS_CFG_RCGR,
	.m_reg        = (uint32_t *) BLSP2_UART2_APPS_M,
	.n_reg        = (uint32_t *) BLSP2_UART2_APPS_N,
	.d_reg        = (uint32_t *) BLSP2_UART2_APPS_D,

	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_2_uart1_6_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "blsp2_uart2_apps_clk",
		.ops      = &clk_ops_rcg_mnd,
	},
};

static struct branch_clk gcc_blsp2_uart2_apps_clk =
{
	.cbcr_reg     = (uint32_t *) BLSP2_UART2_APPS_CBCR,
	.parent       = &blsp2_uart2_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_uart2_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct vote_clk gcc_blsp1_ahb_clk = {
	.cbcr_reg     = (uint32_t *) BLSP1_AHB_CBCR,
	.vote_reg     = (uint32_t *) APCS_CLOCK_BRANCH_ENA_VOTE,
	.en_mask      = BIT(17),

	.c = {
		.dbg_name = "gcc_blsp1_ahb_clk",
		.ops      = &clk_ops_vote,
	},
};

static struct vote_clk gcc_blsp2_ahb_clk = {
	.cbcr_reg     = (uint32_t *) BLSP2_AHB_CBCR,
	.vote_reg     = (uint32_t *) APCS_CLOCK_BRANCH_ENA_VOTE,
	.en_mask      = BIT(15),

	.c = {
		.dbg_name = "gcc_blsp2_ahb_clk",
		.ops      = &clk_ops_vote,
	},
};

static struct clk_freq_tbl ftbl_gcc_blsp1_qup2_i2c_apps_clk_src[] = {
	F(      96000,    cxo,  10,   1,  2),
	F(    4800000,    cxo,   4,   0,  0),
	F(    9600000,    cxo,   2,   0,  0),
	F(   16000000,  gpll0,  10,   1,  5),
	F(   19200000,  gpll0,   1,   0,  0),
	F(   25000000,  gpll0,  16,   1,  2),
	F(   50000000,  gpll0,  16,   0,  0),
	F_END
};

static struct rcg_clk gcc_blsp2_qup2_i2c_apps_clk_src = {
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP2_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP2_CFG_RCGR,
	.set_rate     = clock_lib2_rcg_set_rate_hid,
	.freq_tbl     = ftbl_gcc_blsp1_qup2_i2c_apps_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup2_i2c_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup2_i2c_apps_clk = {
	.cbcr_reg = (uint32_t *) GCC_BLSP2_QUP2_APPS_CBCR,
	.parent   = &gcc_blsp2_qup2_i2c_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup2_i2c_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

/* SDCC Clocks */
static struct clk_freq_tbl ftbl_gcc_sdcc1_4_apps_clk[] =
{
	F(   144000,    cxo,  16,   3,  25),
	F(   400000,    cxo,  12,   1,   4),
	F( 20000000,  gpll0,  15,   1,   2),
	F( 25000000,  gpll0,  12,   1,   2),
	F( 50000000,  gpll0,  12,   0,   0),
	F( 96000000,  gpll4,   4,   0,   0),
	F(192000000,  gpll4,   2,   0,   0),
	F(384000000,  gpll4,   1,   0,   0),
	F_END
};

static struct clk_freq_tbl ftbl_gcc_sdcc2_4_apps_clk[] =
{
	F(   144000,    cxo,  16,   3,  25),
	F(   400000,    cxo,  12,   1,   4),
	F( 20000000,  gpll0,  15,   1,   2),
	F( 25000000,  gpll0,  12,   1,   2),
	F( 50000000,  gpll0,  12,   0,   0),
	F(100000000,  gpll0,   6,   0,   0),
	F(200000000,  gpll0,   3,   0,   0),
	F_END
};


static struct rcg_clk sdcc1_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) SDCC1_CMD_RCGR,
	.cfg_reg      = (uint32_t *) SDCC1_CFG_RCGR,
	.m_reg        = (uint32_t *) SDCC1_M,
	.n_reg        = (uint32_t *) SDCC1_N,
	.d_reg        = (uint32_t *) SDCC1_D,

	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_sdcc1_4_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "sdc1_clk",
		.ops      = &clk_ops_rcg_mnd,
	},
};

static struct branch_clk gcc_sdcc1_apps_clk =
{
	.cbcr_reg     = (uint32_t *) SDCC1_APPS_CBCR,
	.parent       = &sdcc1_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_sdcc1_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk gcc_sdcc1_ahb_clk =
{
	.cbcr_reg     = (uint32_t *) SDCC1_AHB_CBCR,
	.has_sibling  = 1,

	.c = {
		.dbg_name = "gcc_sdcc1_ahb_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk sdcc2_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) SDCC2_CMD_RCGR,
	.cfg_reg      = (uint32_t *) SDCC2_CFG_RCGR,
	.m_reg        = (uint32_t *) SDCC2_M,
	.n_reg        = (uint32_t *) SDCC2_N,
	.d_reg        = (uint32_t *) SDCC2_D,

	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_sdcc2_4_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "sdc2_clk",
		.ops      = &clk_ops_rcg_mnd,
	},
};

static struct branch_clk gcc_sdcc2_apps_clk =
{
	.cbcr_reg     = (uint32_t *) SDCC2_APPS_CBCR,
	.parent       = &sdcc2_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_sdcc2_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk gcc_sdcc2_ahb_clk =
{
	.cbcr_reg     = (uint32_t *) SDCC2_AHB_CBCR,
	.has_sibling  = 1,

	.c = {
		.dbg_name = "gcc_sdcc2_ahb_clk",
		.ops      = &clk_ops_branch,
	},
};


static struct branch_clk gcc_sys_noc_usb30_axi_clk = {
	.cbcr_reg    = (uint32_t *) SYS_NOC_USB3_AXI_CBCR,
	.has_sibling = 1,

	.c = {
		.dbg_name = "sys_noc_usb30_axi_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk gcc_periph_noc_usb20_ahb_clk = {
	.cbcr_reg    = (uint32_t *) PERIPH_NOC_USB20_AHB_CBCR,
	.has_sibling = 1,

	.c = {
		.dbg_name = "periph_noc_usb20_ahb_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct clk_freq_tbl ftbl_gcc_usb30_master_clk[] = {
	F(  19200000, cxo,    1,    0,    0),
	F( 120000000, gpll0,    5,    0,    0),
	F( 150000000, gpll0,    4,    0,    0),
	F_END
};

static struct clk_freq_tbl ftbl_gcc_usb20_master_clk[] = {
	F(  19200000, cxo,    1,    0,    0),
	F( 120000000, gpll0,    5,    0,    0),
	F( 150000000, gpll0,    4,    0,    0),
	F_END
};

static struct rcg_clk usb30_master_clk_src = {
	.cmd_reg      = (uint32_t *) USB30_MASTER_CMD_RCGR,
	.cfg_reg      = (uint32_t *) USB30_MASTER_CFG_RCGR,
	.m_reg        = (uint32_t *) USB30_MASTER_M,
	.n_reg        = (uint32_t *) USB30_MASTER_N,
	.d_reg        = (uint32_t *) USB30_MASTER_D,

	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_usb30_master_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "usb30_master_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct rcg_clk usb20_master_clk_src = {
	.cmd_reg      = (uint32_t *) USB20_MASTER_CMD_RCGR,
	.cfg_reg      = (uint32_t *) USB20_MASTER_CFG_RCGR,
	.m_reg        = (uint32_t *) USB20_MASTER_M,
	.n_reg        = (uint32_t *) USB20_MASTER_N,
	.d_reg        = (uint32_t *) USB20_MASTER_D,

	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_usb20_master_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "usb20_master_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_usb30_master_clk = {
	.cbcr_reg = (uint32_t *) USB30_MASTER_CBCR,
	.bcr_reg  = (uint32_t *) USB_30_BCR,
	.parent   = &usb30_master_clk_src.c,

	.c = {
		.dbg_name = "usb30_master_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk gcc_usb20_master_clk = {
	.cbcr_reg = (uint32_t *) USB20_MASTER_CBCR,
	.bcr_reg  = (uint32_t *) USB_20_BCR,
	.parent   = &usb20_master_clk_src.c,

	.c = {
		.dbg_name = "usb20_master_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk gcc_aggre2_usb3_axi_clk = {
	.cbcr_reg     = (uint32_t *) GCC_AGGRE2_USB3_AXI_CBCR,
	.parent       = &usb30_master_clk_src.c,

	.c = {
		.dbg_name = "gcc_aggre2_usb3_axi_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct clk_freq_tbl ftbl_gcc_usb30_mock_utmi_clk_src[] = {
	F(  60000000, gpll0,   10,    0,     0),
	F_END
};

static struct clk_freq_tbl ftbl_gcc_usb20_mock_utmi_clk_src[] = {
	F(  60000000, gpll0,   10,    0,     0),
	F_END
};

static struct rcg_clk usb30_mock_utmi_clk_src = {
	.cmd_reg      = (uint32_t *) USB30_MOCK_UTMI_CMD_RCGR,
	.cfg_reg      = (uint32_t *) USB30_MOCK_UTMI_CFG_RCGR,
	.set_rate     = clock_lib2_rcg_set_rate_hid,
	.freq_tbl     = ftbl_gcc_usb30_mock_utmi_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "usb30_mock_utmi_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct rcg_clk usb20_mock_utmi_clk_src = {
	.cmd_reg      = (uint32_t *) USB20_MOCK_UTMI_CMD_RCGR,
	.cfg_reg      = (uint32_t *) USB20_MOCK_UTMI_CFG_RCGR,
	.set_rate     = clock_lib2_rcg_set_rate_hid,
	.freq_tbl     = ftbl_gcc_usb20_mock_utmi_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "usb20_mock_utmi_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_usb30_mock_utmi_clk = {
	.cbcr_reg    = (uint32_t *) USB30_MOCK_UTMI_CBCR,
	.has_sibling = 0,
	.parent      = &usb30_mock_utmi_clk_src.c,

	.c = {
		.dbg_name = "usb30_mock_utmi_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk gcc_usb20_mock_utmi_clk = {
	.cbcr_reg    = (uint32_t *) USB20_MOCK_UTMI_CBCR,
	.has_sibling = 0,
	.parent      = &usb20_mock_utmi_clk_src.c,

	.c = {
		.dbg_name = "usb20_mock_utmi_clk",
		.ops      = &clk_ops_branch,
	},
};


static struct branch_clk gcc_usb30_sleep_clk = {
	.cbcr_reg    = (uint32_t *) USB30_SLEEP_CBCR,
	.has_sibling = 1,

	.c = {
		.dbg_name = "usb30_sleep_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk gcc_usb20_sleep_clk = {
	.cbcr_reg    = (uint32_t *) USB20_SLEEP_CBCR,
	.has_sibling = 1,

	.c = {
		.dbg_name = "usb20_sleep_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct clk_freq_tbl ftbl_gcc_usb30_phy_aux_clk_src[] = {
	F(   1200000,         cxo,   16,    0,     0),
	F_END
};

static struct rcg_clk usb30_phy_aux_clk_src = {
	.cmd_reg      = (uint32_t *) USB30_PHY_AUX_CMD_RCGR,
	.cfg_reg      = (uint32_t *) USB30_PHY_AUX_CFG_RCGR,
	.set_rate     = clock_lib2_rcg_set_rate_hid,
	.freq_tbl     = ftbl_gcc_usb30_phy_aux_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "usb30_phy_aux_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_usb30_phy_aux_clk = {
	.cbcr_reg    = (uint32_t *)USB30_PHY_AUX_CBCR,
	.has_sibling = 0,
	.parent      = &usb30_phy_aux_clk_src.c,

	.c = {
		.dbg_name = "usb30_phy_aux_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk gcc_usb30_pipe_clk = {
	.bcr_reg      = (uint32_t *) USB30PHY_PHY_BCR,
	.cbcr_reg     = (uint32_t *) USB30_PHY_PIPE_CBCR,
	.has_sibling  = 1,

	.c = {
		.dbg_name = "usb30_pipe_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct reset_clk gcc_usb30_phy_reset = {
	.bcr_reg = (uint32_t )USB30_PHY_BCR,

	.c = {
		.dbg_name = "usb30_phy_reset",
		.ops      = &clk_ops_rst,
	},
};

static struct branch_clk gcc_usb_phy_cfg_ahb2phy_clk = {
	.cbcr_reg = (uint32_t *)USB_PHY_CFG_AHB2PHY_CBCR,
	.has_sibling = 1,

	.c = {
		.dbg_name = "usb_phy_cfg_ahb2phy_clk",
		.ops = &clk_ops_branch,
	},
};

/* SPI Clocks */
static struct clk_freq_tbl ftbl_gcc_blsp1_qup_spi_apps_clk[] = {
	F(960000,	cxo,	10,	1,	2),
	F(4800000,	cxo,	4,	0,	0),
	F(9600000,	cxo,	2,	0,	0),
	F(5000000,	gpll0,	10,	1,	4),
	F(25000000,	gpll0,	12,	1,	2),
	F(50000000,	gpll0,	12,	0,	0),
	F_END
};

static struct rcg_clk gcc_blsp1_qup0_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP0_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP0_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP0_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP0_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP0_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup0_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup0_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP0_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup0_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup0_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp1_qup1_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP1_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP1_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP1_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP1_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP1_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup1_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup1_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP1_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup1_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup1_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp1_qup2_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP2_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP2_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP2_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP2_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP2_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup2_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup2_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP2_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup2_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup2_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp1_qup3_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP3_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP3_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP3_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP3_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP3_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup3_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup3_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP3_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup3_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup3_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp1_qup4_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP4_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP4_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP4_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP4_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP4_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup4_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup4_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP4_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup4_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup4_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp1_qup5_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP5_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP5_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP5_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP5_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP5_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup5_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup5_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP5_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup5_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup5_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct clk_freq_tbl ftbl_gcc_blsp2_qup_spi_apps_clk[] = {
	F(960000,	cxo,	10,	1,	2),
	F(4800000,	cxo,	4,	0,	0),
	F(9600000,	cxo,	2,	0,	0),
	F(5000000,	gpll0,	10,	1,	4),
	F(25000000,	gpll0,	12,	1,	2),
	F(50000000,	gpll0,	12,	0,	0),
	F_END
};

static struct rcg_clk gcc_blsp2_qup0_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP0_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP0_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP0_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP0_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP0_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup0_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup0_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP0_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup0_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup0_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp2_qup1_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP1_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP1_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP1_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP1_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP1_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup1_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup1_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP1_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup1_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup1_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp2_qup2_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP2_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP2_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP2_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP2_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP2_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup2_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup2_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP2_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup2_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup2_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp2_qup3_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP3_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP3_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP3_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP3_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP3_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup3_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup3_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP3_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup3_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup3_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp2_qup4_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP4_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP4_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP4_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP4_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP4_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup4_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup4_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP4_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup4_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup4_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp2_qup5_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP5_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP5_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP5_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP5_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP5_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup5_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup5_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP5_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup5_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup5_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

/* SPI Clocks */
static struct clk_freq_tbl ftbl_gcc_blsp1_qup_spi_apps_clk[] = {
	F(960000,	cxo,	10,	1,	2),
	F(4800000,	cxo,	4,	0,	0),
	F(9600000,	cxo,	2,	0,	0),
	F(5000000,	gpll0,	10,	1,	4),
	F(25000000,	gpll0,	12,	1,	2),
	F(50000000,	gpll0,	12,	0,	0),
	F_END
};

static struct rcg_clk gcc_blsp1_qup0_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP0_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP0_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP0_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP0_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP0_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup0_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup0_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP0_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup0_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup0_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp1_qup1_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP1_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP1_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP1_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP1_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP1_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup1_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup1_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP1_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup1_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup1_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp1_qup2_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP2_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP2_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP2_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP2_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP2_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup2_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup2_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP2_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup2_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup2_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp1_qup3_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP3_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP3_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP3_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP3_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP3_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup3_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup3_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP3_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup3_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup3_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp1_qup4_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP4_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP4_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP4_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP4_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP4_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup4_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup4_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP4_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup4_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup4_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp1_qup5_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP1_QUP5_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP1_QUP5_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP1_QUP5_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP1_QUP5_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP1_QUP5_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp1_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp1_qup5_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp1_qup5_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP1_QUP5_SPI_APPS_CBCR,
	.parent   = &gcc_blsp1_qup5_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp1_qup5_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct clk_freq_tbl ftbl_gcc_blsp2_qup_spi_apps_clk[] = {
	F(960000,	cxo,	10,	1,	2),
	F(4800000,	cxo,	4,	0,	0),
	F(9600000,	cxo,	2,	0,	0),
	F(5000000,	gpll0,	10,	1,	4),
	F(25000000,	gpll0,	12,	1,	2),
	F(50000000,	gpll0,	12,	0,	0),
	F_END
};

static struct rcg_clk gcc_blsp2_qup0_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP0_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP0_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP0_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP0_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP0_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup0_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup0_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP0_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup0_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup0_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp2_qup1_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP1_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP1_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP1_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP1_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP1_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup1_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup1_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP1_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup1_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup1_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp2_qup2_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP2_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP2_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP2_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP2_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP2_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup2_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup2_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP2_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup2_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup2_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp2_qup3_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP3_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP3_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP3_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP3_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP3_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup3_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup3_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP3_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup3_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup3_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp2_qup4_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP4_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP4_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP4_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP4_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP4_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup4_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup4_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP4_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup4_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup4_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct rcg_clk gcc_blsp2_qup5_spi_apps_clk_src =
{
	.cmd_reg      = (uint32_t *) GCC_BLSP2_QUP5_SPI_APPS_CMD_RCGR,
	.cfg_reg      = (uint32_t *) GCC_BLSP2_QUP5_SPI_CFG_RCGR,
	.m_reg        = (uint32_t *) GCC_BLSP2_QUP5_SPI_APPS_M,
	.n_reg        = (uint32_t *) GCC_BLSP2_QUP5_SPI_APPS_N,
	.d_reg        = (uint32_t *) GCC_BLSP2_QUP5_SPI_APPS_D,
	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_gcc_blsp2_qup_spi_apps_clk,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "gcc_blsp2_qup5_spi_apps_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk gcc_blsp2_qup5_spi_apps_clk = {
	.cbcr_reg = (uint32_t *)GCC_BLSP2_QUP5_SPI_APPS_CBCR,
	.parent   = &gcc_blsp2_qup5_spi_apps_clk_src.c,

	.c = {
		.dbg_name = "gcc_blsp2_qup5_spi_apps_clk",
		.ops      = &clk_ops_branch,
	},
};

/* Display clocks */
static struct clk_freq_tbl ftbl_mdss_esc0_1_clk[] = {
	F_MM(19200000,    cxo,   1,   0,   0),
	F_END
};

static struct clk_freq_tbl ftbl_mdss_esc1_1_clk[] = {
	F_MM(19200000,    cxo,   1,   0,   0),
	F_END
};

static struct clk_freq_tbl ftbl_axi_clk_src[] = {
	F_MM( 171430000,      gpll0,  3.5,    0,     0),
	F_MM( 200000000,      gpll0,    3,    0,     0),
	F_MM( 320000000,     mmpll0,  2.5,    0,     0),
	F_END
};

static struct clk_freq_tbl ftbl_mdp_clk_src[] = {
	F_MM(  85714286,   gpll0,    7,    0,     0),
	F_MM( 100000000,   gpll0,    6,    0,     0),
	F_MM( 150000000,   gpll0,    4,    0,     0),
	F_MM( 171428571,   gpll0,  3.5,    0,     0),
	F_MM( 320000000,  mmpll0,  2.5,    0,     0),
	F_END
};

static struct clk_freq_tbl ftbl_ahb_clk_src[] = {
	F_MM(  19200000,        cxo,     1,    0,     0),
	F_END
};

static struct rcg_clk ahb_clk_src = {
	.cmd_reg = (uint32_t *)MMSS_AHB_CMD_RCGR,
	.cfg_reg = (uint32_t *)MMSS_AHB_CFG_RCGR,
	.set_rate     = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_ahb_clk_src,
	.c = {
		.dbg_name = "ahb_clk_src",
		.ops = &clk_ops_rcg,
	},
};

static struct rcg_clk dsi_esc0_clk_src = {
	.cmd_reg  = (uint32_t *) DSI_ESC0_CMD_RCGR,
	.cfg_reg  = (uint32_t *) DSI_ESC0_CFG_RCGR,
	.set_rate = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_mdss_esc0_1_clk,

	.c        = {
		.dbg_name = "dsi_esc0_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct rcg_clk dsi_esc1_clk_src = {
	.cmd_reg  = (uint32_t *) DSI_ESC1_CMD_RCGR,
	.cfg_reg  = (uint32_t *) DSI_ESC1_CFG_RCGR,
	.set_rate = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_mdss_esc1_1_clk,

	.c        = {
		.dbg_name = "dsi_esc1_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct clk_freq_tbl ftbl_mdss_vsync_clk[] = {
	F_MM(19200000,    cxo,   1,   0,   0),
	F_END
};

static struct rcg_clk vsync_clk_src = {
	.cmd_reg  = (uint32_t *) VSYNC_CMD_RCGR,
	.cfg_reg  = (uint32_t *) VSYNC_CFG_RCGR,
	.set_rate = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_mdss_vsync_clk,

	.c        = {
		.dbg_name = "vsync_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct rcg_clk axi_clk_src = {
	.cmd_reg  = (uint32_t *) AXI_CMD_RCGR,
	.cfg_reg  = (uint32_t *) AXI_CFG_RCGR,
	.set_rate = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_axi_clk_src,

	.c        = {
		.dbg_name = "axi_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk mdss_esc0_clk = {
	.cbcr_reg    = (uint32_t *) DSI_ESC0_CBCR,
	.parent      = &dsi_esc0_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "mdss_esc0_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk mdss_esc1_clk = {
	.cbcr_reg    = (uint32_t *) DSI_ESC1_CBCR,
	.parent      = &dsi_esc1_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "mdss_esc1_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk mdss_axi_clk = {
	.cbcr_reg    = (uint32_t *) MDSS_AXI_CBCR,
	.parent      = &axi_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "mdss_axi_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk smmu_mdp_axi_clk = {
	.cbcr_reg    = (uint32_t *) SMMU_MDP_AXI_CBCR,
	.parent      = &axi_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "smmu_mdp_axi_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk mmss_mmagic_axi_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_MMAGIC_AXI_CBCR,
	.parent      = &axi_clk_src.c,
	.has_sibling = 0,
	.c           = {
		.dbg_name = "mmss_mmagic_axi_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk mmagic_mdss_axi_clk = {
	.cbcr_reg    = (uint32_t *) MMAGIC_MDSS_AXI_CBCR,
	.parent      = &axi_clk_src.c,
	.has_sibling = 0,
	.c           = {
		.dbg_name = "mmagic_mdss_axi_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk mmagic_bimc_axi_clk = {
	.cbcr_reg    = (uint32_t *) MMAGIC_BIMC_AXI_CBCR,
	.parent      = &axi_clk_src.c,
	.has_sibling = 0,
	.c           = {
		.dbg_name = "mmagic_bimc_axi_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk mmss_s0_axi_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_S0_AXI_CBCR,
	.parent      = &axi_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "mmss_s0_axi_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk mdp_ahb_clk = {
	.cbcr_reg    = (uint32_t *) MDSS_AHB_CBCR,
	.has_sibling = 1,
	.parent      = &ahb_clk_src.c,

	.c           = {
		.dbg_name = "mdp_ahb_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk mmss_mmagic_ahb_clk = {
	.cbcr_reg = (uint32_t *) MMSS_MMAGIC_AHB_CBCR,
	.has_sibling = 0,
	.parent      = &ahb_clk_src.c,
	.no_halt_check_on_disable = true,

	.c = {
		.dbg_name = "mmss_mmagic_ahb_clk",
		.ops = &clk_ops_branch,
    },
};

static struct branch_clk smmu_mdp_ahb_clk = {
	.cbcr_reg = (uint32_t *) SMMU_MDP_AHB_CBCR,
	.has_sibling = 1,
	.parent      = &ahb_clk_src.c,

	.c = {
		.dbg_name = "smmu_mdp_ahb_clk",
		.ops = &clk_ops_branch,
    },
};

static struct rcg_clk mdss_mdp_clk_src = {
	.cmd_reg      = (uint32_t *) MDP_CMD_RCGR,
	.cfg_reg      = (uint32_t *) MDP_CFG_RCGR,
	.set_rate     = clock_lib2_rcg_set_rate_hid,
	.freq_tbl     = ftbl_mdp_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c            = {
		.dbg_name = "mdss_mdp_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk mdss_mdp_clk = {
	.cbcr_reg    = (uint32_t *) MDP_CBCR,
	.parent      = &mdss_mdp_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "mdss_mdp_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk mdss_vsync_clk = {
	.cbcr_reg    = (uint32_t *) MDSS_VSYNC_CBCR,
	.parent      = &vsync_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "mdss_vsync_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk mdss_hdmi_ahb_clk = {
	.cbcr_reg = (uint32_t *) MDSS_HDMI_AHB_CBCR,
	.has_sibling = 1,
	.c = {
		.dbg_name = "mdss_hdmi_ahb_clk",
		.ops = &clk_ops_branch,
	},
};

static struct clk_freq_tbl ftbl_mdss_hdmi_clk[] = {
	F_MM( 19200000,  cxo,    1, 0, 0),
	F_END
};

static struct rcg_clk hdmi_clk_src = {
	.cmd_reg = (uint32_t *) HDMI_CMD_RCGR,
	.cfg_reg = (uint32_t *) HDMI_CFG_RCGR,
	.set_rate = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_mdss_hdmi_clk,
	.current_freq = &rcg_dummy_freq,
	.c = {
		.dbg_name = "hdmi_clk_src",
		.ops = &clk_ops_rcg,
	},
};

static struct branch_clk mdss_hdmi_clk = {
	.cbcr_reg = (uint32_t *) MDSS_HDMI_CBCR,
	.has_sibling = 0,
	.parent = &hdmi_clk_src.c,
	.c = {
		.dbg_name = "mdss_hdmi_clk",
		.ops = &clk_ops_branch,
	},
};

static struct clk_freq_tbl ftbl_mdss_extpclk_clk[] = {
	F_MDSS( 74250000, hdmipll, 1, 0, 0),
	F_MDSS( 25200000, hdmipll, 1, 0, 0),
	F_MDSS( 27000000, hdmipll, 1, 0, 0),
	F_MDSS( 27030000, hdmipll, 1, 0, 0),
	F_MDSS( 27070000, hdmipll, 1, 0, 0),
	F_MDSS( 65000000, hdmipll, 1, 0, 0),
	F_MDSS(108000000, hdmipll, 1, 0, 0),
	F_MDSS(148500000, hdmipll, 1, 0, 0),
	F_MDSS(268500000, hdmipll, 1, 0, 0),
	F_MDSS(297000000, hdmipll, 1, 0, 0),
	F_MDSS(594000000, hdmipll, 1, 0, 0),
	F_END
};

static struct rcg_clk extpclk_clk_src = {
	.cmd_reg = (uint32_t *) EXTPCLK_CMD_RCGR,
	.cfg_reg = (uint32_t *) EXTPCLK_CFG_RCGR,
	.set_rate = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_mdss_extpclk_clk,
	.current_freq = &rcg_dummy_freq,
	.c = {
		.dbg_name = "extpclk_clk_src",
		.ops = &clk_ops_rcg,
	},
};

static struct branch_clk mdss_extpclk_clk = {
	.cbcr_reg = (uint32_t *) MDSS_EXTPCLK_CBCR,
	.has_sibling = 0,
	.parent = &extpclk_clk_src.c,
	.c = {
		.dbg_name = "mdss_extpclk_clk",
		.ops = &clk_ops_branch,
	},
};

#ifdef EARLYDOMAIN_SUPPORT
static struct clk_freq_tbl ftbl_csi2phytimer_clk_src[] = {
	F_MM( 200000000, gpll0,    3,    0,     0),
	F_END
};

static struct rcg_clk csi2phytimer_clk_src = {
	.cmd_reg = (uint32_t *) MMSS_CSI2PHYTIMER_CMD_RCGR,
	.cfg_reg      = (uint32_t *) MMSS_CSI2PHYTIMER_CFG_RCGR,
	.set_rate = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_csi2phytimer_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "csi2phytimer_clk_src",
		.ops = &clk_ops_rcg,
	},
};

static struct clk_freq_tbl ftbl_csiphy2_3p_clk_src[] = {
	F_MM( 100000000,   gpll0,    6,    0,     0),
	F_END
};

static struct rcg_clk csiphy2_3p_clk_src = {
	.cmd_reg = (uint32_t *) MMSS_CSIPHY2_3P_CMD_RCGR,
	.cfg_reg = (uint32_t *) MMSS_CSIPHY2_3P_CFG_RCGR,
	.set_rate = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_csiphy2_3p_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "csiphy2_3p_clk_src",
		.ops = &clk_ops_rcg,
	},
};

static struct branch_clk camss_csiphy2_3p_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_CSIPHY2_3P_CBCR,
	.parent = &csiphy2_3p_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_csiphy2_3p_clk",
		.ops = &clk_ops_branch,
	},
};

static struct rcg_clk csiphy1_3p_clk_src = {
	.cmd_reg = (uint32_t *) MMSS_CSIPHY1_3P_CMD_RCGR,
	.cfg_reg = (uint32_t *) MMSS_CSIPHY1_3P_CFG_RCGR,
	.set_rate = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_csiphy2_3p_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "csiphy1_3p_clk_src",
		.ops = &clk_ops_rcg,
	},
};

static struct branch_clk camss_csiphy1_3p_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_CSIPHY1_3P_CBCR,
	.parent = &csiphy1_3p_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_csiphy1_3p_clk",
		.ops = &clk_ops_branch,
	},
};

static struct clk_freq_tbl ftbl_csi2_clk_src[] = {
	F_MM( 200000000,     gpll0,    3,    0,     0),
	F_END
};

static struct rcg_clk csi2_clk_src = {
	.cmd_reg = (uint32_t *) MMSS_CSI2_CMD_RCGR,
	.cfg_reg = (uint32_t *) MMSS_CSI2_CFG_RCGR,
	.set_rate = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_csi2_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "csi2_clk_src",
		.ops = &clk_ops_rcg,
	},
};

static struct branch_clk camss_csi2phy_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_CSI2PHY_CBCR,
	.parent = &csi2_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_csi2phy_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi2_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_CSI2_CBCR,
	.parent = &csi2_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_csi2_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi2rdi_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_CSI2RDI_CBCR,
	.parent = &csi2_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_csi2rdi_clk",
		.ops = &clk_ops_branch,
	},
};

static struct clk_freq_tbl ftbl_vfe0_clk_src[] = {
	F_MM(  75000000,   gpll0,    4,    0,     0),
	F_MM( 100000000,   gpll0,    6,    0,     0),
	F_MM( 200000000,   gpll0,    3,    0,     0),
	F_MM( 320000000,   mmpll0,  2.5,    0,     0),
	F_MM( 400000000,   mmpll0,    2,    0,     0),
	F_MM( 600000000,   gpll0,    1,    0,     0),
	F_END
};

/* CCI Clocks */
static struct clk_freq_tbl ftbl_cci_clk_src[] =
{
	F_MM(  19200000,       cxo,    1,    0,   0),
	F_MM(  37500000,     gpll0,   16,    0,   0),
	F_MM(  50000000,     gpll0,   12,    0,   0),
	F_MM( 100000000,     gpll0,    6,    0,   0),
	F_END
};

static struct rcg_clk cci_clk_src =
{
	.cmd_reg      = (uint32_t *) MMSS_CCI_CMD_RCGR,
	.cfg_reg      = (uint32_t *) MMSS_CCI_CFG_RCGR,
	.m_reg        = (uint32_t *) MMSS_CCI_M,
	.n_reg        = (uint32_t *) MMSS_CCI_N,
	.d_reg        = (uint32_t *) MMSS_CCI_D,

	.set_rate     = clock_lib2_rcg_set_rate_mnd,
	.freq_tbl     = ftbl_cci_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "cci_clk_src",
		.ops      = &clk_ops_rcg_mnd,
	},
};


static struct clk_freq_tbl ftbl_csi0_clk_src[] = {
	F_MM( 200000000,     gpll0,    3,    0,     0),
	F_END
};

static struct rcg_clk csi0_clk_src =
{
	.cmd_reg      = (uint32_t *) MMSS_CSI0_CMD_RCGR,
	.cfg_reg      = (uint32_t *) MMSS_CSI0_CFG_RCGR,
	.set_rate     = clock_lib2_rcg_set_rate_hid,
	.freq_tbl     = ftbl_csi0_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "csi0_clk_src",
		.ops      = &clk_ops_rcg,
	},
};
static struct clk_freq_tbl ftbl_csi1_clk_src[] = {
	F_MM( 200000000,     gpll0,    3,    0,     0),
	F_END
};

static struct rcg_clk csi1_clk_src = {
	.cmd_reg = (uint32_t *) MMSS_CSI1_CMD_RCGR,
	.cfg_reg = (uint32_t *) MMSS_CSI1_CFG_RCGR,
	.set_rate = clock_lib2_rcg_set_rate_hid,
	.freq_tbl = ftbl_csi1_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "csi1_clk_src",
		.ops = &clk_ops_rcg,
	},
};

static struct rcg_clk vfe0_clk_src =
{
	.cmd_reg      = (uint32_t *) MMSS_VFE0_CMD_RCGR,
	.cfg_reg      = (uint32_t *) MMSS_VFE0_CFG_RCGR,
	.set_rate     = clock_lib2_rcg_set_rate_hid,
	.freq_tbl     = ftbl_vfe0_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "vfe0_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk camss_ispif_ahb_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_ISPIF_AHB_CBCR,
	.has_sibling = 1,

	.c = {
		.dbg_name = "camss_ispif_ahb_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_vfe0_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_VFE0_CBCR,
	.parent = &vfe0_clk_src.c,
	.bcr_reg = (uint32_t *) MMSS_CAMSS_VFE0_BCR,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_vfe0_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_ahb_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_AHB_CBCR,
	.parent      = &ahb_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "camss_ahb_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_cci_ahb_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CCI_AHB_CBCR,
	.has_sibling = 1,

	.c           = {
		.dbg_name = "camss_cci_ahb_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk mmagic_camss_axi_clk = {
	.cbcr_reg = (uint32_t *) MMSS_MMAGIC_CAMSS_AXI_CBCR,
	.parent      = &axi_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "mmagic_camss_axi_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_top_ahb_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_TOP_AHB_CBCR,
	.has_sibling = 1,

	.c           = {
		.dbg_name = "camss_top_ahb_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_cci_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CCI_CBCR,
	.parent      = &cci_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "camss_cci_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_vfe0_ahb_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_VFE0_AHB_CBCR,
	.has_sibling = 1,

	.c           = {
		.dbg_name = "camss_vfe0_ahb_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi_vfe0_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CSI_VFE0_CBCR,
	.parent      = &vfe0_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "camss_csi_vfe0_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_vfe_ahb_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_VFE_AHB_CBCR,
	.has_sibling = 1,

	.c = {
		.dbg_name = "camss_vfe_ahb_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_vfe0_stream_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_VFE0_STREAM_CBCR,
	.parent = &vfe0_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_vfe0_stream_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi2_ahb_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_CSI2_AHB_CBCR,
	.has_sibling = 1,

	.c = {
		.dbg_name = "camss_csi2_ahb_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi0_ahb_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CSI0_AHB_CBCR,
	.has_sibling = 1,

	.c           = {
		.dbg_name = "camss_csi0_ahb_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi1_ahb_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CSI1_AHB_CBCR,
	.has_sibling = 1,

	.c           = {
		.dbg_name = "camss_csi1_ahb_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi0_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CSI0_CBCR,
	.parent      = &csi0_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "camss_csi0_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi1_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CSI1_CBCR,
	.parent      = &csi1_clk_src.c,
	.has_sibling = 0,

	.c           = {
		.dbg_name = "camss_csi1_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi0phy_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CSI0PHY_CBCR,
	.parent      = &csi0_clk_src.c,
	.has_sibling = 1,

	.c           = {
		.dbg_name = "camss_csi0phy_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi0pix_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CSI0PIX_CBCR,
	.parent      = &csi0_clk_src.c,
	.has_sibling = 1,

	.c           = {
		.dbg_name = "camss_csi0pix_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi0rdi_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CSI0RDI_CBCR,
	.parent      = &csi0_clk_src.c,
	.has_sibling = 1,

	.c           = {
		.dbg_name = "camss_csi0rdi_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi1phy_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CSI1PHY_CBCR,
	.parent      = &csi1_clk_src.c,
	.has_sibling = 1,

	.c           = {
		.dbg_name = "camss_csi1phy_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi1pix_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CSI1PIX_CBCR,
	.parent      = &csi1_clk_src.c,
	.has_sibling = 1,

	.c           = {
		.dbg_name = "camss_csi1pix_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi1rdi_clk = {
	.cbcr_reg    = (uint32_t *) MMSS_CAMSS_CSI1RDI_CBCR,
	.parent      = &csi1_clk_src.c,
	.has_sibling = 1,

	.c           = {
		.dbg_name = "camss_csi1rdi_clk",
		.ops      = &clk_ops_branch,
	},
};

static struct clk_freq_tbl ftbl_csi0phytimer_clk_src[] = {
		F_MM( 200000000,  gpll0,    3,    0,     0),
	F_END
};

static struct rcg_clk csi0phytimer_clk_src =
{
	.cmd_reg      = (uint32_t *) MMSS_CSI0PHYTIMER_CMD_RCGR,
	.cfg_reg      = (uint32_t *) MMSS_CSI0PHYTIMER_CFG_RCGR,
	.set_rate     = clock_lib2_rcg_set_rate_hid,
	.freq_tbl     = ftbl_csi0phytimer_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "csi0phytimer_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct rcg_clk csi1phytimer_clk_src =
{
	.cmd_reg      = (uint32_t *) MMSS_CSI1PHYTIMER_CMD_RCGR,
	.cfg_reg      = (uint32_t *) MMSS_CSI1PHYTIMER_CFG_RCGR,
	.set_rate     = clock_lib2_rcg_set_rate_hid,
	.freq_tbl     = ftbl_csi0phytimer_clk_src,
	.current_freq = &rcg_dummy_freq,

	.c = {
		.dbg_name = "csi1phytimer_clk_src",
		.ops      = &clk_ops_rcg,
	},
};

static struct branch_clk camss_csi1phytimer_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_CSI1PHYTIMER_CBCR,
	.parent = &csi1phytimer_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_csi1phytimer_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi2phytimer_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_CSI2PHYTIMER_CBCR,
	.parent = &csi2phytimer_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_csi2phytimer_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_csi2pix_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_CSI2PIX_CBCR,
	.parent = &csi2_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_csi2pix_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_cpp_vbif_ahb_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_CPP_VBIF_AHB_CBCR,
	.has_sibling = 1,

	.c = {
		.dbg_name = "camss_cpp_vbif_ahb_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_vfe_axi_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_VFE_AXI_CBCR,
	.parent      = &axi_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_vfe_axi_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_cpp_axi_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_CPP_AXI_CBCR,
	.parent      = &axi_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_cpp_axi_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk camss_jpeg_axi_clk = {
	.cbcr_reg = (uint32_t *) MMSS_CAMSS_JPEG_AXI_CBCR,
	.parent      = &axi_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "camss_jpeg_axi_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk smmu_vfe_axi_clk = {
	.cbcr_reg = (uint32_t *) MMSS_SMMU_VFE_AXI_CBCR,
	.parent      = &axi_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "smmu_vfe_axi_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk smmu_vfe_ahb_clk = {
	.cbcr_reg = (uint32_t *) MMSS_SMMU_VFE_AHB_CBCR,
	.parent = &ahb_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "smmu_vfe_ahb_clk",
		.ops = &clk_ops_branch,
	},
};

static struct branch_clk mmss_mmagic_cfg_ahb_clk = {
	.cbcr_reg = (uint32_t *) MMSS_MMSS_MMAGIC_CFG_AHB_CBCR,
	.parent = &ahb_clk_src.c,
	.has_sibling = 0,

	.c = {
		.dbg_name = "mmss_mmagic_cfg_ahb_clk",
		.ops = &clk_ops_branch,
	},
};
#endif

/* Clock lookup table */
static struct clk_lookup msm_msm8996_clocks[] =
{
	CLK_LOOKUP("sdc1_iface_clk", gcc_sdcc1_ahb_clk.c),
	CLK_LOOKUP("sdc1_core_clk",  gcc_sdcc1_apps_clk.c),

	CLK_LOOKUP("sdc2_iface_clk", gcc_sdcc2_ahb_clk.c),
	CLK_LOOKUP("sdc2_core_clk",  gcc_sdcc2_apps_clk.c),

	CLK_LOOKUP("uart8_iface_clk", gcc_blsp2_ahb_clk.c),
	CLK_LOOKUP("uart8_core_clk",  gcc_blsp2_uart2_apps_clk.c),

	/* USB30 clocks */
	CLK_LOOKUP("usb30_master_clk",    gcc_usb30_master_clk.c),
	CLK_LOOKUP("gcc_aggre2_usb3_axi_clk", gcc_aggre2_usb3_axi_clk.c),
	CLK_LOOKUP("usb30_iface_clk",     gcc_sys_noc_usb30_axi_clk.c),
	CLK_LOOKUP("usb30_mock_utmi_clk", gcc_usb30_mock_utmi_clk.c),
	CLK_LOOKUP("usb30_sleep_clk",     gcc_usb30_sleep_clk.c),
	CLK_LOOKUP("usb30_phy_aux_clk",   gcc_usb30_phy_aux_clk.c),
	CLK_LOOKUP("usb30_pipe_clk",      gcc_usb30_pipe_clk.c),
	CLK_LOOKUP("usb30_phy_reset",     gcc_usb30_phy_reset.c),

	CLK_LOOKUP("usb_phy_cfg_ahb2phy_clk",     gcc_usb_phy_cfg_ahb2phy_clk.c),

	/* USB20 clocks */
	CLK_LOOKUP("usb20_noc_usb20_clk",     gcc_periph_noc_usb20_ahb_clk.c),
	CLK_LOOKUP("usb20_master_clk",    gcc_usb20_master_clk.c),
	CLK_LOOKUP("usb20_mock_utmi_clk", gcc_usb20_mock_utmi_clk.c),
	CLK_LOOKUP("usb20_sleep_clk",     gcc_usb20_sleep_clk.c),

	/* mdss clocks */
	CLK_LOOKUP("mdss_mdp_clk",     mdss_mdp_clk.c),
	CLK_LOOKUP("mdss_vsync_clk",       mdss_vsync_clk.c),
	CLK_LOOKUP("mdss_mdp_clk",         mdss_mdp_clk.c),
	CLK_LOOKUP("mdss_esc0_clk",        mdss_esc0_clk.c),
	CLK_LOOKUP("mdss_esc1_clk",        mdss_esc1_clk.c),
	CLK_LOOKUP("mmss_s0_axi_clk",      mmss_s0_axi_clk.c),
	CLK_LOOKUP("mmss_mmagic_axi_clk",  mmss_mmagic_axi_clk.c),
	CLK_LOOKUP("mmagic_mdss_axi_clk",  mmagic_mdss_axi_clk.c),
	CLK_LOOKUP("mmagic_bimc_axi_clk",  mmagic_bimc_axi_clk.c),
	CLK_LOOKUP("smmu_mdp_axi_clk",     smmu_mdp_axi_clk.c),
	CLK_LOOKUP("mdss_axi_clk",         mdss_axi_clk.c),
	CLK_LOOKUP("mmss_mmagic_ahb_clk",       mmss_mmagic_ahb_clk.c),
	CLK_LOOKUP("smmu_mdp_ahb_clk",     smmu_mdp_ahb_clk.c),
	CLK_LOOKUP("mdp_ahb_clk",          mdp_ahb_clk.c),

	/* BLSP CLOCKS */
	CLK_LOOKUP("gcc_blsp2_ahb_clk", gcc_blsp2_ahb_clk.c),
	CLK_LOOKUP("gcc_blsp1_ahb_clk", gcc_blsp1_ahb_clk.c),
	CLK_LOOKUP("gcc_blsp2_qup2_i2c_apps_clk_src",
		gcc_blsp2_qup2_i2c_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp2_qup2_i2c_apps_clk",
		gcc_blsp2_qup2_i2c_apps_clk.c),
	CLK_LOOKUP("gcc_blsp1_qup0_spi_apps_clk_src", gcc_blsp1_qup0_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp1_qup0_spi_apps_clk", gcc_blsp1_qup0_spi_apps_clk.c),
	CLK_LOOKUP("gcc_blsp1_qup1_spi_apps_clk_src", gcc_blsp1_qup1_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp1_qup1_spi_apps_clk", gcc_blsp1_qup1_spi_apps_clk.c),
	CLK_LOOKUP("gcc_blsp1_qup2_spi_apps_clk_src", gcc_blsp1_qup2_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp1_qup2_spi_apps_clk", gcc_blsp1_qup2_spi_apps_clk.c),
	CLK_LOOKUP("gcc_blsp1_qup3_spi_apps_clk_src", gcc_blsp1_qup3_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp1_qup3_spi_apps_clk", gcc_blsp1_qup3_spi_apps_clk.c),
	CLK_LOOKUP("gcc_blsp1_qup4_spi_apps_clk_src", gcc_blsp1_qup4_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp1_qup4_spi_apps_clk", gcc_blsp1_qup4_spi_apps_clk.c),
	CLK_LOOKUP("gcc_blsp1_qup5_spi_apps_clk_src", gcc_blsp1_qup5_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp1_qup5_spi_apps_clk", gcc_blsp1_qup5_spi_apps_clk.c),
	CLK_LOOKUP("gcc_blsp2_qup0_spi_apps_clk_src", gcc_blsp2_qup0_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp2_qup0_spi_apps_clk", gcc_blsp2_qup0_spi_apps_clk.c),
	CLK_LOOKUP("gcc_blsp2_qup1_spi_apps_clk_src", gcc_blsp2_qup1_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp2_qup1_spi_apps_clk", gcc_blsp2_qup1_spi_apps_clk.c),
	CLK_LOOKUP("gcc_blsp2_qup2_spi_apps_clk_src", gcc_blsp2_qup2_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp2_qup2_spi_apps_clk", gcc_blsp2_qup2_spi_apps_clk.c),
	CLK_LOOKUP("gcc_blsp2_qup3_spi_apps_clk_src", gcc_blsp2_qup3_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp2_qup3_spi_apps_clk", gcc_blsp2_qup3_spi_apps_clk.c),
	CLK_LOOKUP("gcc_blsp2_qup4_spi_apps_clk_src", gcc_blsp2_qup4_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp2_qup4_spi_apps_clk", gcc_blsp2_qup4_spi_apps_clk.c),
	CLK_LOOKUP("gcc_blsp2_qup5_spi_apps_clk_src", gcc_blsp2_qup5_spi_apps_clk_src.c),
	CLK_LOOKUP("gcc_blsp2_qup5_spi_apps_clk", gcc_blsp2_qup5_spi_apps_clk.c),

	/* HDMI clocks*/
	CLK_LOOKUP("hdmi_ahb_clk",         mdss_hdmi_ahb_clk.c),
	CLK_LOOKUP("hdmi_core_clk",        mdss_hdmi_clk.c),
	CLK_LOOKUP("hdmi_extp_clk",        mdss_extpclk_clk.c),

#ifdef EARLYDOMAIN_SUPPORT
	/* camera clocks */
	CLK_LOOKUP("camss_ahb_clk",        camss_ahb_clk.c),
	CLK_LOOKUP("camss_cci_ahb_clk",    camss_cci_ahb_clk.c),
	CLK_LOOKUP("camss_cci_clk",        camss_cci_clk.c),
	CLK_LOOKUP("cci_clk_src",          cci_clk_src.c),
	CLK_LOOKUP("camss_top_ahb_clk",    camss_top_ahb_clk.c),
	CLK_LOOKUP("vfe0_clk_src",         vfe0_clk_src.c),
	CLK_LOOKUP("camss_vfe0_ahb_clk",   camss_vfe0_ahb_clk.c),
	CLK_LOOKUP("camss_csi_vfe0_clk",   camss_csi_vfe0_clk.c),
	CLK_LOOKUP("camss_csi0_ahb_clk",   camss_csi0_ahb_clk.c),
	CLK_LOOKUP("camss_csi1_ahb_clk",   camss_csi1_ahb_clk.c),
	CLK_LOOKUP("csi0_clk_src",         csi0_clk_src.c),
	CLK_LOOKUP("csi1_clk_src",         csi1_clk_src.c),
	CLK_LOOKUP("camss_csi0_clk",       camss_csi0_clk.c),
	CLK_LOOKUP("camss_csi1_clk",       camss_csi1_clk.c),
	CLK_LOOKUP("camss_csi0phy_clk",    camss_csi0phy_clk.c),
	CLK_LOOKUP("camss_csi0pix_clk",    camss_csi0pix_clk.c),
	CLK_LOOKUP("camss_csi0rdi_clk",    camss_csi0rdi_clk.c),
	CLK_LOOKUP("camss_csi1phy_clk",    camss_csi1phy_clk.c),
	CLK_LOOKUP("camss_csi1pix_clk",    camss_csi1pix_clk.c),
	CLK_LOOKUP("camss_csi1rdi_clk",    camss_csi1rdi_clk.c),
	CLK_LOOKUP("csi0phytimer_clk_src", csi0phytimer_clk_src.c),
	CLK_LOOKUP("csi1phytimer_clk_src", csi1phytimer_clk_src.c),
	CLK_LOOKUP("mmagic_camss_axi_clk", mmagic_camss_axi_clk.c),
	CLK_LOOKUP("camss_vfe0_clk", camss_vfe0_clk.c),
	CLK_LOOKUP("camss_vfe_ahb_clk", camss_vfe_ahb_clk.c),
	CLK_LOOKUP("camss_vfe0_stream_clk", camss_vfe0_stream_clk.c),
	CLK_LOOKUP("camss_ispif_ahb_clk", camss_ispif_ahb_clk.c),
	CLK_LOOKUP("csi2phytimer_clk_src", csi2phytimer_clk_src.c),
	CLK_LOOKUP("csi2_clk_src", csi2_clk_src.c),
	CLK_LOOKUP("camss_csi2_clk", camss_csi2_clk.c),
	CLK_LOOKUP("camss_csi2phy_clk", camss_csi2phy_clk.c),
	CLK_LOOKUP("csiphy2_3p_clk_src", csiphy2_3p_clk_src.c),
	CLK_LOOKUP("camss_csiphy2_3p_clk", camss_csiphy2_3p_clk.c),
	CLK_LOOKUP("camss_csiphy1_3p_clk", camss_csiphy1_3p_clk.c),
	CLK_LOOKUP("csiphy1_3p_clk_src", csiphy1_3p_clk_src.c),
	CLK_LOOKUP("camss_csi2rdi_clk", camss_csi2rdi_clk.c),
	CLK_LOOKUP("camss_csi2pix_clk", camss_csi2pix_clk.c),
	CLK_LOOKUP("camss_cpp_vbif_ahb_clk", camss_cpp_vbif_ahb_clk.c),
	CLK_LOOKUP("camss_vfe_axi_clk", camss_vfe_axi_clk.c),
	CLK_LOOKUP("camss_cpp_axi_clk", camss_cpp_axi_clk.c),
	CLK_LOOKUP("camss_jpeg_axi_clk", camss_jpeg_axi_clk.c),
	CLK_LOOKUP("camss_csi2_ahb_clk", camss_csi2_ahb_clk.c),
	CLK_LOOKUP("camss_csi2phytimer_clk", camss_csi2phytimer_clk.c),
	CLK_LOOKUP("camss_csi1phytimer_clk", camss_csi1phytimer_clk.c),
	CLK_LOOKUP("smmu_vfe_axi_clk", smmu_vfe_axi_clk.c),
	CLK_LOOKUP("smmu_vfe_ahb_clk", smmu_vfe_ahb_clk.c),
	CLK_LOOKUP("mmss_mmagic_cfg_ahb_clk", mmss_mmagic_cfg_ahb_clk.c),
#endif
};

void platform_clock_init(void)
{
	clk_init(msm_msm8996_clocks, ARRAY_SIZE(msm_msm8996_clocks));
}
