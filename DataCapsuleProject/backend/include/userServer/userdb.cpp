#include "userdb.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

Database::Database() : conn(nullptr) {}

Database::~Database() {
    std::lock_guard<std::mutex> lock(mutex_);
    disconnectUnlocked();
}

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

MYSQL* Database::getConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    return conn;
}

static inline std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

bool Database::loadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    config.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                key = trim(key);
                value = trim(value);
                if (!key.empty()) {
                    config[key] = value;
                }
            }
        }
    }
    return !config.empty();
}

void Database::disconnectUnlocked() {
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
}

bool Database::isConnectionLostError(unsigned int err) {
    switch (err) {
    case 2002: // CR_CONNECTION_ERROR
    case 2003: // CR_CONN_HOST_ERROR
    case 2006: // CR_SERVER_GONE_ERROR
    case 2013: // CR_SERVER_LOST
        return true;
    default:
        return false;
    }
}

bool Database::connectUnlocked() {
    if (config.empty()) {
        std::cerr << "[Database] 配置为空，无法连接数据库" << std::endl;
        return false;
    }

    disconnectUnlocked();

    conn = mysql_init(nullptr);
    if (!conn) {
        std::cerr << "[Database] mysql_init() failed" << std::endl;
        return false;
    }

    unsigned int reconnect_timeout = 10;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &reconnect_timeout);

    const char* host = config.count("host") ? config["host"].c_str() : "127.0.0.1";
    const char* user = config.count("user") ? config["user"].c_str() : "root";
    const char* password = config.count("password") ? config["password"].c_str() : "";
    const char* database = config.count("database") ? config["database"].c_str() : "";
    unsigned int port = 3306;
    if (config.count("port")) {
        try {
            port = static_cast<unsigned int>(std::stoul(config["port"]));
        } catch (...) {
            std::cerr << "[Database] 无效端口配置: " << config["port"] << std::endl;
            disconnectUnlocked();
            return false;
        }
    }

    if (!mysql_real_connect(conn, host, user, password, database, port, nullptr, 0)) {
        std::cerr << "[Database] mysql_real_connect() failed: " << mysql_error(conn) << std::endl;
        disconnectUnlocked();
        return false;
    }

    std::cout << "[Database] 已连接 MySQL " << host << ":" << port << "/" << database << std::endl;
    return true;
}

bool Database::reconnectUnlocked() {
    std::cout << "[Database] 检测到连接断开，正在重连..." << std::endl;
    if (!config_path_.empty() && config.empty()) {
        if (!loadConfig(config_path_)) {
            std::cerr << "[Database] 重连时无法加载配置: " << config_path_ << std::endl;
            return false;
        }
    }
    if (!connectUnlocked()) {
        std::cerr << "[Database] 重连失败" << std::endl;
        return false;
    }
    std::cout << "[Database] 重连成功" << std::endl;
    return true;
}

bool Database::ensureConnectedUnlocked() {
    if (conn && mysql_ping(conn) == 0) {
        return true;
    }
    if (conn) {
        std::cerr << "[Database] mysql_ping 失败: "
                  << mysql_error(conn) << " (errno=" << mysql_errno(conn) << ")" << std::endl;
    }
    return reconnectUnlocked();
}

bool Database::connect(const std::string& config_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_path_ = config_path;
    if (!loadConfig(config_path)) {
        std::cerr << "[Database] 无法加载配置文件: " << config_path << std::endl;
        return false;
    }

    std::cout << "[Database] host: " << config["host"] << std::endl;
    std::cout << "[Database] user: " << config["user"] << std::endl;
    std::cout << "[Database] database: " << config["database"] << std::endl;
    std::cout << "[Database] port: " << config["port"] << std::endl;

    return connectUnlocked();
}

// =================== INSERT USER ===================
bool Database::insertUserUnlocked(const std::string& user_id, const std::string& username, const std::string& hash) {
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        std::cerr << "[Database] mysql_stmt_init() failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    const char* query = "INSERT INTO users (user_id, username, password_hash) VALUES (?, ?, ?)";
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        std::cerr << "[Database] mysql_stmt_prepare() failed: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND bind[3]{};
    unsigned long len1 = user_id.size();
    unsigned long len2 = username.size();
    unsigned long len3 = hash.size();

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)user_id.c_str();
    bind[0].buffer_length = len1;
    bind[0].length = &len1;

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void*)username.c_str();
    bind[1].buffer_length = len2;
    bind[1].length = &len2;

    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (void*)hash.c_str();
    bind[2].buffer_length = len3;
    bind[2].length = &len3;

    if (mysql_stmt_bind_param(stmt, bind)) {
        std::cerr << "[Database] mysql_stmt_bind_param() failed: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt)) {
        std::cerr << "[Database] 插入失败: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);
    return true;
}

bool Database::insertUser(const std::string& user_id, const std::string& username, const std::string& hash) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!ensureConnectedUnlocked()) {
            return false;
        }
        if (insertUserUnlocked(user_id, username, hash)) {
            return true;
        }
        const unsigned int err = mysql_errno(conn);
        if (attempt == 0 && isConnectionLostError(err)) {
            disconnectUnlocked();
            continue;
        }
        return false;
    }
    return false;
}

// =================== QUERY PASSWORD HASH ===================
bool Database::getPasswordHashUnlocked(const std::string& identifier, std::string& out_hash, bool by_user_id) {
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        std::cerr << "[Database] mysql_stmt_init() failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    const char* query = by_user_id ?
        "SELECT password_hash FROM users WHERE user_id = ?" :
        "SELECT password_hash FROM users WHERE username = ?";

    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        std::cerr << "[Database] mysql_stmt_prepare() failed: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND bind_param{};
    unsigned long id_len = identifier.size();
    bind_param.buffer_type = MYSQL_TYPE_STRING;
    bind_param.buffer = (void*)identifier.c_str();
    bind_param.buffer_length = id_len;
    bind_param.length = &id_len;

    if (mysql_stmt_bind_param(stmt, &bind_param)) {
        std::cerr << "[Database] mysql_stmt_bind_param() failed: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    char hash_buffer[512];
    unsigned long hash_len = 0;
    bool is_null = 0;
    MYSQL_BIND bind_result{};
    bind_result.buffer_type = MYSQL_TYPE_STRING;
    bind_result.buffer = hash_buffer;
    bind_result.buffer_length = sizeof(hash_buffer);
    bind_result.length = &hash_len;
    bind_result.is_null = &is_null;

    if (mysql_stmt_bind_result(stmt, &bind_result)) {
        std::cerr << "[Database] mysql_stmt_bind_result() failed: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt)) {
        std::cerr << "[Database] mysql_stmt_execute() failed: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_store_result(stmt)) {
        std::cerr << "[Database] mysql_stmt_store_result() failed: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_fetch(stmt) == 0 && !is_null) {
        out_hash.assign(hash_buffer, hash_len);
    } else {
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);
    return true;
}

bool Database::getPasswordHash(const std::string& identifier, std::string& out_hash, bool by_user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!ensureConnectedUnlocked()) {
            return false;
        }
        if (getPasswordHashUnlocked(identifier, out_hash, by_user_id)) {
            return true;
        }
        const unsigned int err = mysql_errno(conn);
        if (attempt == 0 && isConnectionLostError(err)) {
            disconnectUnlocked();
            continue;
        }
        return false;
    }
    return false;
}
