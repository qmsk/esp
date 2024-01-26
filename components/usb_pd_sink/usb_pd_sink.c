#include "usb_pd_sink.h"

#include <stdlib.h>

#include <logging.h>

int usb_pd_sink_init(struct usb_pd_sink *sink, const struct usb_pd_sink_options *options)
{
  switch ((sink->type = options->type)) {
    case USB_PD_SINK_STUSB4500:
      return stusb4500_init(&sink->state.stusb4500, options);

    default:
      LOG_FATAL("invalid type=%d", options->type);
  }
}

int usb_pd_sink_new(struct usb_pd_sink **sinkp, const struct usb_pd_sink_options *options)
{
  struct usb_pd_sink *sink;
  int err;

  if (!(sink = calloc(1, sizeof(*sink)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = usb_pd_sink_init(sink, options))) {
    goto error;
  }

  *sinkp = sink;

  return 0;

error:
  free(sink);

  return err;
}

int usb_pd_sink_setup(struct usb_pd_sink *sink)
{
  switch (sink->type) {
    case USB_PD_SINK_STUSB4500:
      return stusb4500_setup(&sink->state.stusb4500);

    default:
      LOG_FATAL("invalid type=%d", sink->type);
  }

}

int usb_pd_sink_start(struct usb_pd_sink *sink)
{
  switch (sink->type) {
    case USB_PD_SINK_STUSB4500:
      return stusb4500_start(&sink->state.stusb4500);

    default:
      LOG_FATAL("invalid type=%d", sink->type);
  }
}

int usb_pd_sink_status(struct usb_pd_sink *sink, struct usb_pd_sink_status *status)
{
  switch (sink->type) {
    case USB_PD_SINK_STUSB4500:
      return stusb4500_status(&sink->state.stusb4500, status);

    default:
      LOG_FATAL("invalid type=%d", sink->type);
  }
}

int usb_pd_sink_print(struct usb_pd_sink *sink, FILE *file)
{
  switch (sink->type) {
    case USB_PD_SINK_STUSB4500:
      return stusb4500_print(&sink->state.stusb4500, file);

    default:
      LOG_FATAL("invalid type=%d", sink->type);
  }
}
