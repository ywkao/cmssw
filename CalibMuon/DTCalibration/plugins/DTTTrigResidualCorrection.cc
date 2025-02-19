/*
 *  See header file for a description of this class.
 *
 *  \author A. Vilela Pereira
 */

#include "DTTTrigResidualCorrection.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DataFormats/MuonDetId/interface/DTSuperLayerId.h"
#include "CondFormats/DTObjects/interface/DTTtrig.h"
#include "CondFormats/DataRecord/interface/DTTtrigRcd.h"
#include "CondFormats/DTObjects/interface/DTMtime.h"
#include "CondFormats/DataRecord/interface/DTMtimeRcd.h"
#include "CondFormats/DTObjects/interface/DTRecoConditions.h"
#include "CondFormats/DataRecord/interface/DTRecoConditionsVdriftRcd.h"

#include "CalibMuon/DTCalibration/interface/DTResidualFitter.h"

#include "TFile.h"
#include "TH1F.h"
#include "TF1.h"
#include "TCanvas.h"

#include "RooPlot.h"
#include "RooRealVar.h"
#include "RooDataHist.h"
#include "RooGaussian.h"
#include "RooAddPdf.h"
#include "RooFitResult.h"
#include "RooGlobalFunc.h"

#include <string>
#include <sstream>
#include <fstream>

using namespace std;
using namespace edm;

namespace dtCalibration {

  DTTTrigResidualCorrection::DTTTrigResidualCorrection(const ParameterSet& pset) {
    string residualsRootFile = pset.getParameter<string>("residualsRootFile");
    rootFile_ = new TFile(residualsRootFile.c_str(), "READ");
    rootBaseDir_ = pset.getUntrackedParameter<string>("rootBaseDir", "/DQMData/DT/DTCalibValidation");
    useFit_ = pset.getParameter<bool>("useFitToResiduals");
    //useConstantvDrift_ = pset.getParameter<bool>("useConstantDriftVelocity");
    dbLabel_ = pset.getUntrackedParameter<string>("dbLabel", "");
    useSlopesCalib_ = pset.getUntrackedParameter<bool>("useSlopesCalib", false);
    readLegacyVDriftDB = pset.getParameter<bool>("readLegacyVDriftDB");

    // Load external slopes
    if (useSlopesCalib_) {
      ifstream fileSlopes;
      fileSlopes.open(pset.getParameter<FileInPath>("slopesFileName").fullPath().c_str());

      int tmp_wheel = 0, tmp_sector = 0, tmp_station = 0, tmp_SL = 0;
      double tmp_ttrig = 0., tmp_t0 = 0., tmp_kfact = 0.;
      int tmp_a = 0, tmp_b = 0, tmp_c = 0, tmp_d = 0;
      double tmp_v_eff = 0.;
      while (!fileSlopes.eof()) {
        fileSlopes >> tmp_wheel >> tmp_sector >> tmp_station >> tmp_SL >> tmp_a >> tmp_b >> tmp_ttrig >> tmp_t0 >>
            tmp_kfact >> tmp_c >> tmp_d >> tmp_v_eff;
        vDriftEff_[tmp_wheel + 2][tmp_sector - 1][tmp_station - 1][tmp_SL - 1] = -tmp_v_eff;
      }
      fileSlopes.close();
    }

    bool debug = pset.getUntrackedParameter<bool>("debug", false);
    fitter_ = new DTResidualFitter(debug);
  }

  DTTTrigResidualCorrection::~DTTTrigResidualCorrection() {
    delete rootFile_;
    delete fitter_;
  }

  void DTTTrigResidualCorrection::setES(const EventSetup& setup) {
    // Get tTrig record from DB
    ESHandle<DTTtrig> tTrig;
    //setup.get<DTTtrigRcd>().get(tTrig);
    setup.get<DTTtrigRcd>().get(dbLabel_, tTrig);
    tTrigMap_ = &*tTrig;

    // Get vDrift record
    if (readLegacyVDriftDB) {
      ESHandle<DTMtime> mTimeHandle;
      setup.get<DTMtimeRcd>().get(mTimeHandle);
      mTimeMap_ = &*mTimeHandle;
      vDriftMap_ = nullptr;
    } else {
      ESHandle<DTRecoConditions> hVdrift;
      setup.get<DTRecoConditionsVdriftRcd>().get(hVdrift);
      vDriftMap_ = &*hVdrift;
      mTimeMap_ = nullptr;
      // Consistency check: no parametrization is implemented for the time being
      int version = vDriftMap_->version();
      if (version != 1) {
        throw cms::Exception("Configuration") << "only version 1 is presently supported for VDriftDB";
      }
    }
  }

