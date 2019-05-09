#include "SQLThread.h"

#include <fcntl.h>
#include <fstream>
#include "JSONWrapper.h"
#include "SQLWrapper.h"
#include "Debug.h"

#define SQL_HOST "localhost"
#define SQL_USER "root"
#define SQL_PASSWORD ""
#define SQL_DB "Voltron"

sem_t* SQLThread::mutex;

void SQLThread::ListenerThread(std::string drivesFileName)
{
    mutex = sem_open("LogDrive", O_CREAT, 0644, 1);
    
    while(1000 == 1000)
    {
        Json::Value root;
        bool fileExists;
        
        sem_wait(mutex);
        try
        {
            JSONWrapper::ReadJSONFile(root, drivesFileName);
            fileExists = true;
        } catch(Json::RuntimeError) {
            fileExists = false;
        }
        sem_post(mutex);
        
        if(fileExists)
        {
            int driveIndex = 0;
            for (const Json::Value& drive : root["Drives"])
            {
                if(drive["IsCompleted"].asBool() == true)
                {
                    UploadDrive(driveIndex, drivesFileName);
                }
                driveIndex++;
            }
        }
        
        //Sleep for 1 min
        usleep(60000000);
    }
}

void SQLThread::UploadDrive(int driveIndex, std::string drivesFileName)
{
    Debug::writeDebugMessage("[Logging] Connecting to SQL server\n");
    
    if(!SQLWrapper::ConnectToDatabase(SQL_HOST, SQL_USER, SQL_PASSWORD, SQL_DB))
    {
        Debug::writeDebugMessage("[Logging] Cannot connect to SQL server\n");
        return;
    }
    
    Debug::writeDebugMessage("[Logging] Uploading drive to SQL server\n");
    
    Json::Value root;
    
    sem_wait(mutex);
    JSONWrapper::ReadJSONFile(root, drivesFileName);
    sem_post(mutex);
    
    sql::PreparedStatement* driveQuery = SQLWrapper::GetPreparedStatement("INSERT INTO drive(StartUnixTimestamp, EndUnixTimestamp) VALUES (?, ?)");
    driveQuery->setInt(1, root["Drives"][driveIndex]["StartUnixTimestamp"].asInt());
    driveQuery->setInt(2, root["Drives"][driveIndex]["EndUnixTimestamp"].asInt());
    driveQuery->execute();
    
    sql::PreparedStatement* captureQuery = SQLWrapper::GetPreparedStatement("INSERT INTO capture(CaptureTypeID, StartUnixTimestamp, EndUnixTimestamp, CaptureBlob) VALUES (?, ?, ?, ?)");
    
    for (const Json::Value& capture : root["Drives"][driveIndex]["Captures"])
    {
        //From SQL table capturetype
        int captureType;
        if(capture["Type"].asString() == "Battery")
        {
            captureType = 1;
        }
        else if(capture["Type"].asString() == "CAN")
        {
            captureType = 2;
        }
        else if(capture["Type"].asString() == "LIDAR")
        {
            captureType = 3;
        }
        else if(capture["Type"].asString() == "ZED")
        {
            captureType = 5;
        }
        
        captureQuery->setInt(1, captureType);
        captureQuery->setInt(2, capture["StartUnixTimestamp"].asInt());
        captureQuery->setInt(3, capture["EndUnixTimestamp"].asInt());
        
        std::ifstream captureFile(capture["File"].asString(), std::ifstream::binary);
        
        captureQuery->setBlob(4, &captureFile);
        captureQuery->execute();
        
        //remove(capture["File"].asCString());
    }
    
    Json::Value removed;
    root["Drives"].removeIndex(driveIndex, &removed);
    
    sem_wait(mutex);
    JSONWrapper::WriteJSONToFile(root, drivesFileName);
    sem_post(mutex);
    
    Debug::writeDebugMessage("[Logging] Drive uploaded to SQL server\n");
}
