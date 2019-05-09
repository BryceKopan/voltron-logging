#ifndef SQLWRAPPER_H
#define SQLWRAPPER_H

#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

class SQLWrapper
{
    public:
        static bool ConnectToDatabase(std::string url, std::string user, std::string pass, std::string database);
        static void ExecuteQuery(std::string query);
        static sql::PreparedStatement* GetPreparedStatement(std::string query);
    
    
    private:
        static sql::Connection* connection;
    
        static void SQLExceptionMessage(sql::SQLException e);
};
#endif
