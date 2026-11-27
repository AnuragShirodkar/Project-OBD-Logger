#include "usb_names.h"
#include "imxrt.h"
#include <stdio.h>

char loggerName[16];
char loggerUsbName[32];
struct usb_string_descriptor_struct usb_string_product_name = {
  2, 3, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};
struct usb_string_descriptor_struct usb_string_manufacturer_name = {
  46, 3, {'S','k','i','l','l','i','c','o','n',' ','T','e','c','h','n','o','l','o','g','i','e','s'}
};

// Teensy startup invokes this after RAM initialization and before USB enumeration.
void startup_middle_hook(void) {
  unsigned number = HW_OCOTP_MAC0 & 0xFFFFFFu;
  snprintf(loggerName, sizeof loggerName, "OBD_%u", number);
  int length = snprintf(loggerUsbName, sizeof loggerUsbName, "SkiDrive_%u", number);
  for (int i = 0; i < length; ++i) usb_string_product_name.wString[i] = loggerUsbName[i];
  usb_string_product_name.bLength = 2 + 2 * length;
}
