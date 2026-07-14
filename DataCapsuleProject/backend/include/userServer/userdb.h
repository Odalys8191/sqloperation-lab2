#pragma once
#include <mysql/mysql.h>
#include <mutex>
#include <string>
#include <map>

class Database {
public:
    static Database& getInstance();

    bool connect(const std::string& config_path);
    MYSQL* getConnection();

    bool insertUser(const std::string& user_id, const std::string& username, const std::string& hash);
    bool getPasswordHash(const std::string& identifier, std::string& out_hash, bool by_user_id = false);

private:
    Database();
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool loadConfig(const std::string& path);
    void disconnectUnlocked();
    bool connectUnlocked();
    bool ensureConnectedUnlocked();
    bool reconnectUnlocked();
    static bool isConnectionLostError(unsigned int err);

    bool insertUserUnlocked(const std::string& user_id, const std::string& username, const std::string& hash);
    bool getPasswordHashUnlocked(const std::string& identifier, std::string& out_hash, bool by_user_id);

    MYSQL* conn;
    std::map<std::string, std::string> config;
    std::string config_path_;
    std::mutex mutex_;
};
