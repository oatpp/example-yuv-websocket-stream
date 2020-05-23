/**
 * @file ${FILE_NAME}
 *
 * @brief ToDo
 *
 * @author Benedikt-Alexander Mokroß <oatpp@bamkrs.de>
 */

#include "V4LGrabber.hpp"

/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 *
 *      This program is provided with the V4L2 API
 * see http://linuxtv.org/docs.php for more information
 */

#include <cstdlib>
#include <cstring>
#include <cassert>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <oatpp/core/macro/component.hpp>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#undef OATPP_LOGD
#define OATPP_LOGD(...) do{}while(false)

const char* V4LGrabber::TAG = "V4L-Grabber";

int V4LGrabber::errno_report(const char *s)
{
  OATPP_LOGE(TAG, "%s - %s error %d, %s", m_devname, s, errno, strerror(errno));
  return errno;
}

int V4LGrabber::xioctl(int fh, unsigned long int request, void *arg)
{
  int r;

  do {
    r = ioctl(fh, request, arg);
  } while (-1 == r && EINTR == errno);

  return r;
}

int V4LGrabber::read_frame()
{
  struct v4l2_buffer buf;
  unsigned int i;

  OATPP_LOGD(TAG, "%s - Reading frame", m_devname);

  switch (m_io) {
    case IO_METHOD_READ:
      if (-1 == read(m_fd, m_buffers[0].start, m_buffers[0].length)) {
        switch (errno) {
          case EAGAIN:
            return 0;

          case EIO:
            /* Could ignore EIO, see spec. */

            /* fall through */

          default:return errno_report("read");
        }
      }

      if(m_buffers[0].length > 76800) {
        OATPP_LOGD(TAG, "%s - Calling Framecb", m_devname);
        m_imagecb(m_userdata, m_buffers[0].start, m_buffers[0].length);
      } else {
        OATPP_LOGD(TAG, "%s - Discarding image with size %u", m_buffers[0].length);
      }
      break;

    case IO_METHOD_MMAP:
      CLEAR(buf);

      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;

      if (-1 == xioctl(m_fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
          case EAGAIN:
            return 0;

          case EIO:
            /* Could ignore EIO, see spec. */

            /* fall through */

          default:return errno_report("VIDIOC_DQBUF");
        }
      }

      assert(buf.index < m_nbuffers);

      if(buf.bytesused > 76800) {
        OATPP_LOGD(TAG, "%s - Calling Framecb", m_devname);
        m_imagecb(m_userdata, m_buffers[buf.index].start, buf.bytesused);
      } else {
        OATPP_LOGD(TAG, "%s - Discarding image with size %u", buf.bytesused);
      }

      if (-1 == xioctl(m_fd, VIDIOC_QBUF, &buf))
        return errno_report("VIDIOC_QBUF");
      break;

    case IO_METHOD_USERPTR:
      CLEAR(buf);

      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_USERPTR;

      if (-1 == xioctl(m_fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
          case EAGAIN:
            return 0;

          case EIO:
            /* Could ignore EIO, see spec. */

            /* fall through */

          default:return errno_report("VIDIOC_DQBUF");
        }
      }

      for (i = 0; i < m_nbuffers; ++i)
        if (buf.m.userptr == (unsigned long)m_buffers[i].start
            && buf.length == m_buffers[i].length)
          break;

      assert(i < m_nbuffers);

      OATPP_LOGD(TAG, "%s - Calling Framecb", m_devname);
      if(buf.bytesused > 76800) {
        OATPP_LOGD(TAG, "%s - Calling Framecb", m_devname);
        m_imagecb(m_userdata, (void *)buf.m.userptr, buf.bytesused);
      } else {
        OATPP_LOGD(TAG, "%s - Discarding image with size %u", buf.bytesused);
      }

      if (-1 == xioctl(m_fd, VIDIOC_QBUF, &buf))
        return errno_report("VIDIOC_QBUF");
      break;
  }

  OATPP_LOGD(TAG, "%s - Frame read", m_devname);

  return 0;
}

