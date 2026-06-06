# Khazanah Ilmu — Library Management System

**KILMS** (خزانة علم) is a console-based C++ application for managing a historical and multilingual book library, connected to a MySQL 8 database running in Docker.

## Quick Start

### 1. Start the database
```bash
docker compose up -d
```

### 2. Build the application

```bash
mkdir build && cd build
cmake ..
make
```

### 3. Run
```bash
./kilms
```

## Requirements

| Dependency | Version |
|---|---|
| Docker + Docker Compose | Any recent version |
| MySQL Connector/C++ | 9.x (`mysqlcppconnx`) — X DevAPI |
| C++ compiler | C++17 or later |
| CMake | 3.16+ |

**Install MySQL Connector/C++ on macOS (Homebrew):**
```bash
brew install mysql-connector-c++
```

## Database Management

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

## Connection Details

| Field | Value |
|---|---|
| Host | 127.0.0.1 |
| Port (C++ app) | 33060 (X Protocol) |
| Port (TablePlus / phpMyAdmin) | 3306 (Classic) |
| Database | khazanah_ilmu |
| User | kilms_user |
| Password | kilms_pass |
