//
// Created by Adrien BLANCHET on 02/06/2021.
//

#include <TChain.h>
#include <TTreeReader.h>
#include <TFormula.h>
#include "AnaEvent.hh"

#include "Logger.h"

LoggerInit([](){
  Logger::setUserHeaderStr("[AnaEvent]");
})


AnaEvent::AnaEvent(){
  _anaEventType_     = AnaEventType::MC;
  _intNameListPtr_ = nullptr;
  _floatNameListPtr_ = nullptr;
  reset();
}
AnaEvent::AnaEvent(AnaEventType::AnaEventType anaEventType_) {
  _anaEventType_ = anaEventType_;
  _intNameListPtr_ = nullptr;
  _floatNameListPtr_ = nullptr;
  reset();
}
AnaEvent::AnaEvent(long eventId_) {
  _anaEventType_     = AnaEventType::MC;
  _intNameListPtr_ = nullptr;
  _floatNameListPtr_ = nullptr;
  reset();
  m_evid = eventId_;
}

void AnaEvent::reset() {

  delete _singleEntryTree_; _singleEntryTree_ = nullptr;

  _isBeingEdited_ = false;
  _treeEventHasBeenDumped_ = false;

  // long int
  m_evid = -1;

  _trueBinIndex_ = -1;
  _recoBinIndex_ = -1;

  // bool
  m_signal   = false;
  m_true_evt = false;

  ResetIntContainer();
  ResetFloatContainer();

}

// Setters
void AnaEvent::SetAnaEventType(AnaEventType::AnaEventType anaEventType_) {
  _anaEventType_ = anaEventType_;
}
void AnaEvent::SetEventId(long evid) { m_evid = evid; }
void AnaEvent::SetIntVarNameListPtr(std::vector<std::string> *intNameListPtr_) {
  // Copy old values
  std::vector<Int_t> newIntValuesList(intNameListPtr_->size());
  for( size_t iName = 0 ; iName < _intNameListPtr_->size() ; iName++ ){
    for( size_t jName = 0 ; jName < intNameListPtr_->size() ; jName++ ){
      if(_intNameListPtr_->at(iName) == intNameListPtr_->at(jName)){
        newIntValuesList.at(jName) = _intValuesList_.at(iName);
      }
    }
  }

  _intNameListPtr_ = intNameListPtr_;
  _defaultIntNameList_.clear(); // save memory since it's not used anymore

  _intValuesList_ = newIntValuesList;
  HookIntMembers();
}
void AnaEvent::SetFloatVarNameListPtr(std::vector<std::string> *floatNameListPtr_) {
  // Copy old values
  std::vector<Float_t> newFloatValuesList(floatNameListPtr_->size());
  for( size_t iName = 0 ; iName < _floatNameListPtr_->size() ; iName++ ){
    for( size_t jName = 0 ; jName < floatNameListPtr_->size() ; jName++ ){
      if(_floatNameListPtr_->at(iName) == floatNameListPtr_->at(jName)){
        newFloatValuesList.at(jName) = _floatValuesList_.at(iName);
      }
    }
  }

  _floatNameListPtr_ = floatNameListPtr_;
  _defaultFloatNameList_.clear(); // save memory since it's not used anymore
  _floatValuesList_ = newFloatValuesList;
  HookFloatMembers();
}


