/**
 * Copyright (C) 2009 Ubixum, Inc. 
 * Copyright (C) 2020 William Woods
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 **/

#include <cstdio>
#include <cassert>
#include <libusb-1.0/libusb.h>

#include "transpose.h"

const uint16_t VID = 0x04b4;
const uint16_t PID = 0x8613;

const unsigned char EP2_IN_ADDR = LIBUSB_ENDPOINT_IN | 0x02;

int main(int argc, char* argv[]) {

  int rv;

  libusb_context* ctx;
  libusb_init(&ctx);
  
  // TODO: this is a "convenience function" not intended for final use
  // because if multiple devices share PID/VID then it only opens the first.
  //
  // Should be using lib_usb_get_device_list at some point before, and opening
  // that way.
  libusb_device_handle* hndl = libusb_open_device_with_vid_pid(ctx, VID, PID);
  if (hndl == NULL) {
    printf("Could not find device %4x:%4x\n", VID, PID);
    return -1;
  }

  rv = libusb_set_auto_detach_kernel_driver(hndl, 1);
  if (rv != 0) {
    printf("Found device, but could not auto-detach kernel driver! Continuing anway...\n");
  }

  // "instruct the underlying operating system that your application wishes to take ownership of the interface."
  rv = libusb_claim_interface(hndl, 0);
  if (rv != 0) {
    printf("Found device, but could not claim interface!\n");
    return -1;
  }

  rv = libusb_set_interface_alt_setting(hndl, 0, 0);
  if (rv != 0) {
    printf("Found device and claimed interface but could not set alt interface!\n");
    return -1;
  }

  // send control transfer if we have been called with them as args
  if (argc > 1) {
    // host to device | vendor command | recipient is an endpoint
    uint8_t bmRequestType = (0 << 7) | (2 << 5) | (2 << 0); 
    // we'll use [0xB0..0xB3] for configuring channels 1-4
    uint8_t bRequest = 0xB0;
    uint8_t data[12] = "hello world";

    rv = libusb_control_transfer(
      hndl,
      bmRequestType,
      bRequest,
      0x0000,
      0x0000,
      data,
      12,
      100
    );
  } else {

    unsigned char buf2[504];
    int nTransferred = 0;
    // TODO: this is blocking; need to look into the async version
    rv = libusb_bulk_transfer(hndl, EP2_IN_ADDR, buf2, sizeof(buf2), &nTransferred, 500);
    if (rv) {
      printf ( "IN Transfer failed: %d, transferred: %d\n", rv, nTransferred );
      return rv;
    }

    // See what we read in
    printf("Read in %d bytes\n", nTransferred);
  /*
    // the raw hex we got
    for (int i=0;i<nTransferred;++i) {
      if (i % 12 == 0) printf("\n");
      printf ( "%02x ", buf2[i] );
    }
  */
    printf("\n");
    // transposed so we can see what each channel saw
    printf("n\tCH0\tCH1\tCH2\tCH3\tCH4\tCH5\tCH6\tCH7\n");
    for (int i = 0; i < 504/12; i++) {
      uint16_t channels[8];
      transpose_ADC_reading(buf2 + i*12, channels, false);
      printf("%d:\t", i);
      for (int j = 0; j < 8; j++)
        printf("%04x\t", channels[j]);
      printf("\n");
    }
  }

  return 0;
}
