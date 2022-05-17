#include "s98.h"
#include "zlib.h"
#include "device/s98device.h"
#include <stdio.h>

/* S98 file header struct */
typedef struct {
  char chk[4];                  /* 'S98'(check code) + '1'(format version) */
  int synctime;                 /* sync time(ms) */
  int timer_reserve_2;          /* must be zero */
  int compsize;                 /* 0:uncompressed 1-:uncompressed size */
  int ofstitle;                 /* file offset to title(7bitASCII or SJIS) */
  int ofsdata;                  /* file offset to logged data */
  int ofsloop;                  /* 0:noloop 1-:file offset to loop point */
} S98HEAD;

// returns the raw info text.. parsing must be done later (see tagged V3 info)
void s98File::getRawFileInfo(unsigned char* titleOut, int max, int stripGarbage)
{
  if (titleOut != NULL)
  {
    if (s98head == NULL)
    {
      titleOut[0] = 0;
    }
    else
    {
      S98HEAD* head = (S98HEAD *)s98head;
      if (head->ofstitle)
      {
        snprintf((char *)titleOut, max, "%s", (char *)s98data + head->ofstitle);

        // some files contain "garbage" or some unknown encoding..
        if (stripGarbage)
        {
          for (int i = 0; i<max; i++)
          {
            if (titleOut[i] > 128)
            {
              titleOut[i] = 0;
              break;
            }
          }
        }
      }
      else
      {
        // caller will have to think of some a fallback
      }
    }
  }
}

/*
 #define KMPMODULE_PLUGIN_VERSION    10
 #define KMPMODULE_PLUGIN_NAME       "m_s98.kpi v1.0r8 by Mamiya"
 #define KMPMODULE_PLUGIN_COPYRIGHT  "FM Sound Generator rel.008 (C) cisc 1998-2003. / emu2413 by Mitsutaka Okazaki 2001-2004 / zlib " ## ZLIB_VERSION ## " (C) 1995-2005 Jean-loup Gailly and Mark Adler"
 #define KMPMODULE_REENTRANT 0
 */
#define MASTER_CLOCK (7987200)
#define LOOPNUM 2


/* S98 file header */
#define S98_MAGIC_V0    (0x53393830)    /* 'S980' */
#define S98_MAGIC_V1    (0x53393831)    /* 'S981' */
#define S98_MAGIC_V2    (0x53393832)    /* 'S982' */
#define S98_MAGIC_V3    (0x53393833)    /* 'S983' */
#define S98_MAGIC_VZ    (0x5339385A)    /* 'S98Z' */
#define S98_OFS_MAGIC           (0x00)
#define S98_OFS_TIMER_INFO1     (0x04)
#define S98_OFS_TIMER_INFO2     (0x08)
#define S98_OFS_COMPRESSING     (0x0C)
#define S98_OFS_OFSTITLE        (0x10)
#define S98_OFS_OFSDATA         (0x14)
#define S98_OFS_OFSLOOP         (0x18)
#define S98_OFS_OFSCOMP         (0x1C)
#define S98_OFS_DEVICECOUNT     (0x1C)
#define S98_OFS_DEVICEINFO      (0x20)

#define VGM_MAGIC               (0x56676D20)    /* 'Vgm ' */

S98DEVICEIF * S98DeviceCreate(int type, int clock, int rate)
{
  S98DEVICEIF* ret = 0;
  switch (type)
  {
  case S98DEVICETYPE_PSG_YM:
    ret = CreateS98DevicePSG(true);
    break;
  case S98DEVICETYPE_PSG_AY:
    ret = CreateS98DevicePSG(false);
    break;
  case S98DEVICETYPE_OPN:
    ret = CreateS98DeviceOPN();
    break;
  case S98DEVICETYPE_OPN2:
    ret = CreateS98DeviceOPN2();
    break;
  case S98DEVICETYPE_OPNA:
    ret = CreateS98DeviceOPNA();
    break;
  case S98DEVICETYPE_OPM:
    ret = CreateS98DeviceOPM();
    break;
  case S98DEVICETYPE_OPLL:
    ret = CreateS98DeviceOPLL();
    break;
  case S98DEVICETYPE_SNG:
    ret = CreateS98DeviceSNG();
    break;
  case S98DEVICETYPE_OPL:
    ret = CreateS98DeviceOPL();
    break;
  case S98DEVICETYPE_OPL2:
    ret = CreateS98DeviceOPL2();
    break;
  case S98DEVICETYPE_OPL3:
    ret = CreateS98DeviceOPL3();
    break;
  }
  if (ret)
  {
    ret->Init(clock, rate);
  }
  return ret;
}


