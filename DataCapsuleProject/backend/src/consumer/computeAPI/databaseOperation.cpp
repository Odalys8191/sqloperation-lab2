#include "../../../include/consumer/computeAPI/databaseOperation.h"
#include <sstream>

MySQLDatabase::~MySQLDatabase()
{
    try
    {
        if (!dcDbName_.empty())
        {
            std::cout << "Dropping temporary database: " << dcDbName_ << std::endl;
            session_.sql("DROP DATABASE IF EXISTS " + dcDbName_).execute();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error while dropping database: " << e.what() << std::endl;
    }
}

void MySQLDatabase::loadSQL(const std::string &sqlContent, const std::string &dc_id)
{
    dcDbName_ = generateTempDbName(dc_id);
    session_.sql("CREATE DATABASE " + dcDbName_).execute();
    session_.sql("USE " + dcDbName_).execute();

    std::istringstream sqlStream(sqlContent);
    std::string sqlStatement;
    while (std::getline(sqlStream, sqlStatement, ';'))
    {
        if (!sqlStatement.empty() && sqlStatement.find_first_not_of(" \t\n\r") != std::string::npos)
        {
            session_.sql(sqlStatement).execute();
        }
    }
}

mysqlx::RowResult MySQLDatabase::executeQuery(const std::string &query)
{
    if (dcDbName_.empty())
    {
        throw std::runtime_error("Temporary database is not initialized.");
    }
    return session_.sql(query).execute();
}

bool MySQLDatabase::dcSQLOperation(const std::string &dc_id, const std::string &dbContent, const std::string &query, std::string &result)
{
    try
    {
        // 首先，根据dbContent创建临时数据库
        MySQLDatabase tempDb;
        tempDb.loadSQL(dbContent, dc_id);

        // 然后执行查询
        auto queryResult = tempDb.executeQuery(query);

        // 将查询结果写入result字符串
        std::ostringstream resultStream;
        for (mysqlx::Row row : queryResult)
        {
            for (size_t i = 0; i < row.colCount(); i++)
            {
                resultStream << row[i] << " ";
            }
            resultStream << std::endl;
        }
        result += resultStream.str();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

int testDatabaseOperation(const std::string &host, const int &port, const std::string &user, const std::string &password, const std::string &dc_id)
{
    try
    {
        const std::string dc_id = "8a759bd10021acd3";

        // 模拟 SQL 文件内容
        std::string sqlContent = R"(
            CREATE TABLE test_table (id INT PRIMARY KEY, name VARCHAR(50));
            INSERT INTO test_table VALUES (1, 'Alice'), (2, 'Bob');
            INSERT INTO test_table VALUES (3, 'Charlie'), (4, 'David');
            INSERT INTO test_table VALUES (5, 'Eve'), (6, 'Frank');
            INSERT INTO test_table VALUES (7, 'Grace'), (8, 'Helen');
            INSERT INTO test_table VALUES (9, 'Ivy'), (10, 'Jack');
            INSERT INTO test_table VALUES (11, 'Kevin'), (12, 'Lily');
        )";

        // 创建并操作临时数据库
        MySQLDatabase tempDb(host, port, user, password);
        tempDb.loadSQL(sqlContent, dc_id);

        std::cout << "------------ Select Query Test ------------" << std::endl;
        // 查询数据
        auto result = tempDb.executeQuery("SELECT * FROM test_table");
        for (mysqlx::Row row : result)
        {
            std::cout << "ID: " << row[0] << ", Name: " << row[1] << std::endl;
        }
        std::cout << "------------ End of Select Query Test ------------" << std::endl;

        std::string query;
        while (true)
        {
            std::cout << "Enter a query (or 'exit' to quit): ";
            std::getline(std::cin, query);

            if (query == "exit")
            {
                break;
            }

            try
            {
                result = tempDb.executeQuery(query);
                for (mysqlx::Row row : result)
                {
                    for (size_t i = 0; i < row.colCount(); i++)
                    {
                        std::cout << row[i] << " ";
                    }
                    std::cout << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }

        // 临时数据库会在析构时自动删除
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
