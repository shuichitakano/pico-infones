#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/divider.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include <hardware/sync.h>
#include <pico/multicore.h>
#include <memory>
#include <math.h>
#include <util/dump_bin.h>
#include <string.h>
#include <stdarg.h>
#include <algorithm>

#include <InfoNES.h>
#include <InfoNES_System.h>
#include <InfoNES_pAPU.h>

#include <dvi/dvi.h>
#include <tusb.h>
#include <gamepad.h>

const uint LED_PIN = PICO_DEFAULT_LED_PIN;

namespace
{
    constexpr dvi::Config dviConfig_ = {
        .pinTMDS = {10, 12, 14},
        .pinClock = 8,
        .invert = true,
    };

    std::unique_ptr<dvi::DVI> dvi_;
}

const WORD __not_in_flash_func(NesPalette)[64] = {
    0x39ce, 0x1071, 0x0015, 0x2013, 0x440e, 0x5402, 0x5000, 0x3c20,
    0x20a0, 0x0100, 0x0140, 0x00e2, 0x0ceb, 0x0000, 0x0000, 0x0000,
    0x5ef7, 0x01dd, 0x10fd, 0x401e, 0x5c17, 0x700b, 0x6ca0, 0x6521,
    0x45c0, 0x0240, 0x02a0, 0x0247, 0x0211, 0x0000, 0x0000, 0x0000,
    0x7fff, 0x1eff, 0x2e5f, 0x223f, 0x79ff, 0x7dd6, 0x7dcc, 0x7e67,
    0x7ae7, 0x4342, 0x2769, 0x2ff3, 0x03bb, 0x0000, 0x0000, 0x0000,
    0x7fff, 0x579f, 0x635f, 0x6b3f, 0x7f1f, 0x7f1b, 0x7ef6, 0x7f75,
    0x7f94, 0x73f4, 0x57d7, 0x5bf9, 0x4ffe, 0x0000, 0x0000, 0x0000};

int InfoNES_Menu() { return 0; }

void InfoNES_PadState(DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem)
{
    for (int i = 0; i < 2; ++i)
    {
        auto &dst = i == 0 ? *pdwPad1 : *pdwPad2;
        auto &gp = io::getCurrentGamePadState(i);
        dst = 0;
        if (gp.axis[0] < 64)
        {
            // left
            dst |= 1 << 6;
        }
        else if (gp.axis[0] > 192)
        {
            // right
            dst |= 1 << 7;
        }
        else if (gp.axis[1] < 64)
        {
            // up
            dst |= 1 << 4;
        }
        else if (gp.axis[1] > 192)
        {
            // down
            dst |= 1 << 5;
        }

        if (gp.buttons & 0x40)
        {
            // select
            dst |= 1 << 2;
        }
        if (gp.buttons & 0x80)
        {
            // start
            dst |= 1 << 3;
        }
        if (gp.buttons & 0x01)
        {
            // A
            dst |= 1 << 0;
        }
        if (gp.buttons & 0x02)
        {
            // B
            dst |= 1 << 1;
        }
    }

    //    printf("pad = %02x %02x\n", *pdwPad1, *pdwPad2);
    // *pdwPad1 = 0;
    // *pdwPad2 = 0;
    *pdwSystem = 0;
}

void InfoNES_MessageBox(const char *pszMsg, ...)
{
    printf("[MSG]");
    va_list args;
    va_start(args, pszMsg);
    vprintf(pszMsg, args);
    va_end(args);
    printf("\n");
}

bool parseROM(const uint8_t *nesFile)
{
    memcpy(&NesHeader, nesFile, sizeof(NesHeader));
    if (memcmp(NesHeader.byID, "NES\x1a", 4) != 0)
    {
        return false;
    }

    nesFile += sizeof(NesHeader);

    memset(SRAM, 0, SRAM_SIZE);

    if (NesHeader.byInfo1 & 4)
    {
        memcpy(&SRAM[0x1000], nesFile, 512);
        nesFile += 512;
    }

    auto romSize = NesHeader.byRomSize * 0x4000;
    ROM = (BYTE *)nesFile;
    nesFile += romSize;

    if (NesHeader.byVRomSize > 0)
    {
        auto vromSize = NesHeader.byVRomSize * 0x2000;
        VROM = (BYTE *)nesFile;
        nesFile += vromSize;
    }

    return true;
}

void InfoNES_ReleaseRom()
{
    ROM = nullptr;
    VROM = nullptr;
}

void InfoNES_SoundInit()
{
}

int InfoNES_SoundOpen(int samples_per_sync, int sample_rate)
{
    return 0;
}

