#ifndef HGCalCommissioning_DQM_interface_HGCalDQMGeometry_h
#define HGCalCommissioning_DQM_interface_HGCalDQMGeometry_h

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "CondFormats/DataRecord/interface/HGCalElectronicsMappingRcd.h"
#include "CondFormats/DataRecord/interface/HGCalModuleConfigurationRcd.h"
#include "CondFormats/HGCalObjects/interface/HGCalConfiguration.h"
#include "CondFormats/HGCalObjects/interface/HGCalMappingModuleIndexer.h"
#include "CondFormats/HGCalObjects/interface/HGCalMappingModuleIndexerTrigger.h"
#include "CondFormats/HGCalObjects/interface/HGCalMappingParameterHost.h"

#include "DQM/HGCAL/interface/HGCalSysValDQMCommon.h"

class TFile;
class TGraph;

namespace hgcal {
  namespace dqm {

    // Instance-owned geometry/template helper. build() is the only mutator and
    // runs from a single-threaded harvester callback on the first LS. Tokens
    // are owned by the plugin; this class does not esConsumes.
    class HGCalDQMGeometry {
    public:
      // -----------------------------------------------------------------------
      // GROUP 1 TYPES (pure data)
      // -----------------------------------------------------------------------
      using MonitoredElementKey_t = std::pair<uint32_t, uint32_t>;

      struct MonitoredElement_t {
        std::string typecode;
        char irot;
        bool zside, isSiPM;
        std::vector<double> x0y0;
        uint32_t plane, layer, i1, i2, nErx, dqmIndex, fedid, modid,
                 econdidx, cassette, endcap, moduleIndex;
      };

      struct TriggerMonitoredElement_t {
        std::string typecode;
        bool zside, isSiPM;
        char irot;
        uint32_t plane, i1, i2, nTrLinks, nTrCells, fedid, modid, dqmIndex,
                 econtidx, cassette, moduleIndex, endcap, layer;
        std::vector<double> x0y0;
      };

      struct Timings {
        long long getModuleIndexer{0}, resizeVectors{0}, getModuleInfo{0},
                  openTFile{0}, calculateCorners{0};
        std::vector<long long> getModuleVariables;
      };

      HGCalDQMGeometry(std::string templateDir,
                       std::string geometryTemplate,
                       bool skipTriggerDQM,
                       std::string era = "");
      ~HGCalDQMGeometry();
      HGCalDQMGeometry(HGCalDQMGeometry const&) = delete;
      HGCalDQMGeometry& operator=(HGCalDQMGeometry const&) = delete;

      // Plugin owns the tokens (esConsumes in its ctor) and passes them in.
      void build(edm::EventSetup const& iSetup,
                 edm::ESGetToken<HGCalMappingModuleIndexer,        HGCalElectronicsMappingRcd> const& moduleIdxTkn,
                 edm::ESGetToken<HGCalMappingModuleIndexerTrigger, HGCalElectronicsMappingRcd> const& moduleIdxTriggerTkn,
                 edm::ESGetToken<hgcal::HGCalMappingModuleParamHost,        HGCalElectronicsMappingRcd> const& moduleInfoTkn,
                 edm::ESGetToken<hgcal::HGCalMappingModuleTriggerParamHost, HGCalElectronicsMappingRcd> const& moduleInfoTriggerTkn,
                 edm::ESGetToken<HGCalConfiguration, HGCalModuleConfigurationRcd> const& moduleConfigTkn);

      // ======================================================================
      // GROUP 1 — PURE DATA
      // All POD/STL. Candidate for an ESProducer conditions product later.
      // ======================================================================
      auto const& hgcalMap()                const { return HGCALMap_; }
      auto const& trigHgcalMap()            const { return TrigHGCALMap_; }
      auto const& triggerModuleMap()        const { return TriggerModuleMap_; }
      auto const& cornersLayer()            const { return corners_layer_; }
      auto const& cornersCassette()         const { return corners_cassette_; }
      auto const& typecodes()               const { return typecodes_; }
      auto const& directionalLayerStates()  const { return directionallayerstates_; }
      auto const& cassetteStates()          const { return cassettestates_; }
      auto const& moduleScopes()            const { return module_scopes_; }
      // Bounds of the rotated channel polygons used to book module plots.
      BoundingBox const& moduleChannelScope(MonitoredElement_t const& element) const;
      auto const& cassettesPerLayer()       const { return cassettesPerLayer_; }
      auto const& uniqueDirectionalLayers() const { return unique_directionallayers_; }
      size_t         nLayers()              const { return nLayers_; }
      bool           tileboardExists()      const { return flag_tileboard_exists_; }
      Timings const& timings()              const { return timings_; }