void V4LGrabber::mainloop(V4LGrabber *parent)
{

  OATPP_LOGD(TAG, "%s - Created mainloop", parent->m_devname);
  while (parent->m_capturing) {
    fd_set fds;
    struct timeval tv;
    int r;

    std::chrono::steady_clock::time_point nextframeon = std::chrono::steady_clock::now();
    nextframeon += std::chrono::milliseconds(75); // why only 8fps wth 75ms frametime? calculated are 13...

    FD_ZERO(&fds);
    FD_SET(parent->m_fd, &fds);

    /* Timeout. */
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    r = select(parent->m_fd + 1, &fds, NULL, NULL, &tv);

    if (-1 == r) {
      if (EINTR == errno)
        continue;
      parent->errno_report("select");
      break;
    }

    if (0 == r) {
      OATPP_LOGE(TAG, "%s - select timeout", parent->m_devname);
      parent->stream_off();
      //parent->uninit_device();
      //parent->close_device();
      //parent->open_device();
      //parent->init_device();
      if(parent->stream_on())
        break;
    }

    OATPP_LOGD(TAG, "%s - New image in mainloop", parent->m_devname);
    if (parent->read_frame())
      break;
    /* EAGAIN - continue select loop. */

    std::this_thread::sleep_until(nextframeon);
  }
  OATPP_LOGD(TAG, "%s - Mainloop stopped", parent->m_devname);
  parent->m_capturing = false;
}

int V4LGrabber::stream_on() {
  enum v4l2_buf_type type;
  int i;

  OATPP_LOGD(TAG, "%s - Starting stream", m_devname);

  switch (m_io) {
    case IO_METHOD_READ:
      /* Nothing to do. */
      break;

    case IO_METHOD_MMAP:
      for (i = 0; i < m_nbuffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == xioctl(m_fd, VIDIOC_QBUF, &buf))
          return errno_report("VIDIOC_QBUF");
      }
      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (-1 == xioctl(m_fd, VIDIOC_STREAMON, &type))
        return errno_report("VIDIOC_STREAMON");
      break;

    case IO_METHOD_USERPTR:
      for (i = 0; i < m_nbuffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = i;
        buf.m.userptr = (unsigned long)m_buffers[i].start;
        buf.length = m_buffers[i].length;

        if (-1 == xioctl(m_fd, VIDIOC_QBUF, &buf))
          return errno_report("VIDIOC_QBUF");
      }
      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (-1 == xioctl(m_fd, VIDIOC_STREAMON, &type))
        return errno_report("VIDIOC_STREAMON");
      break;
  }

  OATPP_LOGD(TAG, "%s - Started stream", m_devname);

  return 0;
}

int V4LGrabber::stream_off() {
  enum v4l2_buf_type type;

  OATPP_LOGD(TAG, "%s - Stopping stream", m_devname);

  switch (m_io) {
    case IO_METHOD_READ:
      /* Nothing to do. */
      break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (-1 == xioctl(m_fd, VIDIOC_STREAMOFF, &type))
        return errno_report("VIDIOC_STREAMOFF");
      break;
  }

  OATPP_LOGD(TAG, "%s - Stopped stream", m_devname);
  return 0;
}

int V4LGrabber::stop_capturing()
{
  std::lock_guard<std::mutex> lg(m_mutex);
  int rc;

  if(!m_capturing) {
    OATPP_LOGD(TAG, "%s - Not capturing", m_devname);
    return 0;
  }

  OATPP_LOGD(TAG, "%s - Stopping capture", m_devname);

  m_capturing = false;
  if(m_mainloop.joinable()) {
    m_mainloop.join();
  }

  rc = stream_off();

  m_initdone = false;
  uninit_device();
  close_device();

  OATPP_LOGD(TAG, "%s - Stopped capture", m_devname);
  return rc;
}

int V4LGrabber::start_capturing()
{
  std::lock_guard<std::mutex> lg(m_mutex);
  int rc;


  if(m_capturing) {
    OATPP_LOGD(TAG, "%s - Already capturing", m_devname);
    return 0;
  }

  OATPP_LOGD(TAG, "%s - Starting capture", m_devname);

  if(!m_initdone) {
    rc = open_device();
    if(rc) {
      return rc;
    }
    rc = init_device();
    if(rc) {
      return rc;
    }
    m_initdone = true;
  }

  rc = stream_on();

  if(!m_capturing) {
    if(m_mainloop.joinable()) {
      m_mainloop.join();
    }
    OATPP_LOGD(TAG, "%s - Starting mainloop", m_devname);
    m_capturing = true;
    m_mainloop = std::thread(mainloop, this);
  }

  OATPP_LOGD(TAG, "%s - Started capturing", m_devname);
  return rc;
}

