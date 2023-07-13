#ifndef DataFormats_HGCalDigis_HGCalFlaggedECONDInfo_h
#define DataFormats_HGCalDigis_HGCalFlaggedECONDInfo_h

#include <vector>

class HGCalFlaggedECONDInfo
{
 public:

  HGCalFlaggedECONDInfo() : HGCalFlaggedECONDInfo(0,0,0) {}
  HGCalFlaggedECONDInfo(uint32_t loc, uint32_t flagbits, uint32_t id)
    : iword(loc), flags(flagbits), eleid(id) {}
  HGCalFlaggedECONDInfo(const HGCalFlaggedECONDInfo &t)
    : HGCalFlaggedECONDInfo(t.iword,t.flags,t.eleid) {}
  
  static uint32_t buildECONDFlag(bool cbStatus, bool htbits, bool ebobits, bool matchbit,bool truncated){
    return (cbStatus | (htbits<<1) | (ebobits<<2) | (matchbit<<3) | (truncated<<4));
  }
  
  bool cbFlag() { return flags & 0x1; };
  bool htFlag() { return (flags>>1) & 0x1; };
  bool eboFlag() { return (flags>>2) & 0x1; };
  bool matchFlag() { return (flags>>3) & 0x1; };
  bool truncatedFlag() { return (flags>>4) & 0x1; };
    
  uint32_t iword,flags,eleid;
};

typedef std::vector<HGCalFlaggedECONDInfo> HGCalFlaggedECONDInfoCollection;

#endif
