#ifndef JSON_WRAPPER_H
#define JSON_WRAPPER_H

//Ubuntu include path
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/writer.h>
//OSXinclude path
//#include <json/json.h>
//#include <json/writer.h>

class JSONWrapper
{
    public:
        static void InitializeWriter();
        static void ReadJSONFile(Json::Value &root, std::string fileName);
        static void WriteJSONToFile(Json::Value &root, std::string fileName);
    
    private:
        static Json::StreamWriter* writer;
};

#endif