int V4LGrabber::uninit_device()
{
  unsigned int i;


  OATPP_LOGD(TAG, "%s - Uniniting device", m_devname);

  switch (m_io) {
    case IO_METHOD_READ:
      free(m_buffers[0].start);
      break;

    case IO_METHOD_MMAP:
      for (i = 0; i < m_nbuffers; ++i)
        if (-1 == munmap(m_buffers[i].start, m_buffers[i].length))
          return errno_report("munmap");
      break;

    case IO_METHOD_USERPTR:
      for (i = 0; i < m_nbuffers; ++i)
        free(m_buffers[i].start);
      break;
  }

  free(m_buffers);

  m_initdone = false;

  OATPP_LOGD(TAG, "%s - Device uninited", m_devname);

  return 0;
}

int V4LGrabber::init_read(unsigned int buffer_size)
{
  OATPP_LOGD(TAG, "%s - Init read", m_devname);

  m_buffers = (buffer*)calloc(1, sizeof(*m_buffers));

  if (!m_buffers) {
    OATPP_LOGE(TAG, "%s - Out of memory", m_devname);
    return ENOMEM;
  }

  m_buffers[0].length = buffer_size;
  m_buffers[0].start = malloc(buffer_size);

  if (!m_buffers[0].start) {
    OATPP_LOGE(TAG, "%s - Out of memory", m_devname);
    return ENOMEM;
  }
  return 0;
}

int V4LGrabber::init_mmap()
{
  OATPP_LOGD(TAG, "%s - Init mmap", m_devname);

  struct v4l2_requestbuffers req;

  CLEAR(req);

  req.count = 10;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl(m_fd, VIDIOC_REQBUFS, &req)) {
    if (EINVAL == errno) {
      OATPP_LOGE(TAG, "%s - %s does not support memory mapping", m_devname, m_devname);
      return ENOTSUP;
    } else {
      return errno_report("VIDIOC_REQBUFS");
    }
  }

  if (req.count < 5) {
    OATPP_LOGE(TAG, "%s - Insufficient buffer memory on %s", m_devname,
            m_devname);
    return ENOMEM;
  }

  m_buffers = (buffer*)calloc(req.count, sizeof(*m_buffers));

  if (!m_buffers) {
    OATPP_LOGE(TAG, "%s - Out of memory", m_devname);
    return ENOMEM;
  }

  for (m_nbuffers = 0; m_nbuffers < req.count; ++m_nbuffers) {
    struct v4l2_buffer buf;

    CLEAR(buf);

    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = m_nbuffers;

    if (-1 == xioctl(m_fd, VIDIOC_QUERYBUF, &buf))
      return errno_report("VIDIOC_QUERYBUF");

    m_buffers[m_nbuffers].length = buf.length;
    m_buffers[m_nbuffers].start =
        mmap(NULL /* start anywhere */,
             buf.length,
             PROT_READ | PROT_WRITE /* required */,
             MAP_SHARED /* recommended */,
             m_fd, buf.m.offset);

    if (MAP_FAILED == m_buffers[m_nbuffers].start)
      return errno_report("mmap");
  }
  OATPP_LOGD(TAG, "%s - Inited mmap", m_devname);

  return 0;
}

int V4LGrabber::init_userp(unsigned int buffer_size)
{
  OATPP_LOGD(TAG, "%s - Initing User-Pointer", m_devname);

  struct v4l2_requestbuffers req;

  CLEAR(req);

  req.count  = 5;
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;

  if (-1 == xioctl(m_fd, VIDIOC_REQBUFS, &req)) {
    if (EINVAL == errno) {
      OATPP_LOGE(TAG, "%s does not support user pointer i/o", m_devname);
      return ENOTSUP;
    } else {
      return errno_report("VIDIOC_REQBUFS");
    }
  }

  m_buffers = (buffer*)calloc(5, sizeof(*m_buffers));

  if (!m_buffers) {
    OATPP_LOGE(TAG, "%s - Out of memory", m_devname);
    return ENOMEM;
  }

  for (m_nbuffers = 0; m_nbuffers < 4; ++m_nbuffers) {
    m_buffers[m_nbuffers].length = buffer_size;
    m_buffers[m_nbuffers].start = malloc(buffer_size);

    if (!m_buffers[m_nbuffers].start) {
      OATPP_LOGE(TAG, "%s - Out of memory", m_devname);
      return ENOMEM;
    }
  }

  OATPP_LOGD(TAG, "%s - Inited User-Pointer", m_devname);


  return 0;
}

