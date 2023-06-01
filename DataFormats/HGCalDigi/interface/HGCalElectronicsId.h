#ifndef DataFormats_HGCalDigis_HGCalElectronicsId_h
#define DataFormats_HGCalDigis_HGCalElectronicsId_h

#include <iostream>
#include <ostream>
#include <cstdint>

/**
   @class HGCalElectronicsId
   @short wrapper for a 32b data word identifying a readout channel in the raw data
   The format is the following:
   Reserved: b'[29,31]
   Common mode: b'[28]
   FED ID: b'[18,27]
   Capture Block ID: b'[14,17]
   ECON-D idx: b'[10,13]
   ECON-D eRx: b'[6,9]
   1/2 ROC channel number: b'[0-5]   
 */

class HGCalElectronicsId {
public:
  enum HGCalElectronicsIdMask {
    kCommonMode = 0x1,
    kFEDIDMask = 0x3ff,
    kCaptureBlockMask = 0xf,
    kECONDIdxMask = 0xf,
    kECONDeRxMask = 0xf,
    kHalfROCChannelMask = 0x3f
  };
  enum HGCalElectronicsIdShift {
    kCommonModeShift = 28,
    kFEDIDShift = 18,
    kCaptureBlockShift = 14,
    kECONDIdxShift = 10,
    kECONDeRxShift = 6,
    kHalfROCChannelShift = 0
  };

  /**
     @short CTOR
  */
  HGCalElectronicsId() : value_(0) {}
  HGCalElectronicsId(bool cmflag,uint16_t fedid, uint8_t captureblock, uint8_t econdidx, uint8_t econderx, uint8_t halfrocch);
  HGCalElectronicsId(uint32_t value) : value_(value) {}
  HGCalElectronicsId(const HGCalElectronicsId& o) : value_(o.value_) {}

  /**
     @short getters
  */
  uint32_t operator()() { return value_; }
  uint32_t raw() { return value_; }
  uint16_t fedId();
  uint8_t captureBlock();
  uint8_t econdIdx();
  uint8_t econdeRx();
  uint8_t halfrocChannel();
  bool isCM();
  
  void print(std::ostream& out = std::cout) {
    out << "Raw=0x" << std::hex << raw() << std::dec << std::endl
        << "\tFED-ID: " << (uint32_t)fedId() << " Capture Block: " << (uint32_t)captureBlock()
        << " ECON-D idx: " << (uint32_t)econdIdx() << " eRx: " << (uint32_t)econdeRx()
        << " 1/2 ROC ch.: " << (uint32_t)halfrocChannel() << " isCM=" << isCM() << std::endl;
  }

private:
  // a 32-bit word
  uint32_t value_;
};

#endif
