/*
 * author : Shuichi TAKANO
 * since  : Thu Jul 29 2021 03:39:11
 */

#include <tusb.h>
#include <stdio.h>
#include "gamepad.h"

namespace io
{
    namespace
    {
        GamePadState currentGamePad_[2];
    }

    const GamePadState &getCurrentGamePadState(int i)
    {
        return currentGamePad_[i];
    }
}

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_REPORT 4

    namespace
    {
        uint8_t _report_count[CFG_TUH_HID];
        tuh_hid_report_info_t _report_info_arr[CFG_TUH_HID][MAX_REPORT];
    }

    void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len)
    {
        printf("HID device address = %d, instance = %d is mounted\n", dev_addr, instance);

        const char *protocol_str[] = {"None", "Keyboard", "Mouse"}; // hid_protocol_type_t
        uint8_t const interface_protocol = tuh_hid_interface_protocol(dev_addr, instance);

        // Parse report descriptor with built-in parser
        _report_count[instance] = tuh_hid_parse_report_descriptor(_report_info_arr[instance], MAX_REPORT, desc_report, desc_len);
        printf("HID has %u reports and interface protocol = %d:%s\n", _report_count[instance],
               interface_protocol, protocol_str[interface_protocol]);
    }

    void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
    {
        printf("HID device address = %d, instance = %d is unmounted\n", dev_addr, instance);
    }

    void tuh_hid_report_received_cb([[maybe_unused]] uint8_t dev_addr,
                                    uint8_t instance, uint8_t const *report, uint16_t len)
    {
        uint8_t const rpt_count = _report_count[instance];
        tuh_hid_report_info_t *rpt_info_arr = _report_info_arr[instance];
        tuh_hid_report_info_t *rpt_info = NULL;

        if (rpt_count == 1 && rpt_info_arr[0].report_id == 0)
        {
            // Simple report without report ID as 1st byte
            rpt_info = &rpt_info_arr[0];
        }
        else
        {
            // Composite report, 1st byte is report ID, data starts from 2nd byte
            uint8_t const rpt_id = report[0];

            // Find report id in the arrray
            for (uint8_t i = 0; i < rpt_count; i++)
            {
                if (rpt_id == rpt_info_arr[i].report_id)
                {
                    rpt_info = &rpt_info_arr[i];
                    break;
                }
            }

            report++;
            len--;
        }

        if (!rpt_info)
        {
            printf("Couldn't find the report info for this report !\n");
            return;
        }

        //        printf("usage %d, %d\n", rpt_info->usage_page, rpt_info->usage);

        if (rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP)
        {
            switch (rpt_info->usage)
            {
            case HID_USAGE_DESKTOP_KEYBOARD:
                TU_LOG1("HID receive keyboard report\n");
                // Assume keyboard follow boot report layout
                //                process_kbd_report((hid_keyboard_report_t const *)report);
                break;

            case HID_USAGE_DESKTOP_MOUSE:
                TU_LOG1("HID receive mouse report\n");
                // Assume mouse follow boot report layout
                //                process_mouse_report((hid_mouse_report_t const *)report);
                break;

            case HID_USAGE_DESKTOP_JOYSTICK:
            {
                // TU_LOG1("HID receive joystick report\n");
                struct JoyStickReport
                {
                    uint8_t axis[3];
                    uint8_t buttons;
                    // 実際のところはしらん
                };
                auto *rep = reinterpret_cast<const JoyStickReport *>(report);
                //                printf("x %d y %d button %02x\n", rep->axis[0], rep->axis[1], rep->buttons);
                auto &gp = io::currentGamePad_[0];
                gp.axis[0] = rep->axis[0];
                gp.axis[1] = rep->axis[1];
                gp.axis[2] = rep->axis[2];
                gp.buttons = rep->buttons;
            }
            break;

            case HID_USAGE_DESKTOP_GAMEPAD:
                TU_LOG1("HID receive gamepad report\n");

                break;

            default:
                break;
            }
        }
    }

#ifdef __cplusplus
}
#endif