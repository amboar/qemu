/*
 * Faraday FTGMAC100 Gigabit Ethernet
 *
 * Copyright (C) 2016 IBM Corp.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef FTGMAC100_H
#define FTGMAC100_H

#define TYPE_FTGMAC100 "ftgmac100"
#define FTGMAC100(obj) OBJECT_CHECK(Ftgmac100State, (obj), TYPE_FTGMAC100)

#include "hw/sysbus.h"
#include "net/net.h"

typedef struct Ftgmac100State {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    NICState *nic;
    NICConf conf;
    qemu_irq irq;
    MemoryRegion iomem;

    uint32_t irq_state;
    uint32_t isr;
    uint32_t ier;
    uint32_t rx_enabled;
    uint32_t rx_ring;
    uint32_t rbsr;
    uint32_t rx_descriptor;
    uint32_t tx_ring;
    uint32_t tx_descriptor;
    uint32_t maccr;
    uint32_t phycr;
    uint32_t phydata;
    uint32_t aptcr;

    uint32_t phy_status;
    uint32_t phy_control;
    uint32_t phy_advertise;
    uint32_t phy_int;
    uint32_t phy_int_mask;

} Ftgmac100State;

#endif
