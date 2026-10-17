#pragma once

#include <drogon/orm/Mapper.h>
#include <drogon/orm/Result.h>
#include <json/json.h>

using namespace drogon;
using namespace drogon::orm;

class SensorData {
public:
    int id;
    std::string sensor_id;
    double value;
    std::string unit;
    trantor::Date timestamp;
    trantor::Date received_at;

    static const std::string tableName;
    
    // Методы для сериализации
    Json::Value toJson() const {
        Json::Value json;
        json["id"] = id;
        json["sensor_id"] = sensor_id;
        json["value"] = value;
        json["unit"] = unit;
        json["timestamp"] = timestamp.toCustomedFormattedString("%Y-%m-%d %H:%M:%S");
        json["received_at"] = received_at.toCustomedFormattedString("%Y-%m-%d %H:%M:%S");
        return json;
    }
};

const std::string SensorData::tableName = "sensor_data";