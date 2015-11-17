// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/space_mouse.h>

#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#include <fcntl.h>

#if WIN32
  #include <windows.h>
  #include <hidapi.h>

  #define EV_KEY                  0x01
  #define EV_REL                  0x02
  #define EV_ABS                  0x03

  //typedef struct timeval {
  //  long tv_sec;
  //  long tv_usec;
  //} timeval;

  struct input_event {
    struct timeval time;
    unsigned short type;
    unsigned short code;
    int value;
  };
#else
  #include <unistd.h>
  #include <linux/input.h>
#endif

namespace lamure 
{
  namespace ren
  {

    struct SpaceMouse::impl_t {
#if WIN32
      hid_device* handle_;
      impl_t() : handle_(nullptr) {}
#else
      impl_t() : handle_(-1) {}
      int handle_;
#endif
    };

    ///////////////////////////////////////////////////////////////////////////
    SpaceMouse::SpaceMouse()
#if WIN32 
      : impl_(std::make_unique<impl_t>()), 
#else
      : impl_(std::unique_ptr<impl_t>(new impl_t)), 
#endif
        shutdown_flag_(false)
    {}

    ///////////////////////////////////////////////////////////////////////////
    SpaceMouse::~SpaceMouse() 
    {
      try {
        Stop();
      }
      catch ( std::exception& e) 
      {
        std::cerr << "SpaceMouse::~SpaceMouse() " << e.what() << std::endl;
      }
      
    }