static void SetDwordLE(Uint8* p, Uint32 v)
{
  p[0] = (v >> (8 * 0)) & 0xFF;
  p[1] = (v >> (8 * 1)) & 0xFF;
  p[2] = (v >> (8 * 2)) & 0xFF;
  p[3] = (v >> (8 * 3)) & 0xFF;
}
static void SetDwordBE(Uint8* p, Uint32 v)
{
  p[0] = (v >> (8 * 3)) & 0xFF;
  p[1] = (v >> (8 * 2)) & 0xFF;
  p[2] = (v >> (8 * 1)) & 0xFF;
  p[3] = (v >> (8 * 0)) & 0xFF;
}
static Uint32 GetWordLE(Uint8* p)
{
  int ret;
  ret  = ((Uint32)(Uint8)p[0]) << 0x00;
  ret |= ((Uint32)(Uint8)p[1]) << 0x08;
  return ret;
}

static Uint32 GetDwordLE(Uint8* p)
{
  int ret;
  ret  = ((Uint32)(Uint8)p[0]) << 0x00;
  ret |= ((Uint32)(Uint8)p[1]) << 0x08;
  ret |= ((Uint32)(Uint8)p[2]) << 0x10;
  ret |= ((Uint32)(Uint8)p[3]) << 0x18;
  return ret;
}

static Uint32 GetDwordBE(Uint8* p)
{
  Uint32 ret;
  ret  = ((Uint32)(Uint8)p[0]) << 0x18;
  ret |= ((Uint32)(Uint8)p[1]) << 0x10;
  ret |= ((Uint32)(Uint8)p[2]) << 0x08;
  ret |= ((Uint32)(Uint8)p[3]) << 0x00;
  return ret;
}


s98File::s98File()
{
  s98data           = 0;
  number_of_devices = 0;
  memset(devices, 0, sizeof(devices));
}
s98File::~s98File()
{
  Close();
}

void s98File::CalcTime(void)
{
  BYTE* p = s98top;
  looptime = 0;
  playtime = 0;
  if (!s98data)
    return;
  while (1)
  {
    if (p == s98loop)
      looptime = playtime;
    if (*p < 0x80)
    {
      p += 3;
    }
    else if (*p == 0xff)
    {
      playtime += 1;
      p        += 1;
    }
    else if (*p == 0xfe)
    {
      int s = 0, n = 0;
      do
      {
        n |= (*(++p) & 0x7f) << s;
        s += 7;
      }
      while (*p & 0x80);
      playtime += n + 2;
      p        += 1;
    }
    else
    {
      return;
    }
  }
}

static Uint32 DivFix(Uint32 p1, Uint32 p2, Uint32 fix)
{
  Uint32 ret;
  ret = p1 / p2;
  p1  = p1 % p2;      /* p1 = p1 - p2 * ret; */
  while (fix--)
  {
    p1  += p1;
    ret += ret;
    if (p1 >= p2)
    {
      p1 -= p2;
      ret++;
    }
  }
  return ret;
}

void s98File::Reset(void)
{
  for (int d = 0; d < number_of_devices; d++)
    if (devices[d])
      devices[d]->Reset();

  s98cur  = s98top;
  curtime = 0;

  lefthi = 0;
  leftlo = 0;
  Step();
}

