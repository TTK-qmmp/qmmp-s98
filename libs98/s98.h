#ifndef __M_S98___
#define __M_S98___

#include "tchar.h"

class S98DEVICEIF;

typedef struct {
  Uint8 dwIsV3;
  Uint16 dwSamplesPerSec;
  Uint16 dwChannels;
  Uint16 dwBitsPerSample;
  Uint16 dwSeekable;
  Uint32 dwUnitRender;
  Uint32 dwReserved1;
  Uint32 dwReserved2;
  DWORD dwLength;               // song length in ms
} SOUNDINFO;

#define S98DEVICE_MAX 16
#define SAMPLE_RATE 44100
#define SYNC_RATE 60 /* (Hz) */
#define UNIT_RENDER (SAMPLE_RATE / SYNC_RATE)

class s98File {
public:
  s98File();
  virtual ~s98File();

  bool OpenFromBuffer(const BYTE* buffer, DWORD dwSize, SOUNDINFO* pInfo);
  void Close(void);

  DWORD GetPosition();
  DWORD SetPosition(DWORD dwpos);
  DWORD Write(Int16* buffer, DWORD numSample);
  void  getRawFileInfo(unsigned char* titleOut, int max, int stripGarbage);

protected:
  int          number_of_devices;
  S98DEVICEIF* devices[S98DEVICE_MAX];
  BYTE         devicemap[0x40];

  BYTE* s98data;
  BYTE* s98head;
  BYTE* s98top;
  BYTE* s98loop;
  int   length;
  DWORD playtime;       /* syncs */
  DWORD looptime;       /* syncs */

  BYTE* s98cur;
  DWORD curtime;

  int loopnum;

#define SPS_SHIFT 28
#define SPS_LIMIT (1 << SPS_SHIFT)
  enum { SAMPLE_PER_SYNC, SYNC_PER_SAMPLE } spsmode;
  DWORD sps;                    /* sync/sample or sample/syjnc */
  DWORD timerinfo1;
  DWORD timerinfo2;

  double sync_per_sec;

  DWORD lefthi;
  DWORD leftlo;

  Sample bufdev[UNIT_RENDER * 2];

  void CalcTime(void);
  void Step(void);
  void Reset(void);

  void  WriteSub(Int16* buffer, DWORD numSample);
  DWORD SyncToMsec(DWORD sync);
  DWORD MsecToSync(DWORD ms);
};


#endif
