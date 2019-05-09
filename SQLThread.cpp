#include "SQLThread.h"

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
    JSONWrapper::ReadJSONFile(root, drivesFileName);
    
    sql::PreparedStatement* driveQuery = SQLWrapper::GetPreparedStatement("INSERT INTO drive(StartUnixTimestamp, EndUnixTimestamp) VALUES (?, ?)");
    driveQuery->setInt(1, root["Drives"][driveIndex]["StartUnixTimestamp"].asInt());
    driveQuery->setInt(2, root["Drives"][driveIndex]["EndUnixTimestamp"].asInt());
    driveQuery->execute();
    
    sql::PreparedStatement* captureQuery = SQLWrapper::GetPreparedStatement("INSERT INTO capture(CaptureTypeID, StartUnixTimestamp, EndUnixTimestamp) VALUES (?, ?, ?)");
    
    for (const Json::Value& capture : root["Drives"][driveIndex]["Captures"])
    {
        if(capture["Type"].asString() == "Battery")
        {
            captureQuery->setInt(1, 1);
            captureQuery->setInt(2, capture["StartUnixTimestamp"].asInt());
            captureQuery->setInt(3, capture["EndUnixTimestamp"].asInt());
            captureQuery->execute();
        }
    }
    
    Debug::writeDebugMessage("[Logging] Drive uploaded to SQL server\n");
}
