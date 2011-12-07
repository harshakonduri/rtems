/**
 * @file
 *
 * @ingroup lpc24xx
 *
 * @brief BSP start EMC static memory configuration.
 */

/*
 * Copyright (c) 2011 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Obere Lagerstr. 30
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.com/license/LICENSE.
 *
 * $Id$
 */

#include <bsp/start-config.h>
#include <bsp/lpc24xx.h>

BSP_START_DATA_SECTION const lpc24xx_emc_static_chip_config
   lpc24xx_start_config_emc_static_chip [] = {
#if defined(LPC24XX_EMC_NUMONYX_M29W160E)
  /*
   * Static Memory 1: Numonyx M29W160EB
   *
   * 1 clock cycle = 1/72MHz = 13.9ns
   */
  {
    .chip_select = (volatile lpc_emc_static *) EMC_STA_BASE_1,
    .config = {
      /*
       * 16 bit, page mode disabled, active LOW chip select, extended wait
       * disabled, writes not protected, byte lane state LOW/LOW (!).
       */
      .config = 0x81,

      /* 1 clock cycles delay from the chip select 1 to the write enable */
      .waitwen = 0,

      /*
       * 0 clock cycles delay from the chip select 1 or address change
       * (whichever is later) to the output enable
       */
      .waitoen = 0,

      /* 7 clock cycles delay from the chip select 1 to the read access */
      .waitrd = 0x6,

      /*
       * 32 clock cycles delay for asynchronous page mode sequential accesses
       */
      .waitpage = 0x1f,

      /* 5 clock cycles delay from the chip select 1 to the write access */
      .waitwr = 0x3,

      /* 16 bus turnaround cycles */
      .waitrun = 0xf
    }
  }
#elif defined(LPC24XX_EMC_SST39VF3201)
  /* Static Memory 1: SST SST39VF3201 at 51612800Hz (tCK = 19.4ns) */
  {
    .chip_select = (volatile lpc_emc_static *) EMC_STA_BASE_0,
    .config = {
      /*
       * 16 bit, page mode disabled, active LOW chip select, extended wait
       * disabled, writes not protected, byte lane state LOW/LOW.
       */
      .config = 0x81,

      /* (n + 1) clock cycles -> 19.4ns >= 0ns (tCS, tAS) */
      .waitwen = 0,

      /* (n + 1) clock cycles -> 19.4ns >= 0ns (tOES) */
      .waitoen = 0,

      /* (n + 1) clock cycles -> 77.5ns >= 70ns (tRC) */
      .waitrd = 2,

      /* (n + 1) clock cycles -> 77.5ns >= 70ns (tRC) */
      .waitpage = 2,

      /* (n + 2) clock cycles -> 38.8ns >= 20ns (tCHZ, TOHZ) */
      .waitwr = 0,

      /* (n + 1) clock cycles -> 38.8ns >= 20ns (tCHZ, TOHZ) */
      .waitrun = 1
    }
  }
#endif
};

BSP_START_DATA_SECTION const size_t
  lpc24xx_start_config_emc_static_chip_count =
    sizeof(lpc24xx_start_config_emc_static_chip)
      / sizeof(lpc24xx_start_config_emc_static_chip [0]);