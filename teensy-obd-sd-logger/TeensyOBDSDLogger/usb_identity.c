#include "usb_names.h"
// Override Teensy's weak USB descriptor symbols; actual MTP friendly name is
// independently set in the bundled OBD_MTP.cpp.
struct usb_string_descriptor_struct usb_string_product_name = {
  22, 3, {'O','B','D',' ','l','o','g','g','e','r'}
};
struct usb_string_descriptor_struct usb_string_manufacturer_name = {
  22, 3, {'O','B','D',' ','l','o','g','g','e','r'}
};