void s98File::Step(void)
{
  while (1)
  {
    if (*s98cur < 0x80)
    {
      int d = devicemap[*s98cur >> 1];
      if (d != S98DEVICE_MAX && devices[d])
      {
        if (*s98cur & 1)
          devices[d]->SetReg(0x100 | s98cur[1], s98cur[2]);
        else
          devices[d]->SetReg(s98cur[1], s98cur[2]);
      }
      s98cur += 3;
      continue;
    }
    if (*s98cur == 0xfe || *s98cur == 0xff)
      break;
    if (*s98cur == 0xfd && s98loop)
    {
      s98cur = s98loop;
      continue;
    }
    lefthi = 0;
    for (int d = 0; d < number_of_devices; d++)
      if (devices[d])
        devices[d]->Disable();
    return;
  }
  while (1)
  {
    if (*s98cur == 0xff)
    {
      lefthi += 1;
      s98cur++;
    }
    else if (*s98cur == 0xfe)
    {
      int s = 0, n = 0;
      do
      {
        n |= (*(++s98cur) & 0x7f) << s;
        s += 7;
      } while (*s98cur & 0x80);
      lefthi += n + 2;
      s98cur++;
    }
    else
    {
      break;
    }
  }
  return;
}

DWORD s98File::SyncToMsec(DWORD sync)
{
  return (DWORD)(((double)sync) * ((double)1000) / sync_per_sec);
}

DWORD s98File::MsecToSync(DWORD ms)
{
  return (DWORD)(((double)ms) * sync_per_sec / ((double)1000));
}

DWORD s98File::GetPosition()
{
  return SyncToMsec(curtime);
}

DWORD s98File::SetPosition(DWORD dwpos_)
{
  if (!s98data)
    return 0;
  dwpos_ = MsecToSync(dwpos_);
/*
        char buf[1024];
        wsprintf(buf, "s98debug:%d:%d\n", dwpos_, curtime);
        OutputDebugString(buf);
 */
  if (dwpos_ < curtime)
  {
    Reset();
  }
  while (dwpos_ > curtime)
  {
    curtime++;
    if (lefthi && --lefthi == 0)
      Step();
  }
  return SyncToMsec(curtime);
}

/* X1F file header */
#define X1F_MAGIC               (0x58314600)    /* 'X1F\0' */

static BYTE * x1f2s98(BYTE* ps, DWORD slen, DWORD* pdlen)
{
  DWORD spos = 0, dpos = 0x80, psloop = 0;
  BYTE  psgreg = 0;
  BYTE* pd;
  if ((GetDwordBE(ps) & 0xFFFFFF00) != X1F_MAGIC)
    return 0;
  /* 150%(worst case) */
  pd = (BYTE *)malloc(dpos + 1024 + ((slen + slen + slen) >> 1) + 1);
  if (!pd)
    return 0;
  XMEMSET(pd, 0, dpos);
  SetDwordBE(pd + S98_OFS_MAGIC, S98_MAGIC_V2);
  SetDwordLE(pd + S98_OFS_TIMER_INFO1, 1);
  SetDwordLE(pd + S98_OFS_TIMER_INFO2, 60);
  SetDwordLE(pd + S98_OFS_OFSDATA, dpos);
  SetDwordLE(pd + S98_OFS_DEVICEINFO + 0x00, S98DEVICETYPE_OPM);
  SetDwordLE(pd + S98_OFS_DEVICEINFO + 0x04, 4000000 /*3579545*/);

  // XXX INCONSISTENT SOURCE CODE.. is it S98DEVICETYPE_PSG_YM or S98DEVICETYPE_PSG_AY.. or something else!?
// SetDwordLE(pd + S98_OFS_DEVICEINFO + 0x10, S98DEVICETYPE_PSG);
  SetDwordLE(pd + S98_OFS_DEVICEINFO + 0x10, S98DEVICETYPE_PSG_YM);             // some if/else needed? based on what?
  SetDwordLE(pd + S98_OFS_DEVICEINFO + 0x14, 4000000 /*3579545*/);
  for (spos = 4; spos < 256; spos++)
  {
    pd[dpos++] = 0;
    pd[dpos++] = (BYTE)spos;
    pd[dpos++] = ps[spos];
  }
  while (spos < slen)
  {
    if (psloop == spos)
    {
      if (psloop)
        SetDwordLE(pd + S98_OFS_OFSLOOP, dpos);
    }
    switch (ps[spos + 1])
    {
    case 0:
    {
      Uint32 wait = ps[spos + 0];
      if (wait)
      {
        spos += 2;
      }
      else
      {
        wait  = (((Uint32)ps[spos + 3]) << 8) + ps[spos + 2];
        spos += 4;
      }
      if (wait == 1)
      {
        pd[dpos++] = 0xff;
      }
      else if (wait == 2)
      {
        pd[dpos++] = 0xff;
        pd[dpos++] = 0xff;
      }
      else if (wait > 2)
      {
        wait      -= 2;
        pd[dpos++] = 0xfe;
        while (wait > 0x7f)
        {
          pd[dpos++] = (wait & 0x7f) + 0x80;
          wait     >>= 7;
        }
        pd[dpos++] = wait & 0x7f;
      }
    }
    break;
    case 2:                             // PSG register
      psgreg = ps[spos + 0];
      spos  += 2;
      break;
    case 3:                             // PSG data
      pd[dpos++] = 2;
      pd[dpos++] = psgreg;
      pd[dpos++] = ps[spos + 0];
      spos      += 2;
      break;
    default:
      pd[dpos++] = 0;
      pd[dpos++] = ps[spos + 1];
      pd[dpos++] = ps[spos + 0];
      spos      += 2;
      break;
    }
  }
  pd[dpos++] = 0xfd;
  *pdlen     = dpos;
  return pd;
}

