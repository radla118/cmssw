/** \class GEMTriggerPrimitivesAnalyzer
 *
 * Basic analyzer class accesses GEM TPs and plot various quantities. This analyzer
 * can currently load a DQM file and plot the data vs emulation of GEM TPs.
 *
 * \author Sven Dildick (Rice University)
 *
 */

#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "TH1F.h"
#include "TPostScript.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TText.h"
#include "TPaveLabel.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TROOT.h"

#include <iostream>
#include <string>

class GEMTriggerPrimitivesAnalyzer : public edm::one::EDAnalyzer<> {
public:
  /// Constructor
  explicit GEMTriggerPrimitivesAnalyzer(const edm::ParameterSet &conf);

  /// Destructor
  ~GEMTriggerPrimitivesAnalyzer() override {}

  /// Does the job
  void analyze(const edm::Event &event, const edm::EventSetup &setup) override;

  /// Write to ROOT file, make plots, etc.
  void endJob() override;

private:
  void makePlot(TH1F *dataMon,
                TH1F *emulMon,
                TH1F *diffMon,
                TString lcts,
                TString lct,
                TString var,
                TString chamber,
                TPostScript *ps,
                TCanvas *c1) const;

  // plots of data vs emulator
  std::string rootFileName_;
  unsigned runNumber_;
  std::string monitorDir_;
  std::vector<std::string> chambers_;
  std::vector<std::string> clusterVars_;
  bool dataVsEmulatorPlots_;
  void makeDataVsEmulatorPlots();

  // plots of efficiencies in MC
  bool mcEfficiencyPlots_;

  // plots of resolution in MC
  bool mcResolutionPlots_;

  /*
    When set to True, we assume that the data comes from
    the Building 904 GEM test-stand. This test-stand is a single
    ME1/1 chamber.
  */
  bool B904Setup_;
  // label only relevant for B904 local runs
  std::string B904RunNumber_;
};

GEMTriggerPrimitivesAnalyzer::GEMTriggerPrimitivesAnalyzer(const edm::ParameterSet &conf)
    : rootFileName_(conf.getParameter<std::string>("rootFileName")),
      runNumber_(conf.getParameter<unsigned>("runNumber")),
      monitorDir_(conf.getParameter<std::string>("monitorDir")),
      chambers_(conf.getParameter<std::vector<std::string>>("chambers")),
      clusterVars_(conf.getParameter<std::vector<std::string>>("clusterVars")),
      dataVsEmulatorPlots_(conf.getParameter<bool>("dataVsEmulatorPlots")),
      mcEfficiencyPlots_(conf.getParameter<bool>("mcEfficiencyPlots")),
      mcResolutionPlots_(conf.getParameter<bool>("mcResolutionPlots")),
      B904Setup_(conf.getParameter<bool>("B904Setup")),
      B904RunNumber_(conf.getParameter<std::string>("B904RunNumber")) {}

void GEMTriggerPrimitivesAnalyzer::analyze(const edm::Event &ev, const edm::EventSetup &setup) {
  // efficiency and resolution analysis is done here
}

void GEMTriggerPrimitivesAnalyzer::endJob() {
  if (dataVsEmulatorPlots_)
    makeDataVsEmulatorPlots();
}

