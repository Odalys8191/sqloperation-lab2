# 系统使用指南

---

## 系统环境

系统所需环境可从 docker hub 拉取：

```sh
docker pull ftre/datacapsule:v1
```

## consumer依赖
sql代码检查部分依赖 [pybind11](https://github.com/pybind/pybind11) 进行 C++/Python 混合编程，请按如下步骤安装：

```bash
# 克隆 pybind11 源码
git clone https://github.com/pybind/pybind11.git

# 进入目录并编译
cd pybind11
cmake -S . -B build
cmake --build build -j 2  # 使用 2 个线程编译
sudo cmake --install build
```

使用时需要先激活虚拟环境
```bash
source backend/datacapsule-env/bin/activate 
pip install pandas sqlglot
````

## userServer数据库依赖

该模块依赖虚拟机中的 MySQL 数据库，用于存储用户信息。数据库连接信息由配置文件 `backend/include/userServer/userdb.conf` 管理，示例如下：
```ini
host = 127.0.0.1
user = root
password = root
database = user_db
port = 3306
```

系统使用 `user_db` 数据库中的 `users` 表，表结构如下（示例）：
```sql
CREATE TABLE users (
  user_id       VARCHAR(40) PRIMARY KEY,
  username      VARCHAR(50) UNIQUE NOT NULL,
  password_hash TEXT NOT NULL
);
```

启动 Docker 容器时，建议使用 `-network host` 模式以便容器内应用可以访问宿主机（虚拟机）上的 MySQL 服务。

启动容器后，请在容器中安装以下依赖（如未预装）
```bash
apt install -y libmysqlclient-dev
apt install -y libargon2-dev
```

### mysql数据库启动与配置
好的，我给你一个完整的启动 MySQL 并创建 `users` 表的流程示例，包括命令行和 SQL 脚本。

---

#### 1. 启动 MySQL 服务

```bash
sudo service mysql start
```

#### 2. 连接 MySQL

```bash
mysql -h 127.0.0.1 -P 3306 -u root -p
```

输入密码 `root` 。

#### 3. 创建数据库（如果不存在）

```sql
CREATE DATABASE IF NOT EXISTS user_db;
USE user_db;
```

#### 4. 创建 `users` 表

```sql
CREATE TABLE IF NOT EXISTS users (
  user_id       VARCHAR(40) PRIMARY KEY,
  username      VARCHAR(50) UNIQUE NOT NULL,
  password_hash TEXT NOT NULL
);
```

----

## backend

首先编译后端系统：

```sh
cd backend/
mkdir -p cmake/build
cd cmake/build
cmake ../..
make -j4
```

后端服务启动顺序：

```sh
cloud/etcd/fileserver/gateway/ca -> userServer/producer/consumer
```

> ca要早于userServer以及producer、consumer

## frontend

前端启动：

```sh
cd frontend/
npm install #首次启动需要安装必要包
npm run dev
```
---