/* MYM file header */
#define MYM_MAGIC_V0    (0x4D594D30)    /* 'MYM0' */
#define MYM_MAGIC_V1    (0x4D594D31)    /* 'MYM1' */
#define MYM_MAGIC_VC    (0x4D594D43)    /* 'MYMC' */

static BYTE * mym2s98(BYTE* ps, DWORD slen, DWORD* pdlen)
{
  DWORD spos = 0, dpos = 0x40, psloop;
  BYTE* pd;
#ifdef X1F_MAGIC
  if ((GetDwordBE(ps) & 0xFFFFFF00) == X1F_MAGIC)
  {
    return x1f2s98(ps, slen, pdlen);
  }
#endif
  /* 150%(worst case) */
  pd = (BYTE *)malloc(dpos + ((slen + slen + slen) >> 1) + 1);
  if (!pd)
    return 0;
  XMEMSET(pd, 0, dpos);
  SetDwordBE(pd + S98_OFS_MAGIC, S98_MAGIC_V2);
  SetDwordLE(pd + S98_OFS_OFSDATA, dpos);
  SetDwordLE(pd + S98_OFS_DEVICEINFO, S98DEVICETYPE_OPM);
  SetDwordLE(pd + S98_OFS_DEVICEINFO + 4, 3579545);
  if (GetDwordBE(ps) == MYM_MAGIC_V0 || GetDwordBE(ps) == MYM_MAGIC_V1)
  {
    SetDwordLE(pd + S98_OFS_TIMER_INFO1, ps[5] ? ps[5] : 1);
    SetDwordLE(pd + S98_OFS_TIMER_INFO2, 120 * (Uint32)(ps[7] ? ps[7] : 1));
    psloop = (((Uint32)ps[9]) << 24) + (((Uint32)ps[11]) << 16) + (((Uint32)ps[13]) << 8) + ((Uint32)ps[15]);
    spos  += 16;
  }
  else
  {
    SetDwordLE(pd + S98_OFS_TIMER_INFO1, 1);
    SetDwordLE(pd + S98_OFS_TIMER_INFO2, 60);
    psloop = 0;
  }
  if (GetDwordBE(&ps[spos]) == MYM_MAGIC_VC)
  {
    SetDwordLE(pd + S98_OFS_DEVICEINFO + 4, (((Uint32)ps[spos + 5]) << 24) + (((Uint32)ps[spos + 7]) << 16) + (((Uint32)ps[spos + 9]) << 8) + ((Uint32)ps[spos + 11]));
    spos += 12;
  }
  while (spos < slen)
  {
    if (psloop == spos)
    {
      if (psloop)
        SetDwordLE(pd + S98_OFS_OFSLOOP, dpos);
    }
    switch(ps[spos]) {
    case 0:
      pd[dpos++] = 0xff;
      spos++;
      break;
    default:
      pd[dpos++] = 0;
      pd[dpos++] = ps[spos++];
      pd[dpos++] = ps[spos++];
      break;
    }
  }
  pd[dpos++] = 0xfd;
  *pdlen     = dpos;
  return pd;
}

