#include "DataFormats/HGCalDigi/interface/HGCalElectronicsId.h"

//
HGCalElectronicsId::HGCalElectronicsId(
  bool cmflag,uint16_t fedid, uint8_t captureblock, uint8_t econdidx, uint8_t econderx, uint8_t halfrocch) {
  value_ =
    ((cmflag & kCommonMode) << kCommonModeShift) |
    ((fedid & kFEDIDMask) << kFEDIDShift) | ((captureblock & kCaptureBlockMask) << kCaptureBlockShift) |
           ((econdidx & kECONDIdxMask) << kECONDIdxShift) | ((econderx & kECONDeRxMask) << kECONDeRxShift) |
           ((halfrocch & kHalfROCChannelMask) << kHalfROCChannelShift);
}

//
bool HGCalElectronicsId::isCM() { return (value_ >> kCommonModeShift) & kCommonMode; }

//
uint16_t HGCalElectronicsId::fedId() { return (value_ >> kFEDIDShift) & kFEDIDMask; }

//
uint8_t HGCalElectronicsId::captureBlock() { return (value_ >> kCaptureBlockShift) & kCaptureBlockMask; }

//
uint8_t HGCalElectronicsId::econdIdx() { return (value_ >> kECONDIdxShift) & kECONDIdxMask; }

//
uint8_t HGCalElectronicsId::econdeRx() { return (value_ >> kECONDeRxShift) & kECONDeRxMask; }

//
uint8_t HGCalElectronicsId::halfrocChannel() { return (value_ >> kHalfROCChannelShift) & kHalfROCChannelMask; }
