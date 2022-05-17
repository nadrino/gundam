//
// Created by Nadrino on 22/07/2021.
//

#include "DataSetLoader.h"

#include "DialSet.h"
#include "GlobalVariables.h"
#include "GraphDial.h"
#include "SplineDial.h"
#include "JsonUtils.h"

#include "GenericToolbox.h"
#include "GenericToolbox.Root.h"
#include "GenericToolbox.VariablesMonitor.h"
#include "Logger.h"

#include <TTreeFormulaManager.h>
#include "TTree.h"

LoggerInit([](){
  Logger::setUserHeaderStr("[DataSetLoader]");
})

DataSetLoader::DataSetLoader() { this->reset(); }
DataSetLoader::~DataSetLoader(){ this->reset(); }

void DataSetLoader::reset() {
  _isInitialized_ = false;
  _config_.clear();
  _isEnabled_ = false;
  _name_ = "";
}

void DataSetLoader::setConfig(const nlohmann::json &config_) {
  _config_ = config_;
  JsonUtils::forwardConfig(_config_, __CLASS_NAME__);
}
void DataSetLoader::setDataSetIndex(int dataSetIndex) {
  _dataSetIndex_ = dataSetIndex;
}

void DataSetLoader::initialize() {
  LogWarning << "Initializing data set loader..." << std::endl;
  LogThrowIf(_config_.empty(), "Config not set.");

  _name_ = JsonUtils::fetchValue<std::string>(_config_, "name");
  _selectedDataEntry_ = JsonUtils::fetchValue<std::string>(_config_, "selectedDataEntry", "Asimov");
  _isEnabled_ = JsonUtils::fetchValue(_config_, "isEnabled", true);
  if( not _isEnabled_ ){ LogWarning << "\"" << _name_ << "\" is disabled." << std::endl; return; }

  _mcDispenser_.setOwner(this);
  _mcDispenser_.setConfig(JsonUtils::fetchValue<nlohmann::json>(_config_, "mc"));
  _mcDispenser_.getConfigParameters().name = "asimov";
  _mcDispenser_.getConfigParameters().useMcContainer = true;
  _mcDispenser_.initialize();

  // Always loaded by default
  _dataDispenserDict_["Asimov"] = _mcDispenser_;

  for( auto& dataEntry : JsonUtils::fetchValue(_config_, "data", nlohmann::json()) ){
    std::string name = JsonUtils::fetchValue(dataEntry, "name", "data");
    LogThrowIf( GenericToolbox::doesKeyIsInMap(name, _dataDispenserDict_),
                "\"" << name << "\" already taken, please use another name." )

    _dataDispenserDict_[name] = DataDispenser();
    if( JsonUtils::fetchValue(dataEntry, "fromMc", false) ){ _dataDispenserDict_[name] = _mcDispenser_; }
    _dataDispenserDict_[name].getConfigParameters().name = name;
    _dataDispenserDict_[name].setOwner(this);
    _dataDispenserDict_[name].setConfig(dataEntry);
    _dataDispenserDict_[name].initialize();
  }

  if( not GenericToolbox::doesKeyIsInMap(_selectedDataEntry_, _dataDispenserDict_) ){
    LogThrow("selectedDataEntry could not be find in available data: "
    << GenericToolbox::iterableToString(_dataDispenserDict_, [](const std::pair<std::string, DataDispenser>& elm){ return elm.first; })
    << std::endl);
  }

  LogInfo << "Initializing dataset: \"" << _name_ << "\"" << std::endl;
  _isInitialized_ = true;
}

bool DataSetLoader::isEnabled() const {
  return _isEnabled_;
}
const std::string &DataSetLoader::getName() const {
  return _name_;
}
int DataSetLoader::getDataSetIndex() const {
  return _dataSetIndex_;
}

DataDispenser &DataSetLoader::getMcDispenser() {
  return _mcDispenser_;
}
DataDispenser &DataSetLoader::getSelectedDataDispenser(){
  return _dataDispenserDict_[_selectedDataEntry_];
}
std::map<std::string, DataDispenser> &DataSetLoader::getDataDispenserDict() {
  return _dataDispenserDict_;
}

const std::string &DataSetLoader::getSelectedDataEntry() const {
  return _selectedDataEntry_;
}