  DTTTrigData DTTTrigResidualCorrection::correction(const DTSuperLayerId& slId) {
    float tTrigMean, tTrigSigma, kFactor;
    int status = tTrigMap_->get(slId, tTrigMean, tTrigSigma, kFactor, DTTimeUnits::ns);
    if (status != 0)
      throw cms::Exception("[DTTTrigResidualCorrection]") << "Could not find tTrig entry in DB for" << slId << endl;

    float vDrift, hitResolution = 0.;
    if (readLegacyVDriftDB) {  // Legacy format
      status = mTimeMap_->get(slId, vDrift, hitResolution, DTVelocityUnits::cm_per_ns);
      if (status != 0)
        throw cms::Exception("[DTTTrigResidualCorrection]") << "Could not find vDrift entry in DB for" << slId << endl;
    } else {
      vDrift = vDriftMap_->get(DTWireId(slId.rawId()));
    }

    TH1F residualHisto = *(getHisto(slId));
    LogTrace("Calibration") << "[DTTTrigResidualCorrection]: \n"
                            << "   Mean, RMS     = " << residualHisto.GetMean() << ", " << residualHisto.GetRMS();

    double fitMean = -1.;
    double fitSigma = -1.;
    if (useFit_) {
      LogTrace("Calibration") << "[DTTTrigResidualCorrection]: Fitting histogram " << residualHisto.GetName();
      int nSigmas = 2;

      DTResidualFitResult fitResult = fitter_->fitResiduals(residualHisto, nSigmas);
      fitMean = fitResult.fitMean;
      fitSigma = fitResult.fitSigma;

      LogTrace("Calibration") << "[DTTTrigResidualCorrection]: \n"
                              << "   Fit Mean      = " << fitMean << "\n"
                              << "   Fit Sigma     = " << fitSigma;
    }
    double resMean = (useFit_) ? fitMean : residualHisto.GetMean();

    int wheel = slId.wheel();
    int sector = slId.sector();
    int station = slId.station();
    int superLayer = slId.superLayer();
    double resTime = 0.;
    if (useSlopesCalib_) {
      double vdrift_eff = vDriftEff_[wheel + 2][sector - 1][station - 1][superLayer - 1];
      if (vdrift_eff == 0)
        vdrift_eff = vDrift;

      if (vdrift_eff)
        resTime = resMean / vdrift_eff;

      LogTrace("Calibration") << "[DTTTrigResidualCorrection]: Effective vDrift, correction to tTrig = " << vdrift_eff
                              << ", " << resTime;
    } else {
      if (vDrift)
        resTime = resMean / vDrift;

      LogTrace("Calibration") << "[DTTTrigResidualCorrection]: vDrift from DB, correction to tTrig = " << vDrift << ", "
                              << resTime;
    }

    double corrMean = tTrigMean;
    double corrSigma = (tTrigSigma != 0.) ? tTrigSigma : 1.;
    double corrKFact = (tTrigSigma != 0.) ? (kFactor + resTime / tTrigSigma) : resTime;

    return DTTTrigData(corrMean, corrSigma, corrKFact);
  }

  const TH1F* DTTTrigResidualCorrection::getHisto(const DTSuperLayerId& slId) {
    string histoName = getHistoName(slId);
    LogTrace("Calibration") << "[DTTTrigResidualCorrection]: Accessing histogram " << histoName.c_str();
    TH1F* histo = static_cast<TH1F*>(rootFile_->Get(histoName.c_str()));
    if (!histo)
      throw cms::Exception("[DTTTrigResidualCorrection]") << "residual histogram not found:" << histoName << endl;
    return histo;
  }

  string DTTTrigResidualCorrection::getHistoName(const DTSuperLayerId& slId) {
    int step = 3;

    std::string wheel = std::to_string(slId.wheel());
    std::string station = std::to_string(slId.station());
    std::string sector = std::to_string(slId.sector());
    std::string superLayer = std::to_string(slId.superlayer());
    std::string Step = std::to_string(step);

    string histoName = rootBaseDir_ + "/Wheel" + wheel + "/Station" + station + "/Sector" + sector + "/hResDist_STEP" +
                       Step + "_W" + wheel + "_St" + station + "_Sec" + sector + "_SL" + superLayer;

    return histoName;
  }

}  // namespace dtCalibration
