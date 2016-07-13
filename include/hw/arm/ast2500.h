/*
 * ASPEED AST2500 SoC
 *
 * Joel Stanley <joel@jms.id.au>
 *
 * Copyright 2016 IBM Corp.
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#ifndef AST2500_H
#define AST2500_H

#include "hw/arm/arm.h"
#include "hw/intc/aspeed_vic.h"
#include "hw/misc/aspeed_scu.h"
#include "hw/timer/aspeed_timer.h"
#include "hw/i2c/aspeed_i2c.h"
#include "hw/ssi/aspeed_smc.h"
#include "hw/misc/aspeed_sdmc.h"
#include "hw/net/ftgmac100.h"

typedef struct AST2500State {
    /*< private >*/
    DeviceState parent;

    /*< public >*/
    ARMCPU *cpu;
    MemoryRegion iomem;
    AspeedVICState vic;
    AspeedTimerCtrlState timerctrl;
    AspeedSCUState scu;
    AspeedI2CState i2c;
    AspeedSMCState smc;
    AspeedSMCState spi;
    AspeedSDMCState sdmc;
    Ftgmac100State ftgmac100;
} AST2500State;

#define TYPE_AST2500 "ast2500"
#define AST2500(obj) OBJECT_CHECK(AST2500State, (obj), TYPE_AST2500)

#define AST2500_SDRAM_BASE       0x80000000

#endif /* AST2500_H */
