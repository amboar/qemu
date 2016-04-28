#ifndef ASPEED_WDT_H
#define ASPEED_WDT_H

#include "hw/qdev.h"

#define TYPE_ASPEED_WDT "aspeed.wdt"
#define ASPEED_WDT(obj) \
    OBJECT_CHECK(AspeedWDTState, (obj), TYPE_ASPEED_WDT)
#define ASPEED_WDT_CLASS(klass) \
    OBJECT_CLASS_CHECK(AspeedWDTClass, (klass), TYPE_ASPEED_WDT)
#define ASPEED_WDT_GET_CLASS(obj) \
    OBJECT_GET_CLASS(AspeedWDTClass, (obj), TYPE_ASPEED_WDT)

#define WDT_ASPEED_INIT      0
#define WDT_ASPEED_CHANGE    1
#define WDT_ASPEED_CANCEL    2

typedef struct AspeedWDTState {
    /*< private >*/
    DeviceState parent_obj;
    QEMUTimer *timer;
    bool enabled;

    /*< public >*/
    MemoryRegion iomem;

    uint32_t reg_status;
    uint32_t reg_reload_value;
    uint32_t reg_restart;
    uint32_t reg_ctrl;
} AspeedWDTState;

#endif  /* ASPEED_WDT_H */