      // ======================================================================
      // GROUP 2 — ROOT OBJECT OWNERSHIP
      // Pointers into TFile handles owned here. Valid for this class's
      // lifetime. NEVER hand to a conditions product. Callers must not
      // delete/close.
      // ======================================================================
      TGraph* moduleBin(uint32_t dqmIndex)  const;
      TGraph* triggerModuleBin(uint32_t dqmIndex) const;

      // Two semantic entry points, one cache. Both resolve their filename and
      // route through the private templateFile(path) accessor — no other open
      // paths exist.
      TFile* moduleTemplateFile(std::string const& typecode, bool isSiPM) const;  // → wafer.root or tileboard.root
      TFile* trigTemplateFile  (std::string const& typecode, bool isSiPM);  // → TC_wafer.root

      // -----------------------------------------------------------------------
      // Stateless geometry utilities.
      // -----------------------------------------------------------------------
      static void rotateShape(TGraph* gr, char irot, double offset = 0.0);
      static void translateBin(TGraph* gr, float x0, float y0);

      // Exposed for cache-invariant regression checks (returns the number of
      // distinct template files opened over the lifetime of this instance).
      size_t cachedFileCount() const { return templateFiles_.size(); }

    private:
      // Sole open path. Opens on miss, returns cached on hit.
      TFile* templateFile(std::string const& resolvedPath) const;

      // Filename-construction helper shared by moduleTemplateFile/trigTemplateFile.
      static std::string sipmLookupKeyFromTypecode(std::string const& typecode);

      // Template-file readers. Ownership: file owns everything returned;
      // callers borrow, never delete.
      TGraph*             readModuleBin   (TFile* file, bool isSiPM, int plane, int u, int v);
      std::vector<double> readModuleCenter(TFile* file, bool isSiPM, int plane, int u, int v);
      BoundingBox         readModuleScope (TFile* file, bool isSiPM, int plane, int u, int v);
      BoundingBox         readModuleChannelScope(TFile* file, char irot, int v) const;

      void calculatePlotsCorners();
      void timerStart();
      long long timerSave();

      // ---- config ----
      std::string templateDir_;
      std::string geometryTemplate_;
      std::string era_;
      bool skipTriggerDQM_;

      // ---- GROUP 1 storage ----
      std::map<int, std::map<int, std::map<int, std::map<std::string, MonitoredElement_t>>>> HGCALMap_;
      std::map<int, std::map<int, std::map<int, std::set<MonitoredElementKey_t>>>>           TrigHGCALMap_;
      std::map<MonitoredElementKey_t, TriggerMonitoredElement_t>                             TriggerModuleMap_;
      std::map<int, std::array<float, 4>>                corners_layer_;
      std::map<int, std::map<int, std::array<float, 4>>> corners_cassette_;
      std::vector<std::string>                           typecodes_;
      std::vector<int>                                   directionallayerstates_, cassettestates_;
      std::vector<BoundingBox>                           module_scopes_;          // module outlines
      mutable std::vector<BoundingBox>                   module_channel_scopes_;  // channel plot bounds
      std::map<int, std::set<int>>                       cassettesPerLayer_;
      std::set<int>                                      unique_directionallayers_;
      size_t                                             nLayers_{0};
      bool                                               flag_tileboard_exists_{false};
      Timings                                            timings_;

      // ---- GROUP 2 storage ----
      std::vector<TGraph*>          binstates_;      // moduleBin(dqmIndex) → binstates_[dqmIndex]
      std::vector<TGraph*>          trigBinstates_;  // triggerModuleBin(dqmIndex) → trigBinstates_[dqmIndex]
      mutable std::map<std::string, TFile*> templateFiles_;  // closed/deleted in dtor

      std::chrono::time_point<std::chrono::high_resolution_clock> startTime_;
    };

  }  // namespace dqm
}  // namespace hgcal

#endif
