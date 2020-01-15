#ifndef RGBLEDS_H_
#define RGBLEDS_H_
#endif

typedef enum device{
    BLUE = 0,
    GREEN = 1,
    RED = 2
} unit_desig;

static void LP3943_ColorSet(uint32_t unit, uint32_t PWM_DATA);

void LP3943_LedModeSet(uint32_t unit, uint16_t LED_DATA);

void init_RGBLEDS();

