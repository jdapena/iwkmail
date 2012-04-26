#include "im-error.h"

GQuark
im_get_error_quark (void)
{
  static GQuark quark = 0;
  if (quark == 0)
    quark = g_quark_from_static_string ("im-error-quark");
  return quark;
}

