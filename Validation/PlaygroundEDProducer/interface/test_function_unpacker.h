#ifndef __test_function_unpacker_h__
#define __test_function_unpacker_h__

#include "EventFilter/HGCalRawToDigi/interface/HGCalUnpacker.h"
#include "DataFormats/HGCalDigi/interface/HGCalElectronicsId.h"

//
// Test functions for ECOND unpacker
//

uint32_t little_to_big(uint32_t num) {
    return ((num & 0x000000ff) << 24) | ((num & 0x0000ff00) << 8) | ((num & 0x00ff0000) >> 8) | ((num & 0xff000000) >> 24);
}

uint16_t enabledERXMapping(uint16_t sLink, uint8_t captureBlock, uint8_t econd) { return 0b000111101101; }
HGCalElectronicsId logicalMapping(HGCalElectronicsId elecID) { return elecID; }

std::vector<uint32_t> readRawDataFrom(std::string fname, bool doHost2BigEndian) {
  std::vector<uint32_t> data;
  
  //open file
  FILE* in_fptr = fopen(fname.c_str(),"rb");
  if (in_fptr == NULL) {
    std::cout << "Failed to read input file: " << fname << std::endl;
    return data;
  }
  
  //determine file size in 4Byte words
  auto start=ftell(in_fptr);
  fseek(in_fptr, 0, SEEK_END); //go to the end of the file
  auto end=ftell(in_fptr);
  const auto fsize = (end-start)/sizeof(uint32_t); //measure the difference
  fseek(in_fptr, 0, SEEK_SET); //set again the position at start
  
  //read file
  data.resize(fsize);
  uint32_t *p = new uint32_t;
  for(uint32_t i=0; i<fsize; i++) {
    assert(fread(p, sizeof *p, 1, in_fptr)==1);
    //data[i]=doHost2BigEndian ? htobe32(*p) : *p;
    data[i]=doHost2BigEndian ? little_to_big(*p) : *p;
  }
  delete p;

  return data;
}

#endif // __test_function_unpacker_h__
