#include "usb_names.h"
#include "imxrt.h"
#include <stdio.h>

char loggerName[16];
struct usb_string_descriptor_struct usb_string_product_name = {
  2, 3, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};
struct usb_string_descriptor_struct usb_string_manufacturer_name = {
  22, 3, {'O','B','D',' ','l','o','g','g','e','r'}
};

// Teensy startup invokes this after RAM initialization and before USB enumeration.
void startup_middle_hook(void) {
  unsigned number = HW_OCOTP_MAC0 & 0xFFFFFFu;
  int length = snprintf(loggerName, sizeof loggerName, "OBD_%u", number);
  for (int i = 0; i < length; ++i) usb_string_product_name.wString[i] = loggerName[i];
  usb_string_product_name.bLength = 2 + 2 * length;
}
