#include "cpr/cpr.h"
#include "nlohmann/json.hpp"

#include <iostream>
#include <sstream>
#include <string_view>
#include <map>
#include <algorithm>
#include <iterator>
#include <iomanip>
#include <string>


using json = nlohmann::json;

class Application {
public:
    Application(const json config) {
        try {
            update(config);
        }
        catch (const std::exception& err) {
            std::clog << err.what();
            update_succeeded_ = false;
            properties_.clear();
            cities_.clear();
        }
        print();
    }

private:
    std::map<const std::string, std::string> properties_;
    std::vector<std::string> cities_;
    bool update_succeeded_ = true;

    void update(const json& config) {
        for (json::const_iterator it = config.cbegin(); it != config.cend(); it++) {
            properties_.insert({ it.key(), it->dump() });
        }
        std::copy(
            config["cities"].cbegin(),
            config["cities"].cend(),
            std::back_inserter(cities_));
        if (cities_.size() == 0) {
            throw std::runtime_error("invalid city count: 0");
        }
        properties_.insert({ "name", *cities_.cbegin() }); // city name

        cpr::Response city_responce = cpr::Get(
            buildUrl(config["api"]["city_api"]),
            cpr::Header{ {{"X-Api-Key", config["api_key"]}} });
        if (city_responce.status_code != 200) {
            throw std::runtime_error("bad access to city api");
        }

        json city_data = json::parse(removeScope(city_responce.text));
        for (auto it = city_data.cbegin(); it != city_data.cend(); it++) {
            properties_.insert({ it.key(), it->dump() });
        }

        cpr::Response forecast_response = cpr::Get(
            buildUrl(config["api"]["forecast_api"]),
            cpr::Header{ {{"X-Api-Key", config["api_key"]}} });
        if (city_responce.status_code != 200) {
            throw std::runtime_error("bad access to city api");
        }

        json forecast_data = json::parse(forecast_response.text);
        for (auto it = forecast_data.cbegin(); it != forecast_data.cend(); it++) {
            properties_.insert({ it.key(), it->dump() });
        }
        forecast_data.push_back({ "name", properties_["name"] });
        save(forecast_data.dump());
    }

    cpr::Url buildUrl(const json& api_info) {
        std::string url = api_info["url"];

        for (auto it = api_info["properties"].cbegin(); it != api_info["properties"].cend(); it++) {
            std::string unquoted_property = removeScope(it->dump());
            if (properties_.find(unquoted_property) == properties_.end()) {
                throw std::runtime_error("undefined property: " + unquoted_property);
            }
            url.append("&" + unquoted_property + "=" + properties_[unquoted_property]);
        }

        if (api_info["hourly_properties"].empty()) {
            return url;
        }

        url.append("&hourly=");
        for (auto it = api_info["hourly_properties"].cbegin(); it != api_info["hourly_properties"].cend(); it++) {
            std::string unquoted_default_property = removeScope(it->dump());
            url.append(unquoted_default_property + ",");
        }

        return url;
    }

    void save(const std::string& data) {
        std::ofstream save_ofs(PATH_TO_SAVE);
        if (!save_ofs) {
            throw std::runtime_error("save file writing failed");
        }
        save_ofs << data;
    }

    void print() {
        std::ifstream save_ofs(PATH_TO_SAVE);
        json save_data = json::parse(save_ofs);
      
        std::vector<std::string> output = output_template_;
        const std::vector<size_t> time = { 
            static_cast<size_t>(EDAY_TIME::MIDNIGHT),
            static_cast<size_t>(EDAY_TIME::MORNING),
            static_cast<size_t>(EDAY_TIME::AFTERNOON),
            static_cast<size_t>(EDAY_TIME::EVENING) };
        for (size_t i = 0; i < time.size(); i++) {
            auto a = save_data["hourly"]["temperature_2m"];
            std::copy_n(
                save_data["hourly"]["temperature_2m"][time[i]].dump().cbegin(),
                save_data["hourly"]["temperature_2m"][time[i]].dump().size(),
                output[WIDTH_OFFSET + TEMPERATURE_STRING].begin() + FIELD_OFFSET + i * FRAME_OFFSET);
           std::copy_n(
                save_data["hourly"]["windspeed_10m"][time[i]].dump().cbegin(),
                save_data["hourly"]["windspeed_10m"][time[i]].dump().size(),
                output[WIDTH_OFFSET + WIND_SPEED_STRING].begin() + FIELD_OFFSET + i * FRAME_OFFSET);
            std::copy_n(
                save_data["hourly"]["relativehumidity_2m"][time[i]].dump().cbegin(),
                save_data["hourly"]["relativehumidity_2m"][time[i]].dump().size(),
                output[WIDTH_OFFSET + HUMIDITY_STRING].begin() + FIELD_OFFSET + i * FRAME_OFFSET);
            std::copy_n(
                save_data["hourly"]["time"][time[i]].dump().cbegin(),
                save_data["hourly"]["time"][time[i]].dump().size(),
                output[WIDTH_OFFSET + TIME_STRING].begin() + FIELD_OFFSET + i * FRAME_OFFSET);
        }

        for (const auto& str : output) {
            std::cout << str;
        }
    }


    std::string removeScope(const std::string& str) {
        return str.substr(1, str.size() - 2);
    }

    enum class EDAY_TIME : size_t { 
        MIDNIGHT = 0, 
        MORNING = 7,
        AFTERNOON = 14, 
        EVENING = 21,
        DAY = 24
    };

    enum EOUTPUT_FORMAT : size_t {
        CONSOLE_LENGTH = 125,
        CONSOLE_WIDTH = 30,
        FIELD_OFFSET = 14,
        FRAME_OFFSET = 31,
        WIDTH_OFFSET = 3,
        TEMPERATURE_STRING = 0,
        WIND_SPEED_STRING = 1,
        HUMIDITY_STRING = 2,
        TIME_STRING = 3
    };

    const std::vector<std::string> output_template_ = {
"┌──────────────────────────────-─────────────────────────────────────────────────────────────────────────────────────────── ┐\n",
"│             MIDNIGHT                       MORNING                        AFTERNOON                      EVENING          │\n",
"├───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤\n",
"│Temperature: ________________  Temperature: ________________  Temperature: ________________  Temperature: ________________ │\n",
"│Wind speed:  ________________  Wind speed:  ________________  Wind speed:  ________________  Wind speed:  ________________ |\n",
"│Humidity:    ________________  Humidity:    ________________  Humidity:    ________________  Humidity:    ________________ │\n",
"│Time:        ________________  Time:        ________________  Time:        ________________  Time:        ________________ │\n",
"└──────────────────────────────┴──────────────────────────────┴──────────────────────────────┴──────────────────────────────┘\n"
    };
    
};

int main(int argc, char* argv[]) {
    std::ifstream config_ifs(PATH_TO_CONFIG);
    json config = json::parse(config_ifs);
    Application app(config);

    return 0;
}