/* TAR file header */
#define TAR_MAGIC       (0x75737461)    /* 'usta' */
#define TAR_MAGIC4      (0x72)          /* 'r' */

static DWORD GetTarOcts(BYTE* p, DWORD l)
{
  DWORD i, r;
  for (i = 0; i < l && p[i] == 0x20; i++)
    ;                                                   /* skip space */
  for (r = 0; i < l && '0' <= p[i] && p[i] <= '7'; i++)
  {
    r *= 8;
    r += p[i] - '0';
  }
  return r;
}

static DWORD IsTarHeader(BYTE* p)
{
  DWORD i, sum1, sum2;
  /* magic check */
  if (GetDwordBE(p + 0x101) != TAR_MAGIC || p[0x105] != TAR_MAGIC4)
    return 0;
  /* sum cehck */
  sum1 = GetTarOcts(p + 0x94, 8);
  sum2 = 0;
  for (i = 0; i < 512; i++)
    sum2 += (0x94 <= i && i < 0x9C) ? 0x20 : p[i];
  if ((sum1 & 0xFFFF) != (sum2 & 0xFFFF))
    return 0;
  if (p[i] != 0x9C)
    return 512;
  /* check next header */
  sum1 = 512 + ((GetTarOcts(p + 0x7c, 12) + 511) & (~511));
  sum2 = IsTarHeader(p + sum1);
  return sum1 ? (sum1 + sum2) : 0;
}

static int default_sample_rate = SAMPLE_RATE;
static int default_loopnum     = LOOPNUM;

