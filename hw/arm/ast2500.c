/*
 * AST2500 SoC
 *
 * Andrew Jeffery <andrew@aj.id.au>
 * Jeremy Kerr <jk@ozlabs.org>
 * Joel Stanley <joel@jms.id.au>
 *
 * Copyright 2016 IBM Corp.
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "qemu/log.h"
#include "cpu.h"
#include "exec/address-spaces.h"
#include "hw/arm/ast2500.h"
#include "hw/char/serial.h"

#define AST2500_UART_5_BASE      0x00184000
#define AST2500_IOMEM_SIZE       0x00200000
#define AST2500_IOMEM_BASE       0x1E600000
#define AST2500_FMC_BASE         0X1E620000
#define AST2500_SPI_BASE         0X1E630000
#define AST2500_VIC_BASE         0x1E6C0000
#define AST2500_SDMC_BASE        0x1E6E0000
#define AST2500_SCU_BASE         0x1E6E2000
#define AST2500_TIMER_BASE       0x1E782000
#define AST2500_I2C_BASE         0x1E78A000
#define AST2500_ETH1_BASE        0x1E660000
#define AST2500_ETH2_BASE        0x1E680000

#define AST2500_FMC_FLASH_BASE   0x20000000
#define AST2500_SPI_FLASH_BASE   0x30000000

static const int uart_irqs[] = { 9, 32, 33, 34, 10 };
static const int timer_irqs[] = { 16, 17, 18, 35, 36, 37, 38, 39, };

/*
 * IO handlers: simply catch any reads/writes to IO addresses that aren't
 * handled by a device mapping.
 */

static uint64_t ast2500_io_read(void *p, hwaddr offset, unsigned size)
{
    qemu_log_mask(LOG_UNIMP, "%s: 0x%" HWADDR_PRIx " [%u]\n",
                  __func__, offset, size);
    return 0;
}

static void ast2500_io_write(void *opaque, hwaddr offset, uint64_t value,
                unsigned size)
{
    qemu_log_mask(LOG_UNIMP, "%s: 0x%" HWADDR_PRIx " <- 0x%" PRIx64 " [%u]\n",
                  __func__, offset, value, size);
}