int V4LGrabber::init_device()
{
  struct v4l2_capability cap;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  struct v4l2_format fmt;

  OATPP_LOGD(TAG, "%s - Initing device", m_devname);


  if (-1 == xioctl(m_fd, VIDIOC_QUERYCAP, &cap)) {
    if (EINVAL == errno) {
      OATPP_LOGE(TAG, "%s - %s is no V4L2 device", m_devname,
              m_devname);
      return ENODEV;
    } else {
      return errno_report("VIDIOC_QUERYCAP");
    }
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    OATPP_LOGE(TAG, "%s - %s is no video capture device", m_devname,
            m_devname);
    return ENODEV;
  }

  switch (m_io) {
    case IO_METHOD_READ:
      if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
        OATPP_LOGE(TAG, "%s - %s does not support read i/o", m_devname,
                m_devname);
        return ENOTSUP;
      }
      break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
      if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        OATPP_LOGE(TAG, "%s - %s does not support streaming i/o", m_devname,
                m_devname);
        return ENOTSUP;
      }
      break;
  }


  /* Select video input, video standard and tune here. */

  CLEAR(cropcap);

  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (0 == xioctl(m_fd, VIDIOC_CROPCAP, &cropcap)) {
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */

    if (-1 == xioctl(m_fd, VIDIOC_S_CROP, &crop)) {
      switch (errno) {
        case EINVAL:
          /* Cropping not supported. */
          break;
        default:
          /* Errors ignored. */
          break;
      }
    }
  } else {
    /* Errors ignored. */
  }


  CLEAR(fmt);

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//  if (m_forceformat) {
    fmt.fmt.pix.width       = 640;
    fmt.fmt.pix.height      = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(m_fd, VIDIOC_S_FMT, &fmt))
      return errno_report("VIDIOC_S_FMT");
//
//    /* Note VIDIOC_S_FMT may change width and height. */
//  } else {
    /* Preserve original settings as set by v4l2-ctl for example */
//    if (-1 == xioctl(m_fd, VIDIOC_G_FMT, &fmt))
//      return errno_report("VIDIOC_G_FMT");
//  }

  switch (m_io) {
    case IO_METHOD_READ:
      init_read(fmt.fmt.pix.sizeimage);
      break;

    case IO_METHOD_MMAP:init_mmap();
      break;

    case IO_METHOD_USERPTR:
      init_userp(fmt.fmt.pix.sizeimage);
      break;
  }

  OATPP_LOGD(TAG, "%s - Device inited", m_devname);

  return 0;
}

int V4LGrabber::close_device()
{
  OATPP_LOGD(TAG, "%s - Closing device", m_devname);

  if (-1 == close(m_fd))
    return errno_report("close");

  m_fd = -1;
  OATPP_LOGD(TAG, "%s - Device closed", m_devname);

  m_initdone = false;

  return 0;
}

int V4LGrabber::open_device()
{
  int rc = testDevice(m_devname);
  if(rc) {
    return rc;
  }

  OATPP_LOGD(TAG, "%s - Opening device", m_devname,m_devname);
  
  m_fd = open(m_devname, O_RDWR /* required */ | O_NONBLOCK, 0);

  if (-1 == m_fd) {
    OATPP_LOGE(TAG, "%s - Cannot open '%s': %d, %s", m_devname,
            m_devname, errno, strerror(errno));
    return EXIT_FAILURE;
  }

  OATPP_LOGD(TAG, "%s - Device opened", m_devname,m_devname);

  return 0;
}

int V4LGrabber::testDevice(const char *device) {
  struct stat st;
  int fd;

  OATPP_LOGD(TAG, "Testing device '%s'", device);

  if (-1 == stat(device, &st)) {
    OATPP_LOGE(TAG, "Cannot identify '%s': %s (%d)", device, strerror(errno), errno);
    return EXIT_FAILURE;
  }

  if (!S_ISCHR(st.st_mode)) {
    OATPP_LOGE(TAG, "%s is no device", device);
    return ENODEV;
  }

  OATPP_LOGD(TAG, "Tested device '%s' - ok", device);

  return 0;
}


V4LGrabber::V4LGrabber(const char *device, void(*imagecb)(void*, const void*, int), void* userdata)
  : m_devname(device)
  , m_initdone(false)
  , m_imagecb(imagecb)
  , m_userdata(userdata)
  , m_buffers(nullptr)
  , m_nbuffers(0)
  , m_capturing(false) {

}

V4LGrabber::~V4LGrabber() {
  stop_capturing();
}
