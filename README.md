# Employee Database

A client-server employee database system written in C.

## Features

- TCP-based client-server architecture
- Employee management (add, list employees)
- Binary database file format with network byte order
- Multi-client support using poll()
- Protocol-based communication

## Building

### Windows (PowerShell)
```powershell
.\build.ps1
```

### Windows (Command Prompt)
```cmd
build.bat
```

### Linux/Unix (if make is available)
```bash
make
```

## Usage

### Starting the Server
```bash
# Create new database
./bin/dbserver -n -f mynewdb.db -p 8080

# Use existing database
./bin/dbserver -f mynewdb.db -p 8080
```

### Using the Client
```bash
# List employees
./bin/dbcli -h localhost -p 8080 -l

# Add employee (format: name,address,hours)
./bin/dbcli -h localhost -p 8080 -a "John Doe,123 Main St,40"

# Both operations
./bin/dbcli -h localhost -p 8080 -a "Jane Smith,456 Oak Ave,35" -l
```

## Protocol

The system uses a custom binary protocol with the following message types:
- HELLO_REQ/RESP: Protocol handshake
- EMPLOYEE_ADD_REQ/RESP: Add new employee
- EMPLOYEE_LIST_REQ/RESP: List all employees

## Database Format

The database file uses a binary format with:
- Header: magic number, version, count, file size
- Employee records: name, address, hours (network byte order)

## Requirements

- C compiler (GCC recommended)
- POSIX-compatible system (Linux, macOS, Windows with MinGW)
