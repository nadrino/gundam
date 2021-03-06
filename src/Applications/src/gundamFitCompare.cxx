//
// Created by Adrien BLANCHET on 12/05/2022.
//

#include <TLegend.h>
#include "GundamGreetings.h"

#include "Logger.h"
#include "CmdLineParser.h"
#include "GenericToolbox.Root.h"

#include "nlohmann/json.hpp"

#include "string"
#include "vector"

LoggerInit([]{
  Logger::setUserHeaderStr("[gundamFitCompare.cxx]");
  Logger::setPrefixFormat("{USER_HEADER}");
});

///\description gundamFitCompare is an application that takes 2 output files generated by gundamFitter and generates
/// post-fit error comparisons.
///
/// \usage
/// \arg -f1:

int main( int argc, char** argv ){
  GundamGreetings g;
  g.setAppName("FitCompare");
  g.hello();

  CmdLineParser clp(argc, argv);

  // files
  clp.addOption("file-1", {"-f1"}, "Path to first output fit file.", 1);
  clp.addOption("file-2", {"-f2"}, "Path to second output fit file.", 1);

  // display name
  clp.addOption("name-1", {"-n1"}, "Set display name of the first fit file.", 1);
  clp.addOption("name-2", {"-n2"}, "Set display name of the  fit file.", 1);

  // display name
  clp.addOption("algo-1", {"-a1"}, "Specify algo folder to compare for the first fit file.", 1);
  clp.addOption("algo-2", {"-a2"}, "Specify algo folder to compare for the second fit file.", 1);

  clp.addOption("output", {"-o"}, "Output file.", 1);



  clp.parseCmdLine();

  if( clp.isNoOptionTriggered()
      or not clp.isOptionTriggered("file-1")
      or not clp.isOptionTriggered("file-2")
      or not clp.isOptionTriggered("output")
      ){
    LogError << "Missing options. Reminding usage..." << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl;
    exit(EXIT_FAILURE);
  }

  LogInfo << "Reading config..." << std::endl;
  auto filePath1 = clp.getOptionVal<std::string>("file-1");
  auto filePath2 = clp.getOptionVal<std::string>("file-2");

  auto outPath = clp.getOptionVal<std::string>("output");

  auto* file1 = GenericToolbox::openExistingTFile(filePath1);
  auto* file2 = GenericToolbox::openExistingTFile(filePath2);

  auto algo1 = clp.getOptionVal("algo-1", "Migrad");
  auto algo2 = clp.getOptionVal("algo-2", "Migrad");

  auto name1 = clp.getOptionVal("name-1", filePath1);
  auto name2 = clp.getOptionVal("name-2", filePath2);

  std::string strBuffer;

  strBuffer = Form("FitterEngine/postFit/%s/errors", algo1.c_str());
  auto* dir1 = file1->Get<TDirectory>(strBuffer.c_str());
  LogThrowIf(dir1== nullptr, "Could not find \"" << strBuffer << "\" within " << filePath1);

  strBuffer = Form("FitterEngine/postFit/%s/errors", algo2.c_str());
  auto* dir2 = file2->Get<TDirectory>(strBuffer.c_str());
  LogThrowIf(dir2== nullptr, "Could not find \"" << strBuffer << "\" within " << filePath2);

  auto* outFile = TFile::Open(outPath.c_str(), "RECREATE");

  // loop over datasets
  for( int iKey = 0 ; iKey < dir1->GetListOfKeys()->GetEntries() ; iKey++ ){

    std::string parSet = dir1->GetListOfKeys()->At(iKey)->GetName();


    auto* hist1 = dir1->Get<TH1D>(Form("%s/values/postFitErrors_TH1D", dir1->GetListOfKeys()->At(iKey)->GetName()));
    if( hist1 == nullptr ){
      LogError << "Could no find parSet \"" << dir1->GetListOfKeys()->At(iKey)->GetName() << "\" in " << file1->GetPath() << std::endl;
      continue;
    }

    auto* hist2 = dir2->Get<TH1D>(Form("%s/values/postFitErrors_TH1D", dir1->GetListOfKeys()->At(iKey)->GetName()));
    if( hist2 == nullptr ){
      LogError << "Could no find parSet \"" << dir1->GetListOfKeys()->At(iKey)->GetName() << "\" in " << file2->GetPath() << std::endl;
      continue;
    }

    LogInfo << "Processing parameter set: \"" << dir1->GetListOfKeys()->At(iKey)->GetName() << "\"" << std::endl;

    auto* overlayCanvas = new TCanvas( "overlay_TCanvas" ,"", 1280, 720);
    hist1->SetFillColor(kRed-9);
    hist1->SetMarkerStyle(kFullDotLarge);
    hist1->SetMarkerColor(kRed-3);
    hist1->SetMarkerSize(0);
    hist1->SetLabelSize(0.02);
    hist1->SetTitle(Form("%s (%s)", name1.c_str(), algo1.c_str()));
    hist1->GetXaxis()->SetLabelSize(0.03);
    hist1->GetXaxis()->LabelsOption("v");
    hist1->Draw("E2");

    TH1D hist1Line = TH1D("banffHistLine", "banffHistLine",
                          hist1->GetNbinsX(),
                          hist1->GetXaxis()->GetXmin(),
                          hist1->GetXaxis()->GetXmax()
    );
    GenericToolbox::transformBinContent(&hist1Line, [&](TH1D* h_, int b_){
      h_->SetBinContent(b_, hist1->GetBinContent(b_));
    });

    hist1Line.SetLineColor(kRed-3);
    hist1Line.Draw("SAME");

    hist2->SetLineColor(9);
    hist2->SetLineWidth(2);
    hist2->SetMarkerColor(9);
    hist2->SetMarkerStyle(kFullDotLarge);
    hist2->SetTitle(Form("%s (%s)", name2.c_str(), algo2.c_str()));
    hist2->Draw("E1 X0 SAME");

    gPad->SetGridx();
    gPad->SetGridy();

    TLegend l(0.7, 0.8, 0.9, 0.9);
    l.AddEntry(hist1, hist1->GetTitle());
    l.AddEntry(hist2, hist2->GetTitle());
    l.Draw();

    hist1->SetTitle(Form("%s", dir1->GetListOfKeys()->At(iKey)->GetName()));
    GenericToolbox::mkdirTFile(outFile, dir1->GetListOfKeys()->At(iKey)->GetName())->cd();
    overlayCanvas->Write();

  }

  outFile->Close();


  return EXIT_SUCCESS;
}