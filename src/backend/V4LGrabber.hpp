/**
 * @file ${FILE_NAME}
 *
 * @brief ToDo
 *
 * @author Benedikt-Alexander Mokroß <oatpp@bamkrs.de>
 */

#ifndef V4LGRABBER_HPP_
#define V4LGRABBER_HPP_

#include <cstdlib>
#include <atomic>
#include <mutex>
#include <thread>

class V4LGrabber {

 public:
  V4LGrabber(const char* device, void(*imagecb)(void*, const void*, int), void* userdata);
  ~V4LGrabber();

  int stop_capturing();
  int start_capturing();

  static int testDevice(const char* device);

 private:

  enum io_method {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
  };

  struct buffer {
    void   *start;
    size_t  length;
  };

  const char *m_devname;
  enum io_method m_io = IO_METHOD_MMAP;
  int m_fd = -1;
  struct buffer *m_buffers;
  unsigned int m_nbuffers;
  std::atomic_bool m_initdone;
  std::atomic_bool m_capturing;
  std::thread m_mainloop;
  std::mutex m_mutex;

  void (*m_imagecb)(void*, const void*,int);
  void *m_userdata;

  static int xioctl(int fh, unsigned long int request, void *arg);
  static void mainloop(V4LGrabber *parent);

  int errno_report(const char *s);
  int read_frame();
  int uninit_device();
  int init_read(unsigned int buffer_size);
  int init_mmap();
  int init_userp(unsigned int buffer_size);
  int init_device();
  int stream_on();
  int stream_off();
  int close_device();
  int open_device();

  static const char* TAG;

};

#endif //V4LGRABBER_HPP_
