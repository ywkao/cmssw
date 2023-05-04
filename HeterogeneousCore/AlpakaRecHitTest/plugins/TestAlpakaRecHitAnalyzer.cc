#include <cassert>

//#include "DataFormats/PortableTestObjects/interface/TestHostCollection.h"
#include "DataFormats/PortableTestObjects/interface/TestHostRecHitCollection.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDAnalyzer.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"

namespace {

  template <typename T>
  class Column {
  public:
    Column(T const* data, size_t size) : data_(data), size_(size) {}

    void print(std::ostream& out) const {
      std::stringstream buffer;
      buffer << "{ ";
      if (size_ > 0) {
        buffer << data_[0];
      }
      if (size_ > 1) {
        buffer << ", " << data_[1];
      }
      if (size_ > 2) {
        buffer << ", " << data_[2];
      }
      if (size_ > 3) {
        buffer << ", ...";
      }
      buffer << '}';
      out << buffer.str();
    }

  private:
    T const* const data_;
    size_t const size_;
  };

  template <typename T>
  std::ostream& operator<<(std::ostream& out, Column<T> const& column) {
    column.print(out);
    return out;
  }

  // template <typename T>
  // void checkViewAddresses(T const& view) {
  //   assert(view.metadata().addressOf_x() == view.x());
  //   assert(view.metadata().addressOf_x() == &view.x(0));
  //   assert(view.metadata().addressOf_x() == &view[0].x());
  //   assert(view.metadata().addressOf_y() == view.y());
  //   assert(view.metadata().addressOf_y() == &view.y(0));
  //   assert(view.metadata().addressOf_y() == &view[0].y());
  //   assert(view.metadata().addressOf_z() == view.z());
  //   assert(view.metadata().addressOf_z() == &view.z(0));
  //   assert(view.metadata().addressOf_z() == &view[0].z());
  //   assert(view.metadata().addressOf_id() == view.id());
  //   assert(view.metadata().addressOf_id() == &view.id(0));
  //   assert(view.metadata().addressOf_id() == &view[0].id());
  //   assert(view.metadata().addressOf_m() == view.m());
  //   assert(view.metadata().addressOf_m() == &view.m(0).coeffRef(0, 0));
  //   assert(view.metadata().addressOf_m() == &view[0].m().coeffRef(0, 0));
  //   assert(view.metadata().addressOf_r() == &view.r());
  //   //assert(view.metadata().addressOf_r() == &view.r(0));                  // cannot access a scalar with an index
  //   //assert(view.metadata().addressOf_r() == &view[0].r());                // cannot access a scalar via a SoA row-like accessor
  // }

}  // namespace

class TestAlpakaRecHitAnalyzer : public edm::stream::EDAnalyzer<> {
public:
  TestAlpakaRecHitAnalyzer(edm::ParameterSet const& config)
      : source_{config.getParameter<edm::InputTag>("source")},
        token_{consumes(source_)},
        expectSize_(config.getParameter<int>("expectSize")) {}

