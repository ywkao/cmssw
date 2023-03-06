#ifndef DataFormats_PortableTestObjects_interface_TestRecHitCollection_h
#define DataFormats_PortableTestObjects_interface_TestRecHitCollection_h

#include <Eigen/Core>
#include <Eigen/Dense>

#include "DataFormats/SoATemplate/interface/SoACommon.h"
#include "DataFormats/SoATemplate/interface/SoALayout.h"
#include "DataFormats/SoATemplate/interface/SoAView.h"

namespace portabletest {

  // Generate structure of arrays (SoA) layout with RecHit dataformat
  GENERATE_SOA_LAYOUT(TestRecHitSoA_,
                      // columns: one value per element
                      SOA_COLUMN(int32_t, event),
                      SOA_COLUMN(int32_t, detid),
                      SOA_COLUMN(double , adc),
                      SOA_COLUMN(double , adcm),
                      SOA_COLUMN(double , adc_cm),
                      SOA_COLUMN(double , toa),
                      SOA_COLUMN(double , tot),
                      SOA_COLUMN(double , triglatency),
  )

  using TestRecHitSoA = TestRecHitSoA_<>;

}  // namespace portabletest

#endif  // DataFormats_PortableTestObjects_interface_TestRecHitCollection_h
