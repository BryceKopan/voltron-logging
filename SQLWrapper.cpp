#include "SQLWrapper.h"

sql::Connection* SQLWrapper::connection;

void SQLWrapper::ConnectToDatabase(std::string url, std::string user, std::string pass, std::string database)
{
    try
    {
        sql::Driver* driver = get_driver_instance();
        connection = driver->connect(url, user, pass);
        connection->setSchema(database);
    }
    catch (sql::SQLException &e)
    {
        SQLExceptionMessage(e);
    }
}

void SQLWrapper::ExecuteQuery(std::string query)
{
    try
    {
        std::auto_ptr<sql::Statement> stmt(connection->createStatement());
        stmt->execute(query);
    }
    catch (sql::SQLException &e)
    {
        SQLExceptionMessage(e);
    }
}

sql::PreparedStatement* SQLWrapper::GetPreparedStatement(std::string query)
{
    try
    {
        sql::PreparedStatement* preparedStatment = connection->prepareStatement(query);
        return preparedStatment;
    }
    catch (sql::SQLException &e)
    {
        SQLExceptionMessage(e);
    }
    
    return NULL;
}

void SQLWrapper::SQLExceptionMessage(sql::SQLException e)
{
    /*
     MySQL Connector/C++ throws three different exceptions:
     
     - sql::MethodNotImplementedException (derived from sql::SQLException)
     - sql::InvalidArgumentException (derived from sql::SQLException)
     - sql::SQLException (derived from std::runtime_error)
     */
    std::cout << "# ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    /* what() (derived from std::runtime_error) fetches error message */
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
}
