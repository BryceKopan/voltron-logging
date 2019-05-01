#include "JSONWrapper.h"

std::unique_ptr<Json::StreamWriter> writer;

void WriteJSONValueToFile(Json::Value value, std::ofstream* file)
{
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "   ";
    
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer -> write(value, file);
}
