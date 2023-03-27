#include "DataFormats/Common/interface/Wrapper.h"
#include "Validation/PlaygroundEDProducer/interface/hgcalhit.h"

namespace {
    struct dictionary {
        DetectorId detId;
        RecHit hit;
        edm::Wrapper<RecHit> wra_Hit;
    };
}