void GEMTriggerPrimitivesAnalyzer::makeDataVsEmulatorPlots() {
  // data vs emulator plots are created here
  edm::Service<TFileService> fs;

  // split monitorDir_ into two substrings
  std::string delimiter = "/";
  int pos = monitorDir_.find(delimiter);
  std::string superDir = monitorDir_.substr(0, pos);
  monitorDir_.erase(0, pos + delimiter.length());
  std::string subDir = monitorDir_;
  std::string path = "DQMData/Run " + std::to_string(runNumber_) + "/" + superDir + "/Run summary/" + subDir + "/";

  TFile *theFile = new TFile(rootFileName_.c_str());
  if (!theFile) {
    edm::LogError("GEMTriggerPrimitivesAnalyzer") << "Unable to load DQM ROOT file: " << rootFileName_;
    return;
  }

  TDirectory *directory = theFile->GetDirectory(path.c_str());
  if (!directory) {
    edm::LogError("GEMTriggerPrimitivesAnalyzer") << "Unable to navigate to directory: " << path;
    return;
  }

  TString runTitle = "CMS_Run_" + std::to_string(runNumber_);
  if (B904Setup_)
    runTitle = "B904_Cosmic_Run_" + TString(B904RunNumber_);

  TPostScript *ps = new TPostScript("GEM_dataVsEmul_" + runTitle + ".ps", 111);
  TCanvas *c1 = new TCanvas("c1", "", 800, 800);
  c1->Clear();
  c1->Divide(1, 2);

  // alct variable
  for (unsigned iVar = 0; iVar < clusterVars_.size(); iVar++) {
    // chamber type
    for (unsigned iType = 0; iType < chambers_.size(); iType++) {
      const std::string key("cluster_" + clusterVars_[iVar]);
      const std::string histData(key + "_data_" + chambers_[iType]);
      const std::string histEmul(key + "_emul_" + chambers_[iType]);
      const std::string histDiff(key + "_diff_" + chambers_[iType]);

      TH1F *dataMon = (TH1F *)directory->Get(histData.c_str());
      TH1F *emulMon = (TH1F *)directory->Get(histEmul.c_str());
      TH1F *diffMon = (TH1F *)directory->Get(histDiff.c_str());

      // when all histograms are found, make a new canvas and add it to
      // the collection
      if (dataMon && emulMon && diffMon) {
        makePlot(dataMon,
                 emulMon,
                 diffMon,
                 "GEM Cluster",
                 "gemcluster_",
                 TString(clusterVars_[iVar]),
                 TString(chambers_[iType]),
                 ps,
                 c1);
      }
    }
  }

  ps->Close();
  // close the DQM file
  theFile->Close();
  delete c1;
  delete ps;
}

void GEMTriggerPrimitivesAnalyzer::makePlot(TH1F *dataMon,
                                            TH1F *emulMon,
                                            TH1F *diffMon,
                                            TString lcts,
                                            TString lct,
                                            TString var,
                                            TString chamber,
                                            TPostScript *ps,
                                            TCanvas *c1) const {
  ps->NewPage();

  TString runTitle = "(CMS Run " + std::to_string(runNumber_) + ")";
  if (B904Setup_)
    runTitle = "(B904 Cosmic Run " + TString(B904RunNumber_) + ")";
  const TString title(chamber + " " + lcts + " " + var + " " + runTitle);
  c1->cd(1);
  gPad->SetGridx();
  gPad->SetGridy();
  gStyle->SetOptStat(1111);
  dataMon->SetTitle(title);
  dataMon->GetXaxis()->SetTitle(lcts + " " + var);
  dataMon->GetYaxis()->SetTitle("Entries");
  dataMon->SetMarkerColor(kBlack);
  dataMon->SetMarkerStyle(kPlus);
  dataMon->SetMarkerSize(3);
  // add 50% to make sure the legend does not overlap with the histograms
  dataMon->SetMaximum(dataMon->GetBinContent(dataMon->GetMaximumBin()) * 1.5);
  dataMon->Draw("histp");
  dataMon->GetXaxis()->SetLabelSize(0.05);
  dataMon->GetYaxis()->SetLabelSize(0.05);
  dataMon->GetXaxis()->SetTitleSize(0.05);
  dataMon->GetYaxis()->SetTitleSize(0.05);
  emulMon->SetLineColor(kRed);
  emulMon->Draw("histsame");
  auto legend = new TLegend(0.7, 0.7, 0.9, 0.9);
  legend->AddEntry(dataMon, "Data", "p");
  legend->AddEntry(emulMon, "Emulator", "l");
  legend->Draw();

  c1->cd(2);
  gPad->SetGridx();
  gPad->SetGridy();
  gStyle->SetOptStat(0);
  diffMon->SetLineColor(kBlack);
  diffMon->SetTitle(title);
  diffMon->GetXaxis()->SetTitle(lcts + " " + var);
  diffMon->GetYaxis()->SetTitle("Emul - Data");
  diffMon->GetXaxis()->SetLabelSize(0.05);
  diffMon->GetYaxis()->SetLabelSize(0.05);
  diffMon->GetXaxis()->SetTitleSize(0.05);
  diffMon->GetYaxis()->SetTitleSize(0.05);
  diffMon->Draw("ep");
  c1->Update();
}

DEFINE_FWK_MODULE(GEMTriggerPrimitivesAnalyzer);
