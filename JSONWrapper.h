#ifndef JSONWRAPPER_H
#define JSONWRAPPER_H

#include <json/json.h>
#include <json/writer.h>

class JSONWrapper
{
    public:
        static void WriteJSONValueToFile(Json::Value value, std::ofstream& file);
};

#endif