    ///////////////////////////////////////////////////////////////////////////
    void SpaceMouse::Run()
    {
      Stop();

      try {
#if WIN32
        if (hid_init()) {
          std::runtime_error("Cannot init HID.");
        }

        struct hid_device_info *devs, *cur_dev;
        devs = hid_enumerate(0x0, 0x0);
        cur_dev = devs;

        unsigned short vendour_id = 0;
        unsigned short product_id = 0;
        wchar_t* serial;

        while (cur_dev) {
          std::cout << "Search HID device ... \n";
          std::cout << "Testing device: " << cur_dev->vendor_id << " " << cur_dev->product_id << " " << cur_dev->path << " " << cur_dev->serial_number << std::endl;
          std::cout << "  Manufacturer: " << cur_dev->manufacturer_string << std::endl;
          std::cout << "  Product:      " << cur_dev->product_string << std::endl;
          std::cout << "  Release:      " << cur_dev->release_number << std::endl;
          std::cout << "  Interface:    " << cur_dev->interface_number << std::endl;

          if (cur_dev->product_string)
          {
            std::wstring product(cur_dev->product_string);
            std::wstring space_nav = L"SpaceNavigator";
            if (product == space_nav) {
              std::cout << "Found SpaceNavigator." << std::endl;
              vendour_id = cur_dev->vendor_id;
              product_id = cur_dev->product_id;
              serial = cur_dev->serial_number;
            }
          }

          cur_dev = cur_dev->next;
        }

        // free devices
        hid_free_enumeration(devs);

        // try to open device using vendour and product ID
        if (vendour_id && product_id)
        {
          impl_->handle_ = hid_open(vendour_id, product_id, serial);
          if (!impl_->handle_) {
            throw std::runtime_error("Found SpaceNavigator, but device busy...");
          }
        }

#define MAX_STR 255
        wchar_t wstr[MAX_STR];

        // Read the Manufacturer String
        wstr[0] = 0x0000;
        int res = hid_get_manufacturer_string(impl_->handle_, wstr, MAX_STR);
        if (res < 0)
          printf("Unable to read manufacturer string\n");
        printf("Manufacturer String: %ls\n", wstr);

        // Read the Product String
        wstr[0] = 0x0000;
        res = hid_get_product_string(impl_->handle_, wstr, MAX_STR);
        if (res < 0)
          printf("Unable to read product string\n");
        printf("Product String: %ls\n", wstr);

        // Read the Serial Number String
        wstr[0] = 0x0000;
        res = hid_get_serial_number_string(impl_->handle_, wstr, MAX_STR);
        if (res < 0)
          printf("Unable to read serial number string\n");
        printf("Serial Number String: (%d) %ls", wstr[0], wstr);
        printf("\n");

        // Read Indexed String 1
        wstr[0] = 0x0000;
        res = hid_get_indexed_string(impl_->handle_, 1, wstr, MAX_STR);
        if (res < 0)
          printf("Unable to read indexed string 1\n");
        printf("Indexed String 1: %ls\n", wstr);

        hid_set_nonblocking(impl_->handle_, 1);

#else
        Stop();
        shutdown_flag_ = false;


        impl_->handle_ = open("/dev/input/by-id/usb-3Dconnexion_SpaceNavigator-event-joystick", O_RDONLY);
        //impl_->handle_ = open("/dev/input/by-id/usb-3Dconnexion_SpaceNavigator_for_Notebooks-event-if00", O_RDONLY);
        if (impl_->handle_ == -1) {
          std::cerr << "Failed to open input device: " << strerror(errno) << std::endl;
          return;
        }

        char name[256];
        ioctl(impl_->handle_, EVIOCGNAME(sizeof(name)), name);
        std::cout << "Connected input device: " << name << std::endl;

        int ret = ioctl(impl_->handle_, EVIOCGRAB, 1);
        if (ret != 0) {
          std::cerr << "Failed to initialize input device: " << strerror(errno) << std::endl;
          close(impl_->handle_);
          return;
        }
#endif
        thread_ = std::thread([&](){ReadLoop(); });
      }
      catch (std::exception& e) {
        std::cerr << "Error: SpaceMouse::SpaceMouse() " << e.what() << std::endl;
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    void SpaceMouse::Stop()
    {
      if (thread_.joinable())
      {
        shutdown_flag_ = true;
        thread_.join();
#if WIN32
        if (impl_->handle_) {
          hid_close(impl_->handle_);
        }
#else
        close(impl_->handle_);
#endif
      }
    }



    ///////////////////////////////////////////////////////////////////////////
    void SpaceMouse::ReadLoop()
    {
#if WIN32 
      unsigned char buf[256];
      memset(buf, 0x00, sizeof(buf));

      while (true)
      {
        int res = hid_read(impl_->handle_, buf, sizeof(buf));

        struct input_event ev = { 0, 0, 0 };

        if (res == 7)
        {
          ev.type = buf[0];

          short x = *static_cast<short*>(static_cast<void*>(&buf[1]));
          short y = *static_cast<short*>(static_cast<void*>(&buf[3]));
          short z = *static_cast<short*>(static_cast<void*>(&buf[5]));

          // TODO : not sure how to generate proper input events from these values
          // starting point would be
          // http://spacenav.sourceforge.net/spnav-win32.html


          if (ev.type == EV_KEY && event_callback_) {

            event_callback_(ev.code, ev.value);
          }
          Sleep(100);
        }
      }
#else
      const int timeout = 200;
      struct input_event ev;

      while (!shutdown_flag_) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(impl_->handle_, &rfds);

        struct timeval tm = {0, timeout * 1000};
        int ret = select(impl_->handle_ + 1, &rfds, 0, 0, &tm);

        if (ret == -1) {
          std::cerr << "Input device: select failed: " << strerror(errno) << std::endl;
          close(impl_->handle_);
          return;
        }

        if (ret > 0) {
          if (sizeof(input_event) != ::read(impl_->handle_, &ev, sizeof(input_event))) {
            std::cerr << "Input device: read failed: " << strerror(errno) << std::endl;
            close(impl_->handle_);
            return;
          }
          if (ev.type == EV_KEY) {
            //TODO: Implement button click callback
            //if (ev.code == BTN_0)
          }

          if (ev.type == EV_ABS && event_callback_) {
            event_callback_(ev.code, ev.value);
          }
        }
      }
   

#endif
    } 
  } //  namespace ren
} // namespace lamure
