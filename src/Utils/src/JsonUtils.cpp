//
// Created by Nadrino on 22/05/2021.
//

#include "stdexcept"

#include "yaml-cpp/yaml.h"

#include "GenericToolbox.h"
#include "Logger.h"

#include "JsonUtils.h"
#include "YamlUtils.h"

LoggerInit([]{
  Logger::setUserHeaderStr("[JsonUtils]");
  Logger::setMaxLogLevel(Logger::LogLevel::INFO);
} );

namespace JsonUtils{
  nlohmann::json readConfigFile(const std::string& configFilePath_){

    if( not GenericToolbox::doesPathIsFile(configFilePath_) ){
      LogError << "\"" << configFilePath_ << "\" could not be found." << std::endl;
      throw std::runtime_error("file not found.");
    }

    nlohmann::json output;

    if( GenericToolbox::doesFilePathHasExtension(configFilePath_, "yml")
     or GenericToolbox::doesFilePathHasExtension(configFilePath_,"yaml")
     ){
      auto yaml = YamlUtils::readConfigFile(configFilePath_);
      output = YamlUtils::toJson(yaml);
    }
    else{
      std::fstream fs;
      fs.open(configFilePath_, std::ios::in);

      if( not fs.is_open() ) {
        LogError << "\"" << configFilePath_ << "\": could not read file." << std::endl;
        throw std::runtime_error("file not readable.");
      }


      fs >> output;
    }

    return output;
  }
  nlohmann::json getForwardedConfig(const nlohmann::json& config_, const std::string& keyName_){
    auto outConfig = JsonUtils::fetchValue<nlohmann::json>(config_, keyName_);
    while( outConfig.is_string() ){
      outConfig = JsonUtils::readConfigFile(outConfig.get<std::string>());
    }
    return outConfig;
  }
  void forwardConfig(nlohmann::json& config_, const std::string& className_){
    while( config_.is_string() ){
      LogDebug << "Forwarding " << (className_.empty()? "": className_ + " ") << "config: \"" << config_.get<std::string>() << "\"" << std::endl;
      config_ = JsonUtils::readConfigFile(config_.get<std::string>());
    }
  }
  void unfoldConfig(nlohmann::json& config_){
    for( auto& entry : config_ ){
      if( entry.is_string() and (
             GenericToolbox::doesStringEndsWithSubstring(entry.get<std::string>(), ".yaml", true)
          or GenericToolbox::doesStringEndsWithSubstring(entry.get<std::string>(), ".json", true)
          ) ){
        JsonUtils::forwardConfig(entry);
        JsonUtils::unfoldConfig(config_); // remake the loop on the unfolder config
        break; // don't touch anymore
      }

      if( entry.is_structured() ){
        JsonUtils::unfoldConfig(entry);
      }
    }
  }

  bool doKeyExist(const nlohmann::json& jsonConfig_, const std::string& keyName_){
    return jsonConfig_.find(keyName_) != jsonConfig_.end();
  }
  std::vector<std::string> ls(const nlohmann::json& jsonConfig_){
    std::vector<std::string> out{};
    for( const auto& entry : jsonConfig_.get<nlohmann::json::object_t>() ){ out.emplace_back(entry.first); }
    return out;
  }
  nlohmann::json fetchSubEntry(const nlohmann::json& jsonConfig_, const std::vector<std::string>& keyPath_){
    nlohmann::json output = jsonConfig_;
    for( const auto& key : keyPath_ ){
      output = JsonUtils::fetchValue<nlohmann::json>(output, key);
    }
    return output;
  }

  std::string buildFormula(const nlohmann::json& jsonConfig_, const std::string& keyName_, const std::string& joinStr_){
    std::string out;

    LogThrowIf( not JsonUtils::doKeyExist(jsonConfig_, keyName_), "Could not find key \"" << keyName_ << "\" in " << jsonConfig_ );

    try{ return JsonUtils::fetchValue<std::string>(jsonConfig_, keyName_); }
    catch (...){
      // it's a vector of strings
    }

    std::vector<std::string> conditionsList;
    for( auto& condEntry : JsonUtils::fetchValue<std::vector<nlohmann::json>>(jsonConfig_, keyName_) ){
      if( condEntry.is_string() ){
        conditionsList.emplace_back(condEntry.get<std::string>());
      }
      else{
        LogThrow("Could not recognise condition entry: " << condEntry);
      }
    }

    out += "(";
    out += GenericToolbox::joinVectorString(conditionsList, ") " + joinStr_ + " (");
    out += ")";

    return out;
  }
  std::string buildFormula(const nlohmann::json& jsonConfig_, const std::string& keyName_, const std::string& joinStr_, const std::string& defaultFormula_){
    if( not JsonUtils::doKeyExist(jsonConfig_, keyName_) ) return defaultFormula_;
    else return buildFormula(jsonConfig_, keyName_, joinStr_);
  }

}


