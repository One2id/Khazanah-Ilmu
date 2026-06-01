# Khazanah Ilmu — Library Management System

**KILMS** (خزانة علم) is a console-based C++ application for managing a historical and multilingual book library, connected to a MySQL 8 database running in Docker.

## Quick Start

### 1. Start the database
```bash
docker compose up -d
```

### 2. Build the application

**Linux / macOS (CMake):**
```bash
mkdir build && cd build
cmake ..
make
./kilms
```

**Windows (g++ direct):**
```bash
g++ -std=c++17 src/*.cpp src/**/*.cpp -o kilms -lmysqlcppconn -I/path/to/connector/include
```

### 3. Run
```bash
./kilms
```

## Requirements

| Dependency | Version |
|---|---|
| Docker + Docker Compose | Any recent version |
| MySQL Connector/C++ | 8.x (`mysqlcppconn`) |
| C++ compiler | C++17 or later |
| CMake | 3.16+ |

**Install MySQL Connector/C++ on Ubuntu/Debian:**
```bash
sudo apt install libmysqlcppconn-dev
```

**Install on macOS (Homebrew):**
```bash
brew install mysql-connector-c++
```

## Database management

| Task | Command |
|---|---|
| Start database | `docker compose up -d` |
| Stop database | `docker compose down` |
| Wipe all data and restart | `docker compose down -v && docker compose up -d` |

## Modules

- **Members** — library member registration and status
- **Books** — historical book catalogue with language and era metadata
- **Authors** — historical author profiles
- **Languages** — supported book languages and scripts
- **Loans** — borrowing records with due-date tracking
- **Fines** — overdue fine records (amount entered manually by staff)

## Connection details

| Field | Value |
|---|---|
| Host | 127.0.0.1 |
| Port | 3306 |
| Database | khazanah_ilmu |
| User | kilms_user |
| Password | kilms_pass |
