#ifndef _QRENC_H
#define _QRENC_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "config.h"
#include "qrencode.h"
#if !defined(WIN32)
#include <getopt.h>
#include <png.h>
#else
#include "third_party/libpng/png.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int writePNG(QRcode *qrcode, const char *outfile);
QRcode *encode(char *intext, int length);
void qrencode(char *intext, int length, const char *outfile);

#if defined(__cplusplus)
}
#endif

#endif