// info: text info in .98 at pos 0x80-0xaf
bool s98File::OpenFromBuffer(const BYTE* buffer, DWORD dwSize, SOUNDINFO* pInfo)
{
  pInfo->dwIsV3 = 0;

  int sample_rate = (pInfo->dwSamplesPerSec == 0) ? default_sample_rate : pInfo->dwSamplesPerSec;
  Close();

  DWORD dataofs, loopofs;
  BYTE* buf    = 0;
  DWORD length = dwSize;
  DWORD magic  = 0;

  do
  {
    if (length < 0x40)
      break;
    buf = (BYTE *)malloc(length);
    if (!buf)
      break;
    XMEMCPY(buf, buffer, length);
    /* Uncompress GZIP */
    if (buf[0] == 0x1f && buf[1] == 0x8b)
    {
      BYTE*    des = 0;
      BYTE*    gzp;
      unsigned deslen = 4096;
      int      z_err;
      z_stream zs;

      des = (BYTE *)malloc(deslen);
      if (!des)
        break;

      XMEMSET(&zs, 0, sizeof(z_stream));

      gzp = buf + 10;
      if (buf[3] & 4)
      {
        DWORD extra = *gzp++;
        extra += *gzp++ << 8;
        gzp   += extra;
      }
      if (buf[3] & 8)
        while (*gzp++)
          ;
      if (buf[3] & 16)
        while (*gzp++)
          ;
      if (buf[3] & 2)
        gzp += 2;

      zs.next_in   = gzp;
      zs.avail_in  = length - (gzp - buf);
      zs.next_out  = des;
      zs.avail_out = deslen;
      zs.zalloc    = (alloc_func)0;
      zs.zfree     = (free_func)0;
      zs.opaque    = (voidpf)0;

      z_err = inflateInit2(&zs, -MAX_WBITS);
      if (z_err != Z_OK)
      {
        inflateEnd(&zs);
        break;
      }
      inflateReset(&zs);
      while (1)
      {
        z_err = inflate(&zs, Z_SYNC_FLUSH);
        if (z_err == Z_STREAM_END)
          break;
        if (z_err != Z_OK || zs.avail_in == 0)
        {
          free(des);
          des = 0;
          break;
        }
        if (zs.avail_out == 0)
        {
          BYTE* p;
          p = (BYTE *)realloc(des, deslen + 4096);
          if (!p)
          {
            free(des);
            des = 0;
            break;
          }
          des           = p;
          zs.next_out   = des + deslen;
          zs.avail_out += 4096;
          deslen       += 4096;
        }
      }
      ;
      if (des)
      {
        free(buf);
        buf    = des;
        length = zs.total_out;
      }
      inflateEnd(&zs);
      if (!des)
        break;
    }

    /* skip TAR header */
    s98head = buf;
    if (length >= 512)
    {
      DWORD lentarheader = IsTarHeader(s98head);
      s98head += lentarheader;
      length  -= lentarheader;
    }
    if (length < 0x40)
      break;
    magic = GetDwordBE(s98head + S98_OFS_MAGIC);

    if (S98_MAGIC_V0 <= magic && magic <= S98_MAGIC_VZ)
    {
      /* version check */
      if (S98_MAGIC_V3 < magic)
        break;
    }
    else
    {
      DWORD cnvs98length;
      BYTE* cnvs98;
      cnvs98 = mym2s98(s98head, length, &cnvs98length);
      if (!cnvs98)
        break;
      free(buf);
      s98head = buf = cnvs98;
      length  = cnvs98length;
    }
    loopofs = GetDwordLE(s98head + S98_OFS_OFSLOOP);
    dataofs = GetDwordLE(s98head + S98_OFS_OFSDATA);
    /* Uncompress internal deflate(old gimmick) */
    if (GetDwordLE(s98head + S98_OFS_COMPRESSING))
    {
      uLongf dessize;
      BYTE*  des;
      DWORD  compofs;
      if (S98_MAGIC_V2 == magic)
      {
        compofs = GetDwordLE(s98head + S98_OFS_OFSCOMP);
      }
      if (!compofs)
        compofs = dataofs;
      des = (BYTE *)malloc(compofs + GetDwordLE(s98head + S98_OFS_COMPRESSING));
      if (!des)
        break;
      XMEMCPY(des, s98head, compofs);
      if (Z_OK != uncompress(&des[compofs], &dessize, s98head + compofs, length - compofs))
      {
        free(des);
        break;
      }
      length  = dessize;
      s98head = des;
      s98data = des;
      s98top  = s98head + dataofs;
      s98loop = loopofs ? (s98head + loopofs) : 0;
      des     = 0;
    }
    else
    {
      s98data = buf;
      s98top  = s98head + dataofs;
      s98loop = loopofs ? (s98head + loopofs) : 0;
      buf     = 0;
    }
    /* if (length <= loopofs) s98loop = 0; */
  } while(0);
  if (buf)
    free(buf);

  if (!s98data)
    return false;

  timerinfo1 = GetDwordLE(s98head + S98_OFS_TIMER_INFO1);
  timerinfo2 = GetDwordLE(s98head + S98_OFS_TIMER_INFO2);
  if (timerinfo1 == 0)
    timerinfo1 = 10;
  if (timerinfo2 == 0)
    timerinfo2 = 1000;

  int d;
  for (d = 0; d < 0x40; d++)
    devicemap[d] = S98DEVICE_MAX;
  number_of_devices = 0;

  pInfo->dwIsV3 = (S98_MAGIC_V3 == magic);

  if ((S98_MAGIC_V3 != magic && !GetDwordLE(s98head + S98_OFS_DEVICEINFO)) ||
      (S98_MAGIC_V3 == magic && !GetDwordLE(s98head + S98_OFS_DEVICECOUNT)))
  {
    devices[0] = S98DeviceCreate(S98DEVICETYPE_OPNA, MASTER_CLOCK, sample_rate);
    if (devices[0])
    {
      devicemap[0]      = 0;
      number_of_devices = 1;
    }
  }
  else
  {
    int   devicemax = S98DEVICE_MAX;
    BYTE* devinfo   = s98head + S98_OFS_DEVICEINFO;
    if (S98_MAGIC_V3 == magic && GetDwordLE(s98head + S98_OFS_DEVICECOUNT))
    {
      devicemax = GetDwordLE(s98head + S98_OFS_DEVICECOUNT);
    }
    for (d = 0; d < devicemax; d++)
// for (d = 0; d < devicemax && GetDwordLE(devinfo); d++)
    {
      devices[number_of_devices] = S98DeviceCreate(GetDwordLE(devinfo), GetDwordLE(devinfo + 4), sample_rate);
      if (devices[number_of_devices])
      {
        devicemap[d] = number_of_devices++;
        devices[d]->SetPan(GetDwordLE(devinfo + 8));
      }
      else
      {
        number_of_devices++;
      }
      devinfo += 16;
    }
  }

  double dsps;
  double sample_per_sec = (double)sample_rate;
  sync_per_sec = ((double)timerinfo2) / ((double)timerinfo1);
  if (sync_per_sec > sample_per_sec)
  {
    dsps    = sample_per_sec / sync_per_sec;
    spsmode = SAMPLE_PER_SYNC;
  }
  else
  {
    dsps    = sync_per_sec / sample_per_sec;
    spsmode = SYNC_PER_SAMPLE;
  }
  sps = (DWORD)(dsps * (double)SPS_LIMIT);
  if (sps >= SPS_LIMIT)
  {
    sps     = SPS_LIMIT;
    spsmode = SYNC_PER_SAMPLE;
  }

  if (spsmode == SAMPLE_PER_SYNC)
    return false;

  CalcTime();

  pInfo->dwSamplesPerSec = sample_rate;
  pInfo->dwChannels      = 2;
  pInfo->dwBitsPerSample = 16;
  pInfo->dwSeekable      = 1;
  pInfo->dwUnitRender    = UNIT_RENDER * 4;
  pInfo->dwReserved1     = 1;
  pInfo->dwReserved2     = 0;

  if (s98loop)
  {
    loopnum         = default_loopnum;
    pInfo->dwLength = playtime + (playtime - looptime) * (loopnum - 1);
  }
  else
  {
    pInfo->dwLength = playtime;
  }
  loopnum = 0;
  /* syncs to msec */
  pInfo->dwLength = SyncToMsec(pInfo->dwLength);

  Reset();
  return true;
}