  void analyze(edm::Event const& event, edm::EventSetup const&) override {
    portabletest::TestHostRecHitCollection const& product = event.get(token_);
    auto const& view = product.const_view();
    auto& mview = product.view();
    auto const& cmview = product.view();

    if (expectSize_ >= 0 and expectSize_ != view.metadata().size()) {
      throw cms::Exception("Assert") << "Expected input collection size " << expectSize_ << ", got "
                                     << view.metadata().size();
    }

    printf("[INFO] Hello World from analyze()!\n");

    {
      edm::LogInfo msg("TestAlpakaRecHitAnalyzer");
      msg << source_.encode() << ".size() = " << view.metadata().size() << '\n';
      msg << "  data        @ " << product.buffer().data() << ",\n"
          << "  event       @ " << view.metadata().addressOf_event()       << " = " << Column(view.event(), view.metadata().size())       << ",\n"
          << "  detid       @ " << view.metadata().addressOf_detid()       << " = " << Column(view.detid(), view.metadata().size())       << ",\n"
          << "  adc         @ " << view.metadata().addressOf_adc()         << " = " << Column(view.adc(), view.metadata().size())         << ",\n"
          << "  adcm        @ " << view.metadata().addressOf_adcm()        << " = " << Column(view.adcm(), view.metadata().size())        << ",\n"
          << "  adc_cm      @ " << view.metadata().addressOf_adc_cm()      << " = " << Column(view.adc_cm(), view.metadata().size())      << ",\n"
          << "  toa         @ " << view.metadata().addressOf_toa()         << " = " << Column(view.toa(), view.metadata().size())         << ",\n"
          << "  tot         @ " << view.metadata().addressOf_tot()         << " = " << Column(view.tot(), view.metadata().size())         << ",\n"
          << "  triglatency @ " << view.metadata().addressOf_triglatency() << " = " << Column(view.triglatency(), view.metadata().size()) << ",\n"
          << " } ... } \n";
    }

    // {
    //   edm::LogInfo msg("TestAlpakaRecHitAnalyzer");
    //   msg << source_.encode() << ".size() = " << view.metadata().size() << '\n';
    //   msg << "  data @ " << product.buffer().data() << ",\n"
    //       << "  x    @ " << view.metadata().addressOf_x() << " = " << Column(view.x(), view.metadata().size()) << ",\n"
    //       << "  y    @ " << view.metadata().addressOf_y() << " = " << Column(view.y(), view.metadata().size()) << ",\n"
    //       << "  z    @ " << view.metadata().addressOf_z() << " = " << Column(view.z(), view.metadata().size()) << ",\n"
    //       << "  id   @ " << view.metadata().addressOf_id() << " = " << Column(view.id(), view.metadata().size())
    //       << ",\n"
    //       << "  r    @ " << view.metadata().addressOf_r() << " = " << view.r() << '\n'
    //       << "  m    @ " << view.metadata().addressOf_m() << " = { ... {" << view[1].m()(1, Eigen::all)
    //       << " } ... } \n";
    //   msg << std::hex << "  [y - x] = 0x"
    //       << reinterpret_cast<intptr_t>(view.metadata().addressOf_y()) -
    //              reinterpret_cast<intptr_t>(view.metadata().addressOf_x())
    //       << "  [z - y] = 0x"
    //       << reinterpret_cast<intptr_t>(view.metadata().addressOf_z()) -
    //              reinterpret_cast<intptr_t>(view.metadata().addressOf_y())
    //       << "  [id - z] = 0x"
    //       << reinterpret_cast<intptr_t>(view.metadata().addressOf_id()) -
    //              reinterpret_cast<intptr_t>(view.metadata().addressOf_z())
    //       << "  [r - id] = 0x"
    //       << reinterpret_cast<intptr_t>(view.metadata().addressOf_r()) -
    //              reinterpret_cast<intptr_t>(view.metadata().addressOf_id())
    //       << "  [m - r] = 0x"
    //       << reinterpret_cast<intptr_t>(view.metadata().addressOf_m()) -
    //              reinterpret_cast<intptr_t>(view.metadata().addressOf_r());
    // }

    // checkViewAddresses(view);
    // checkViewAddresses(mview);
    // checkViewAddresses(cmview);

    // const portabletest::Matrix matrix{{1, 2, 3, 4, 5, 6}, {2, 4, 6, 8, 10, 12}, {3, 6, 9, 12, 15, 18}};
    // assert(view.r() == 1.);
    // for (int32_t i = 0; i < view.metadata().size(); ++i) {
    //   auto vi = view[i];
    //   assert(vi.x() == 0.);
    //   assert(vi.y() == 0.);
    //   assert(vi.z() == 0.);
    //   assert(vi.id() == i);
    //   assert(vi.m() == matrix * i);
    // }
  }

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("source");
    desc.add<int>("expectSize", -1)
        ->setComment("Expected size of the input collection. Values < 0 mean the check is not performed. Default: -1");
    descriptions.addWithDefaultLabel(desc);
  }

private:
  const edm::InputTag source_;
  const edm::EDGetTokenT<portabletest::TestHostRecHitCollection> token_;
  const int expectSize_;
};

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(TestAlpakaRecHitAnalyzer);