static const MemoryRegionOps ast2500_io_ops = {
    .read = ast2500_io_read,
    .write = ast2500_io_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void ast2500_init(Object *obj)
{
    AST2500State *s = AST2500(obj);

    s->cpu = cpu_arm_init("arm1176");

    object_initialize(&s->vic, sizeof(s->vic), TYPE_ASPEED_VIC);
    object_property_add_child(obj, "vic", OBJECT(&s->vic), NULL);
    qdev_set_parent_bus(DEVICE(&s->vic), sysbus_get_default());

    object_initialize(&s->timerctrl, sizeof(s->timerctrl), TYPE_ASPEED_TIMER);
    object_property_add_child(obj, "timerctrl", OBJECT(&s->timerctrl), NULL);
    qdev_set_parent_bus(DEVICE(&s->timerctrl), sysbus_get_default());

    object_initialize(&s->scu, sizeof(s->scu), TYPE_ASPEED_SCU);
    object_property_add_child(obj, "scu", OBJECT(&s->scu), NULL);
    qdev_set_parent_bus(DEVICE(&s->scu), sysbus_get_default());
    qdev_prop_set_uint32(DEVICE(&s->scu), "silicon-rev",
                         AST2500_A1_SILICON_REV);
    object_property_add_alias(obj, "hw-strap1", OBJECT(&s->scu),
                              "hw-strap1", &error_abort);
    object_property_add_alias(obj, "hw-strap2", OBJECT(&s->scu),
                              "hw-strap2", &error_abort);

    object_initialize(&s->i2c, sizeof(s->i2c), TYPE_ASPEED_I2C);
    object_property_add_child(obj, "i2c", OBJECT(&s->i2c), NULL);
    qdev_set_parent_bus(DEVICE(&s->i2c), sysbus_get_default());

    object_initialize(&s->smc, sizeof(s->smc), "aspeed.smc.fmc");
    object_property_add_child(obj, "smc", OBJECT(&s->smc), NULL);
    qdev_set_parent_bus(DEVICE(&s->smc), sysbus_get_default());

    object_initialize(&s->spi, sizeof(s->spi), "aspeed.smc.spi");
    object_property_add_child(obj, "spi", OBJECT(&s->spi), NULL);
    qdev_set_parent_bus(DEVICE(&s->spi), sysbus_get_default());

    object_initialize(&s->sdmc, sizeof(s->sdmc), TYPE_ASPEED_SDMC);
    object_property_add_child(obj, "sdmc", OBJECT(&s->sdmc), NULL);
    qdev_set_parent_bus(DEVICE(&s->sdmc), sysbus_get_default());
    qdev_prop_set_uint32(DEVICE(&s->sdmc), "silicon-rev",
                         AST2500_A1_SILICON_REV);

    object_initialize(&s->ftgmac100, sizeof(s->ftgmac100), TYPE_FTGMAC100);
    object_property_add_child(obj, "ftgmac100", OBJECT(&s->ftgmac100), NULL);
    qdev_set_parent_bus(DEVICE(&s->ftgmac100), sysbus_get_default());
}

static void ast2500_realize(DeviceState *dev, Error **errp)
{
    int i;
    AST2500State *s = AST2500(dev);
    Error *err = NULL, *local_err = NULL;

    /* IO space */
    memory_region_init_io(&s->iomem, NULL, &ast2500_io_ops, NULL,
            "ast2500.io", AST2500_IOMEM_SIZE);
    memory_region_add_subregion_overlap(get_system_memory(), AST2500_IOMEM_BASE,
            &s->iomem, -1);

    /* VIC */
    object_property_set_bool(OBJECT(&s->vic), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->vic), 0, AST2500_VIC_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->vic), 0,
                       qdev_get_gpio_in(DEVICE(s->cpu), ARM_CPU_IRQ));
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->vic), 1,
                       qdev_get_gpio_in(DEVICE(s->cpu), ARM_CPU_FIQ));

    /* Timer */
    object_property_set_bool(OBJECT(&s->timerctrl), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->timerctrl), 0, AST2500_TIMER_BASE);
    for (i = 0; i < ARRAY_SIZE(timer_irqs); i++) {
        qemu_irq irq = qdev_get_gpio_in(DEVICE(&s->vic), timer_irqs[i]);
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->timerctrl), i, irq);
    }

    /* SCU */
    object_property_set_bool(OBJECT(&s->scu), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
	return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->scu), 0, AST2500_SCU_BASE);

    /* UART - attach an 8250 to the IO space as our UART5 */
    if (serial_hds[0]) {
        qemu_irq uart5 = qdev_get_gpio_in(DEVICE(&s->vic), uart_irqs[4]);
        serial_mm_init(&s->iomem, AST2500_UART_5_BASE, 2,
                       uart5, 38400, serial_hds[0], DEVICE_LITTLE_ENDIAN);
    }

   /* I2C */
    object_property_set_bool(OBJECT(&s->i2c), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->i2c), 0, AST2500_I2C_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->i2c), 0,
                       qdev_get_gpio_in(DEVICE(&s->vic), 12));

    /* add a TMP423 temperature sensor */
    dev = i2c_create_slave(aspeed_i2c_get_bus(DEVICE(&s->i2c), 2),
                           "tmp423", 0x4c);
    object_property_set_int(OBJECT(dev), 31000, "temperature0", &err);
    object_property_set_int(OBJECT(dev), 28000, "temperature1", &err);
    object_property_set_int(OBJECT(dev), 20000, "temperature2", &err);
    object_property_set_int(OBJECT(dev), 110000, "temperature3", &err);

    /* A basic RTC without alarms */
    i2c_create_slave(aspeed_i2c_get_bus(DEVICE(&s->i2c), 0), "ds1338", 0x68);

    /* SMC */
    object_property_set_int(OBJECT(&s->smc), 1, "num-cs", &err);
    object_property_set_bool(OBJECT(&s->smc), true, "realized", &local_err);
    error_propagate(&err, local_err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->smc), 0, AST2500_FMC_BASE);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->smc), 1, AST2500_FMC_FLASH_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->smc), 0,
                       qdev_get_gpio_in(DEVICE(&s->vic), 19));

    /* SPI */
    object_property_set_int(OBJECT(&s->spi), 1, "num-cs", &err);
    object_property_set_bool(OBJECT(&s->spi), true, "realized", &local_err);
    error_propagate(&err, local_err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi), 0, AST2500_SPI_BASE);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi), 1, AST2500_SPI_FLASH_BASE);

    /* SDMC - SDRAM Memory Controller */
    object_property_set_bool(OBJECT(&s->sdmc), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->sdmc), 0, AST2500_SDMC_BASE);

    /* Net */
    qdev_set_nic_properties(DEVICE(&s->ftgmac100), &nd_table[0]);
    object_property_set_bool(OBJECT(&s->ftgmac100), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->ftgmac100), 0, AST2500_ETH1_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->ftgmac100), 0,
                       qdev_get_gpio_in(DEVICE(&s->vic), 2));
}

static void ast2500_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = ast2500_realize;

    /*
     * Reason: creates an ARM CPU, thus use after free(), see
     * arm_cpu_class_init()
     */
    dc->cannot_destroy_with_object_finalize_yet = true;
}

static const TypeInfo ast2500_type_info = {
    .name = TYPE_AST2500,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AST2500State),
    .instance_init = ast2500_init,
    .class_init = ast2500_class_init,
};

static void ast2500_register_types(void)
{
    type_register_static(&ast2500_type_info);
}

type_init(ast2500_register_types)