void s98File::Close(void)
{
  if (s98data)
  {
    free(s98data); s98data = 0;
  }
  for (int d = 0; d < number_of_devices; d++)
    if (devices[d])
      delete devices[d];
  number_of_devices = 0;
}

void s98File::WriteSub(Int16* buffer, DWORD numSample)  // copies sample data(e.g. 32bit to target buffer - e.g. 16bit)
{
  DWORD i, len;
  while (numSample)
  {
    len = (numSample > UNIT_RENDER) ? UNIT_RENDER : numSample;
    XMEMSET(bufdev, 0, len * 2 * sizeof(Sample));
    for (int d = 0; d < number_of_devices; d++)
      if (devices[d])
        devices[d]->Mix(bufdev, len);
    for (i = 0; i < len * 2; i++)
    {
      Sample s = bufdev[i];
      if (((Uint32)(s + 0x8000)) > 0xffff)                      // WTF? so the emu is always using 16bit resolution even when using 32bit Sample?
      {
        s = (s > 0x7fff) ? 0x7fff : -0x8000;
      }
      *buffer++ = (Int16)s;
    }
    numSample -= len;
  }
}

DWORD s98File::Write(Int16* buffer, DWORD numSample)
{
  DWORD pos, numWrite = 0;
  if (!s98data)
    return numWrite;
  if (numSample == 0)
    return numWrite;

  for (pos = 0; pos < numSample; pos++)
  {
    if (lefthi)
    {
      leftlo += sps;
      if (leftlo >= SPS_LIMIT)
      {
        leftlo  -= SPS_LIMIT;
        lefthi  -= 1;
        curtime += 1;
      }
      if (lefthi == 0)
      {
        if (buffer)
          WriteSub(&buffer[numWrite * 2], pos + 1 - numWrite);
        numWrite = pos + 1;
        Step();
        if (lefthi == 0)
          break;
      }
    }
  }
  if (/*lefthi && */ numWrite != numSample)
  {
    if (buffer)
      WriteSub(&buffer[numWrite * 2], numSample - numWrite);
    numWrite = numSample;
  }
  return numWrite;
}
