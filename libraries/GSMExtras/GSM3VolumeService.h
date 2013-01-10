#ifndef _GSM3VOLUMESERVICE_
#define _GSM3VOLUMESERVICE_

#include <GSM3ShieldV1ModemCore.h>

class GSM3VolumeService : public GSM3ShieldV1BaseProvider {
public:
  void setVolume(int);
  void checkVolume();
  int getVolume() { return volume; }
  void manageResponse(byte from, byte to);
private:
  bool parseCLVL();
  int checkVolumeContinue();
  int setVolumeContinue();
  int volume;
};

#endif