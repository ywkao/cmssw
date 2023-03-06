#ifndef DataFormats_PortableTestObjects_interface_TestHostCollection_h
#define DataFormats_PortableTestObjects_interface_TestHostCollection_h

#include "DataFormats/Portable/interface/PortableHostCollection.h"
#include "DataFormats/PortableTestObjects/interface/TestSoA.h"
#include "DataFormats/PortableTestObjects/interface/TestRecHitSoA.h"

namespace portabletest {

  // SoA with x, y, z, id fields in host memory
  using TestHostCollection = PortableHostCollection<TestSoA>;
  using TestHostRecHitCollection = PortableHostCollection<TestRecHitSoA>;

}  // namespace portabletest

#endif  // DataFormats_PortableTestObjects_interface_TestHostCollection_h
