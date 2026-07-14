#pragma once
#include <mysql-cppconn/mysqlx/xdevapi.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <chrono>
#include <thread>

class MySQLDatabase
{
public:
    MySQLDatabase(const std::string &host = "0.0.0.0", int port = 33060, const std::string &user = "root", const std::string &password = "root")
        : session_(host, port, user, password) {}
    ~MySQLDatabase();

    void loadSQL(const std::string &sqlContent, const std::string &dc_id);
    mysqlx::RowResult executeQuery(const std::string &query);
    bool dcSQLOperation(const std::string &dc_id, const std::string &dbContent, const std::string &query, std::string &result);

private:
    mysqlx::Session session_;
    std::string dcDbName_;
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;

    // std::string generateTempDbName(const std::string &dc_id) { return "db_" + dc_id; }
    // 删除id中的'-'
    std::string generateTempDbName(const std::string &dc_id)
    {
        std::string temp = dc_id;
        temp.erase(std::remove(temp.begin(), temp.end(), '-'), temp.end());
        return "db_" + temp;
    }
};

int testDatabaseOperation(const std::string &host, const int &port, const std::string &user, const std::string &password, const std::string &dc_id);