// Init
void AnaEvent::DumpTreeEntryContent(TTree *tree_) {

  if(_treeEventHasBeenDumped_) return;

  TLeaf* leafBuffer = nullptr;
  int index;
  for( int iKey = 0 ; iKey < tree_->GetListOfLeaves()->GetEntries() ; iKey++ ){
    leafBuffer = (TLeaf*) tree_->GetListOfLeaves()->At(iKey);

    if( std::string(leafBuffer->GetTypeName()) == "Int_t" ){
      index = GetIntIndex(leafBuffer->GetName(), false);
      if(index != -1){
        _intValuesList_[index] = Int_t(leafBuffer->GetValue(0));
      }
    }
    else if( std::string(leafBuffer->GetTypeName()) == "Float_t" ){
      index = GetFloatIndex(leafBuffer->GetName(), false);
      if(index != -1){
        _floatValuesList_[index] = Float_t(leafBuffer->GetValue(0));
      }
    }

  }

  _treeEventHasBeenDumped_ = true;

}
double AnaEvent::GetEventVarAsDouble(const std::string &varName_) const {

  int index;

  index = GenericToolbox::findElementIndex(varName_, *_floatNameListPtr_);
  if( index != -1 ) return _floatValuesList_.at(index);

  index = GenericToolbox::findElementIndex(varName_, *_intNameListPtr_);
  if( index != -1 ) return _intValuesList_.at(index);

  LogFatal << "Could not find variable \"" << varName_ << "\" in both int and float lists." << std::endl;
  LogFatal << "Available floats are: ";
  GenericToolbox::printVector(*_floatNameListPtr_);
  LogFatal << std::endl;
  LogFatal << "Available ints are: ";
  GenericToolbox::printVector(*_intNameListPtr_);
  LogFatal << std::endl;
  throw std::logic_error("Could not find variable");

}
void AnaEvent::HookIntMembers() {
  // Hooking ptr members for faster access
  // CAVEAT: all these vars have to be in _intNameListPtr_
  _flavorPtr_ = &_intValuesList_[GetIntIndex("nutype")];
  _beamModePtr_ = &_intValuesList_[GetIntIndex("beammode")];
  _topologyPtr_ = &_intValuesList_[GetIntIndex("topology")];
  _reactionPtr_ = &_intValuesList_[GetIntIndex("reaction")];
  _targetPtr_ = &_intValuesList_[GetIntIndex("target")];
  _samplePtr_ = &_intValuesList_[GetIntIndex("cut_branch")];
  _sigTypePtr_ = &_intValuesList_[GetIntIndex("signal")];
}
void AnaEvent::HookFloatMembers() {
  // Hooking ptr members for faster access
  // CAVEAT: all these vars have to be in _floatNameListPtr_
  _enuTruePtr_ = &_floatValuesList_[GetFloatIndex("enu_true")];
  _enuRecoPtr_ = &_floatValuesList_[GetFloatIndex("enu_reco")];
  _d1TruePtr_ = &_floatValuesList_[GetFloatIndex("D1True")];
  _d1RecoPtr_ = &_floatValuesList_[GetFloatIndex("D1Reco")];
  _d2TruePtr_ = &_floatValuesList_[GetFloatIndex("D2True")];
  _d2RecoPtr_ = &_floatValuesList_[GetFloatIndex("D2Reco")];
  _q2TruePtr_ = &_floatValuesList_[GetFloatIndex("q2_true")];
  _q2RecoPtr_ = &_floatValuesList_[GetFloatIndex("q2_reco")];
  _weightMCPtr_ = &_floatValuesList_[GetFloatIndex("weight")];
//  _weightMCPtr_ = &_floatValuesList_[GetFloatIndex("weightMC")];
}

// Core
int AnaEvent::GetFloatIndex(const std::string &floatName_, bool throwIfNotFound_) const  {
  int index = GenericToolbox::findElementIndex(floatName_, *_floatNameListPtr_);
  if(throwIfNotFound_ and index == -1){
    LogFatal << "Could not find float \"" << floatName_ << "\" in _floatNameListPtr_." << std::endl;
    LogFatal << "Available floats are: ";
    GenericToolbox::printVector(*_floatNameListPtr_);
    LogFatal << std::endl;
    throw std::logic_error("Could not find float");
  }
  return index;
}
int AnaEvent::GetIntIndex(const std::string &intName_, bool throwIfNotFound_) const {
  int index = GenericToolbox::findElementIndex(intName_, *_intNameListPtr_);
  if(throwIfNotFound_ and index == -1){
    LogFatal << "Could not find int \"" << intName_ << "\" in _intNameListPtr_." << std::endl;
    LogFatal << "Available ints are: ";
    GenericToolbox::printVector(*_intNameListPtr_);
    LogFatal << std::endl;
    throw std::logic_error("Could not find int");
  }
  return index;
}
Int_t &AnaEvent::GetEventVarInt(const std::string &varName_) {
  return _intValuesList_.at(GetIntIndex(varName_));
}
Float_t &AnaEvent::GetEventVarFloat(const std::string &varName_)  {
  return _floatValuesList_.at(GetFloatIndex(varName_));
}
void AnaEvent::AddEvWght(double val) {
  _eventWeight_ *= val;
}
void AnaEvent::ResetEvWght() {
  _eventWeight_ = *_weightMCPtr_;
}

