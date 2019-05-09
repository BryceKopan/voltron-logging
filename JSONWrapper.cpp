#include "JSONWrapper.h"

#include <fstream>
#include "Debug.h"

Json::StreamWriter* JSONWrapper::writer;

void JSONWrapper::InitializeWriter()
{
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "   ";
    
    writer = builder.newStreamWriter();
}

void JSONWrapper::ReadJSONFile(Json::Value &root, std::string fileName)
{
    std::ifstream driveFile(fileName, std::ifstream::binary);
    
    driveFile >> root;
    
    driveFile.close();
}

void JSONWrapper::WriteJSONToFile(Json::Value &root, std::string fileName)
{
    std::ofstream driveFile(fileName, std::ifstream::binary);
    
    writer->write(root, &driveFile);

    driveFile.close();
}