void InfoNES_SoundClose()
{
}

void __not_in_flash_func(InfoNES_SoundOutput)(int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5)
{
    while (samples)
    {
        auto &ring = dvi_->getAudioRingBuffer();
        auto n = std::min<int>(samples, ring.getWritableSize());
        if (!n)
        {
            return;
        }
        auto p = ring.getWritePointer();

        int ct = n;
        while (ct--)
        {
            int w1 = *wave1++;
            int w2 = *wave2++;
            int w3 = *wave3++;
            int w4 = *wave4++;
            int w5 = *wave5++;
            int l = w1 * 6 + w2 * 3 + w3 * 5 + w4 * 3 + w5 * 2;
            int r = w1 * 3 + w2 * 6 + w3 * 5 + w4 * 3 + w5 * 2;
            *p++ = {static_cast<short>(l), static_cast<short>(r)};

            // pulse_out = 0.00752 * (pulse1 + pulse2)
            // tnd_out = 0.00851 * triangle + 0.00494 * noise + 0.00335 * dmc

            // 0.00851/0.00752 = 1.131648936170213
            // 0.00494/0.00752 = 0.6569148936170213
            // 0.00335/0.00752 = 0.4454787234042554

            // 0.00752/0.00851 = 0.8836662749706228
            // 0.00494/0.00851 = 0.5804935370152762
            // 0.00335/0.00851 = 0.3936545240893067
        }

        ring.advanceWritePointer(n);
        samples -= n;
    }
}

extern WORD PC;

void InfoNES_LoadFrame()
{
    gpio_put(LED_PIN, (dvi_->getFrameCounter() / 60) & 1);
    //    printf("%04x\n", PC);

    tuh_task();
}

namespace
{
    dvi::DVI::LineBuffer *currentLineBuffer_{};
}

void __not_in_flash_func(InfoNES_PreDrawLine)(int line)
{
    auto b = dvi_->getLineBuffer();
    InfoNES_SetLineBuffer(b->data() + 32, b->size());
    (*b)[319] = line + dvi_->getFrameCounter();

    currentLineBuffer_ = b;
}

void __not_in_flash_func(InfoNES_PostDrawLine)()
{
    assert(currentLineBuffer_);
    dvi_->setLineBuffer(currentLineBuffer_);
    currentLineBuffer_ = nullptr;
}

void __not_in_flash_func(core1_main)()
{
    dvi_->registerIRQThisCore();
    dvi_->waitForValidLine();

    dvi_->start();
    dvi_->loopScanBuffer15bpp();
}

int main()
{
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(10);
    //    set_sys_clock_khz(251750, true);
    set_sys_clock_khz(252000, true);

    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    tusb_init();

    static constexpr uintptr_t NES_FILE_ADDR = 0x10100000;
    static constexpr uintptr_t SRAM_FILE_ADDR = NES_FILE_ADDR - SRAM_SIZE;

    //    util::dumpMemory((void *)NES_FILE_ADDR, 1024);
    if (!parseROM((const uint8_t *)NES_FILE_ADDR))
    {
        printf("NES file parse error.\n");
        return -1;
    }

    if (InfoNES_Reset() < 0)
    {
        printf("NES reset error.\n");
        return -1;
    }

    //
    dvi_ = std::make_unique<dvi::DVI>(pio0, &dviConfig_, dvi::getTiming640x480p60Hz());
    //    dvi_->setAudioFreq(48000, 25200, 6144);
    dvi_->setAudioFreq(44100, 28000, 6272);
    dvi_->allocateAudioBuffer(1024);

#if 0
    int32_t dividend = 123456;
    int32_t divisor = -321;
    // This is the recommended signed fast divider for general use.
    divmod_result_t result = hw_divider_divmod_s32(dividend, divisor);
    printf("%d/%d = %d remainder %d\n", dividend, divisor, to_quotient_s32(result), to_remainder_s32(result));
    // This is the recommended unsigned fast divider for general use.
    int32_t udividend = 123456;
    int32_t udivisor = 321;
    divmod_result_t uresult = hw_divider_divmod_u32(udividend, udivisor);
    printf("%d/%d = %d remainder %d\n", udividend, udivisor, to_quotient_u32(uresult), to_remainder_u32(uresult));

    // Interpolator example code
    interp_config cfg = interp_default_config();
    // Now use the various interpolator library functions for your use case
    // e.g. interp_config_clamp(&cfg, true);
    //      interp_config_shift(&cfg, 2);
    // Then set the config
    interp_set_config(interp0, 0, &cfg);
#endif

    multicore_launch_core1(core1_main);

    InfoNES_Main();

    return 0;
}