// Misc
void AnaEvent::Print() {

  LogInfo << "Event ID: " << m_evid << std::endl;

  LogInfo << "List of Int_t: {" << std::endl;
  for(size_t iInt = 0 ; iInt < _intNameListPtr_->size() ; iInt++){
    if(iInt != 0) LogInfo << ", " << std::endl;
    LogInfo << "  \"" << (*_intNameListPtr_)[iInt] << "\": " << _intValuesList_[GetIntIndex((*_intNameListPtr_)[iInt])];
  }
  LogInfo << std::endl << "}" << std::endl;

  LogInfo << "List of Float_t: {" << std::endl;
  for(size_t iFloat = 0 ; iFloat < _floatNameListPtr_->size() ; iFloat++){
    if(iFloat != 0) LogInfo << ", " << std::endl;
    LogInfo << "  \"" << (*_floatNameListPtr_)[iFloat] << "\": " << _floatValuesList_[GetFloatIndex((*_floatNameListPtr_)[iFloat])];
  }
  LogInfo << std::endl << "}" << std::endl;

}


void AnaEvent::ResetIntContainer() {
  _defaultIntNameList_.clear();

  _defaultIntNameList_.emplace_back("beammode");
  _defaultIntNameList_.emplace_back("topology");
  _defaultIntNameList_.emplace_back("cut_branch");

  if( _anaEventType_ == AnaEventType::MC ){
    _defaultIntNameList_.emplace_back("nutype");
    _defaultIntNameList_.emplace_back("reaction");
    _defaultIntNameList_.emplace_back("target");
    _defaultIntNameList_.emplace_back("signal");
  }

  _intNameListPtr_ = &_defaultIntNameList_;
  _intValuesList_.resize(_intNameListPtr_->size());

  HookIntMembers();
}
void AnaEvent::ResetFloatContainer() {
  _defaultFloatNameList_.clear();

  _defaultFloatNameList_.emplace_back("enu_reco");
  _defaultFloatNameList_.emplace_back("D1Reco");
  _defaultFloatNameList_.emplace_back("D2Reco");
  _defaultFloatNameList_.emplace_back("q2_reco");
  _defaultFloatNameList_.emplace_back("weight"); // asimov

  if(_anaEventType_ == AnaEventType::MC){
    _defaultFloatNameList_.emplace_back("enu_true");
    _defaultFloatNameList_.emplace_back("D1True");
    _defaultFloatNameList_.emplace_back("D2True");
    _defaultFloatNameList_.emplace_back("q2_true");
    _defaultFloatNameList_.emplace_back("weightMC");
  }

  _floatNameListPtr_ = &_defaultFloatNameList_;
  _floatValuesList_.resize(_floatNameListPtr_->size());

  HookFloatMembers();
}

// Interfaces
bool AnaEvent::isInBin( const DataBin& dataBin_) {

  for( const auto& varName : dataBin_.getVariableNameList() ){
    if( not dataBin_.isBetweenEdges(varName, this->GetEventVarAsDouble(varName)) ){
      return false;
    }
  }
  return true;

}
std::map<FitParameterSet *, std::vector<Dial *>>* AnaEvent::getDialCachePtr() {
  return &_dialCache_;
}

// Deprecated
Int_t AnaEvent::GetEventVar(const std::string &var) {
  return GetEventVarInt(var);
}








