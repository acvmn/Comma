// Made by Nenboard

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <time.h>
#include <iostream>
#include <threads.h>
#include <wininet.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include <conio.h>
#include <string>
#include <vector>
#include <random>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <map>
#include <chrono>
#include <cstdlib>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/rand.h>

#include <json.hpp>

#include <sqlite3.h>

#include <mutex>

#define INDEX 0
#define UNDEFINED 1
#define PROFILE 2
#define ROOM 3
#define UPLOAD 4
#define API 5
#define ADMIN 6
#define HTTPS 7
#define HTTP 8
#define MEMORY 9
#define LOG 10
#define KEY 11
#define CRT 12
#define DATABASE 13
#define USERS 14
#define DOMAIN 15
#define DEVELOPER 16
#define POSTS 17
#define ACCOUTNS 18
#define ROOMS 19
#define LIMIT 20

#define FILE_SIZE (int)(1024 * 1024 * atoi(config[MEMORY]))

char config[256][256] = { "index/index.html",
                          "undefined/undefined.html",
                          "profile",
                          "room",
                          "upload",
                          "api",
                          "admi",
                          "443",
                          "80",
                          "16",
                          "log.txt",
                          "server.key",
                          "server.crt",
                          "database.db",
                          "users.db",
                          "127.0.0.1",
                          "admin",
                          "24",
                          "1",
                          "16",
                          "250" };
int rooms = 0;
bool loop;

SSL_CTX* ssl_ctx = nullptr;
sqlite3* db = nullptr;
sqlite3* us = nullptr;

std::vector<std::vector<std::shared_ptr<SSL>>> clients;
std::vector<std::string> creators;

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string version;
    std::string body;
    std::string raw_headers;
    std::map<std::string, std::string> headers;
    std::string filename;
    std::string login;
    std::string password;
    std::string comment;
    std::string file_content;
    int content_length = 0;
};

struct HttpResponse
{
    std::string headers;
    std::string body;
};

bool ws_send(SSL* ssl, const std::string& message)
{
    std::vector<unsigned char> frame;
    frame.push_back(0x81);
    size_t len = message.size();
    if (len <= 125)
    {
        frame.push_back(static_cast<unsigned char>(len));
    }
    else if (len <= 65535)
    {
        frame.push_back(126);
        frame.push_back(static_cast<unsigned char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<unsigned char>(len & 0xFF));
    }
    else
    {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i)
        {
            frame.push_back(static_cast<unsigned char>((len >> (i * 8)) & 0xFF));
        }
    }
    frame.insert(frame.end(), message.begin(), message.end());
    int total = 0;
    while (total < (int)frame.size())
    {
        int sent = SSL_write(ssl, frame.data() + total, (int)frame.size() - total);
        if (sent <= 0)
        {
            return false;
        }
        total += sent;
    }
    return true;
}

bool ws_recv(SSL* ssl, std::string& out)
{
    unsigned char header[2];
    if (SSL_read(ssl, header, 2) != 2)
    {
        return false;
    }
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payload_len = header[1] & 0x7F;
    if (payload_len == 126) {
        unsigned char ext[2];
        if (SSL_read(ssl, ext, 2) != 2)
        {
            return false;
        }
        payload_len = (ext[0] << 8) | ext[1];
    }
    else if (payload_len == 127) {
        unsigned char ext[8];
        if (SSL_read(ssl, ext, 8) != 8)
        {
            return false;
        }
        payload_len = 0;
        for (int i = 0; i < 8; ++i)
        {
            payload_len = (payload_len << 8) | ext[i];
        }
    }
    unsigned char mask_key[4] = { 0 };
    if (masked)
    {
        if (SSL_read(ssl, mask_key, 4) != 4)
        {
            return false;
        }
    }
    std::vector<unsigned char> buf(payload_len);
    int total = 0;
    while (total < (int)payload_len)
    {
        int n = SSL_read(ssl, buf.data() + total, (int)payload_len - total);
        if (n <= 0)
        {
            return false;
        }
        total += n;
    }
    if (masked)
    {
        for (size_t i = 0; i < payload_len; ++i)
        {
            buf[i] ^= mask_key[i % 4];
        }
    }
    out.assign(buf.begin(), buf.end());
    return true;
}

void log_time()
{
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_s(&tm_now, &time_t_now);

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d ", tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
    std::cout << buffer;

    FILE* log = nullptr;
    if (fopen_s(&log, config[LOG], "a") == 0 and log)
    {
        fprintf(log, "%s", buffer);
        fclose(log);
    }
}

void log_message(const std::string& message)
{
    log_time();
    std::cout << message << "\n\n";

    FILE* log = nullptr;
    if (fopen_s(&log, config[LOG], "a") == 0 and log)
    {
        fprintf(log, "%s\n\n", message.c_str());
        fclose(log);
    }
}

std::map<std::string, std::string> parse_headers(const std::string& raw_headers)
{
    std::map<std::string, std::string> headers;
    std::istringstream stream(raw_headers);
    std::string line;

    while (std::getline(stream, line) and line != "\r" and !line.empty())
    {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        if (line.empty())
        {
            continue;
        }

        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos)
        {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);

            value.erase(0, value.find_first_not_of(" \t"));

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            headers[key] = value;
        }
    }

    return headers;
}

void parse_multipart_data(HttpRequest& request)
{
    std::string boundary;
    auto ct_it = request.headers.find("content-type");
    if (ct_it != request.headers.end())
    {
        std::string ct = ct_it->second;
        size_t boundary_pos = ct.find("boundary=");
        if (boundary_pos != std::string::npos)
        {
            boundary = ct.substr(boundary_pos + 9);
            boundary.erase(0, boundary.find_first_not_of(" \t\""));
            boundary.erase(boundary.find_last_not_of(" \t\"") + 1);
        }
    }

    if (boundary.empty())
    {
        return;
    }

    std::string boundary_marker = "--" + boundary;

    size_t pos = 0;
    while (pos < request.body.length())
    {
        size_t part_start = request.body.find(boundary_marker, pos);
        if (part_start == std::string::npos)
        {
            break;
        }

        part_start += boundary_marker.length();

        if (part_start + 2 <= request.body.length() and request.body.substr(part_start, 2) == "--")
        {
            break;
        }

        if (part_start + 2 <= request.body.length() and request.body.substr(part_start, 2) == "\r\n")
        {
            part_start += 2;
        }

        size_t headers_end = request.body.find("\r\n\r\n", part_start);
        if (headers_end == std::string::npos)
        {
            break;
        }

        std::string part_headers = request.body.substr(part_start, headers_end - part_start);
        size_t content_start = headers_end + 4;

        size_t content_end = request.body.find(boundary_marker, content_start);
        if (content_end == std::string::npos)
        {
            content_end = request.body.length();
        }

        if (content_end >= 2 and request.body.substr(content_end - 2, 2) == "\r\n")
        {
            content_end -= 2;
        }

        std::string part_content = request.body.substr(content_start, content_end - content_start);

        std::string part_name;
        std::string part_filename;

        size_t name_pos = part_headers.find("name=\"");
        if (name_pos != std::string::npos)
        {
            name_pos += 6;
            size_t name_end = part_headers.find("\"", name_pos);
            if (name_end != std::string::npos)
            {
                part_name = part_headers.substr(name_pos, name_end - name_pos);
            }
        }

        size_t filename_pos = part_headers.find("filename=\"");
        if (filename_pos != std::string::npos)
        {
            filename_pos += 10;
            size_t filename_end = part_headers.find("\"", filename_pos);
            if (filename_end != std::string::npos)
            {
                part_filename = part_headers.substr(filename_pos, filename_end - filename_pos);
            }
        }

        if (!part_filename.empty())
        {
            request.filename = part_filename;
            request.file_content = part_content;
        }
        else if (part_name == "login")
        {
            request.login = part_content;
        }
        else if (part_name == "password")
        {
            request.password = part_content;
        }
        else if (part_name == "comment")
        {
            request.comment = part_content;
        }

        pos = content_end + 2;
    }
}

bool check(std::string string)
{
    int step = 0;
    while (step < string.length())
    {
        if (string.c_str()[step] < 33 or string.c_str()[step] > 126 or string.c_str()[step] == '/' or string.c_str()[step] == '\\')
        {
            return false;
        }
        step++;
    }
    return true;
}

HttpRequest parse_http_request(const std::string& raw_request)
{
    HttpRequest request;

    size_t first_line_end = raw_request.find("\r\n");
    if (first_line_end == std::string::npos)
    {
        return request;
    }

    std::string first_line = raw_request.substr(0, first_line_end);

    std::istringstream first_line_stream(first_line);
    first_line_stream >> request.method >> request.path >> request.version;

    if (!request.path.empty() and request.path[0] == '/')
    {
        char decoded_url[1024] = {};
        DWORD len = 1024;
        InternetCanonicalizeUrlA(request.path.c_str(), decoded_url, &len, ICU_DECODE | ICU_NO_ENCODE);
        request.path = decoded_url;

        if (!request.path.empty() and request.path[0] == '/')
        {
            request.path = request.path.substr(1);
        }
    }

    size_t headers_start = first_line_end + 2;
    size_t headers_end = raw_request.find("\r\n\r\n", headers_start);

    if (headers_end != std::string::npos)
    {
        request.raw_headers = raw_request.substr(headers_start, headers_end - headers_start);
        request.headers = parse_headers(request.raw_headers);

        auto login_it = request.headers.find("login");
        if ((login_it != request.headers.end()))
        {
            request.login = login_it->second;
        }

        auto password_it = request.headers.find("password");
        if (password_it != request.headers.end())
        {
            request.password = password_it->second;
        }

        auto comment_it = request.headers.find("comment");
        if (comment_it != request.headers.end())
        {
            request.comment = comment_it->second;
        }

        auto cl_it = request.headers.find("content-length");
        if (cl_it != request.headers.end())
        {
            try
            {
                request.content_length = std::stoi(cl_it->second);
            }
            catch (...)
            {
                request.content_length = 0;
            }
        }

        request.body = raw_request.substr(headers_end + 4, request.content_length);

        if (request.headers.find("content-type") != request.headers.end() and request.headers["content-type"].find("multipart/form-data") != std::string::npos)
        {
            parse_multipart_data(request);
        }
        else if (!request.body.empty())
        {
            while (!request.body.empty() and (request.body.back() == '\r' or request.body.back() == '\n'))
            {
                request.body.pop_back();
            }
        }
    }

    return request;
}

std::string get_client_ip(SOCKET client)
{
    sockaddr_in addr;
    int addr_len = sizeof(addr);
    getpeername(client, (sockaddr*)&addr, &addr_len);
    return std::string(inet_ntoa(addr.sin_addr));
}

bool check_passwords(const std::string& login)
{
    try
    {
        sqlite3_stmt* stmt;
        const char* sql = "SELECT COUNT(*) FROM passwords WHERE login = ?;";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
        bool exists = false;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            exists = sqlite3_column_int(stmt, 0) > 0;
        }
        sqlite3_finalize(stmt);

        stmt;
        sql = "SELECT COUNT(*) FROM users WHERE user_id = ?;";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            exists = sqlite3_column_int(stmt, 0) > 0;
        }
        sqlite3_finalize(stmt);
        return exists;
    }
    catch (...)
    {

    }
    return false;
}

bool check_accounts(const std::string& ip)
{
    try
    {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "SELECT accounts FROM accounts WHERE ip = ?;", -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        
        if (count < atoi(config[ACCOUTNS]))
        {
            return true;
        }
    }
    catch (...)
    {

    }
    return false;
}

std::string get_user_id(const std::string& ip, const std::string& login, const std::string& password)
{
    try
    {
        if (!login.empty() and !password.empty())
        {
            sqlite3_stmt* stmt;
            const char* sql = "SELECT password FROM passwords WHERE login = ?;";
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) == SQLITE_ROW)
                {
                    const unsigned char* stored = sqlite3_column_text(stmt, 0);
                    if (stored and password == std::string(reinterpret_cast<const char*>(stored)))
                    {
                        sqlite3_finalize(stmt);
                        return login;
                    }
                }
                sqlite3_finalize(stmt);
            }
        }

        sqlite3_stmt* stmt;
        const char* sql = "SELECT user_id FROM users WHERE ip = ?;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            return "";
        }
        sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
        std::string id;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char* uid = sqlite3_column_text(stmt, 0);
            if (uid) id = reinterpret_cast<const char*>(uid);
        }
        sqlite3_finalize(stmt);
        return id;
    }
    catch (...)
    {

    }
    return "";
}

std::string generate_random_id()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(10000000, 99999999);
    return std::to_string(dis(gen));
}

bool check_like(const std::string& like_id, const std::string& ip, const std::string& login, const std::string& password)
{
    try
    {
        std::string user_id = get_user_id(ip, login, password);
        if (user_id.empty())
        {
            return false;
        }

        sqlite3_stmt* stmt;
        const char* sql = "SELECT 1 FROM likes WHERE post_id = ? AND user_id = ?;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            return false;
        }
        sqlite3_bind_int(stmt, 1, std::stoi(like_id));
        sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);
        bool found = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
        return found;
    }
    catch (...)
    {

    }
    return false;
}

void add_like(const std::string& like_id, const std::string& ip, const std::string& login, const std::string& password)
{
    if (check_like(like_id, ip, login, password))
    {
        return;
    }
    std::string user_id = get_user_id(ip, login, password);
    if (user_id.empty())
    {
        return;
    }

    try
    {
        sqlite3_stmt* stmt;
        const char* sql = "INSERT OR IGNORE INTO likes (post_id, user_id) VALUES (?, ?);";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            return;
        }
        sqlite3_bind_int(stmt, 1, std::stoi(like_id));
        sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    catch (...)
    {

    }
}

void remove_like(const std::string& like_id, const std::string& ip, const std::string& login, const std::string& password)
{
    std::string user_id = get_user_id(ip, login, password);
    if (user_id.empty())
    {
        return;
    }

    try
    {
        sqlite3_stmt* stmt;
        const char* sql = "DELETE FROM likes WHERE post_id = ? AND user_id = ?;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            return;
        }
        sqlite3_bind_int(stmt, 1, std::stoi(like_id));
        sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    catch (...)
    {

    }
}

int count_likes(const std::string& like_id)
{
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM likes WHERE post_id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return 0;
    }
    sqlite3_bind_int(stmt, 1, std::stoi(like_id));
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

std::string get_comments_count(const std::string& like_id)
{
    try
    {
        sqlite3_stmt* stmt;
        const char* sql = "SELECT COUNT(*) FROM comments WHERE post_id = ?;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            return "0";
        }
        sqlite3_bind_int(stmt, 1, std::stoi(like_id));
        std::string count = "0";
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = std::to_string(sqlite3_column_int(stmt, 0));
        }
        sqlite3_finalize(stmt);
        return count;
    }
    catch (...)
    {

    }
    return "0";
}

int get_views_count(const std::string& like_id)
{
    try
    {
        sqlite3_stmt* stmt;
        const char* sql = "SELECT count FROM views WHERE post_id = ?;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            return 0;
        }
        sqlite3_bind_int(stmt, 1, std::stoi(like_id));
        int views = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) views = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return views;
    }
    catch (...)
    {

    }
    return 0;
}

void increment_views(const std::string& like_id)
{
    try
    {
        sqlite3_stmt* stmt;
        const char* sql_ins = "INSERT OR IGNORE INTO views (post_id, count) VALUES (?, 0);";
        sqlite3_prepare_v2(db, sql_ins, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, std::stoi(like_id));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        const char* sql_upd = "UPDATE views SET count = count + 1 WHERE post_id = ?;";
        sqlite3_prepare_v2(db, sql_upd, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, std::stoi(like_id));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    catch (...)
    {

    }
}

std::string get_file_time(const std::string& filepath)
{
    std::wstring wfilepath(filepath.begin(), filepath.end());
    HANDLE hFile = CreateFile(wfilepath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return "";
    }

    FILETIME ftCreate;
    SYSTEMTIME stUTC, stLocal;
    char time_str[64] = {};

    if (GetFileTime(hFile, &ftCreate, NULL, NULL))
    {
        FileTimeToSystemTime(&ftCreate, &stUTC);
        SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
        sprintf_s(time_str, "%02d.%02d.%d %02d:%02d",
            stLocal.wMonth, stLocal.wDay, stLocal.wYear,
            stLocal.wHour, stLocal.wMinute);
    }
    CloseHandle(hFile);

    return std::string(time_str);
}

std::string read_file(const std::string& filepath, bool binary = false)
{
    const char* mode = binary ? "rb" : "r";
    FILE* file = nullptr;
    if (fopen_s(&file, filepath.c_str(), mode) != 0 or !file)
    {
        return "";
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return "";
    }

    long size = ftell(file);
    if (size <= 0 or fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return "";
    }

    std::string result;
    result.resize(size);
    size_t bytes_read = fread(&result[0], 1, size, file);
    fclose(file);

    if (bytes_read != (size_t)size)
    {
        result.resize(bytes_read);
    }

    return result;
}

bool write_file(const std::string& filepath, const std::string& content, bool binary = false)
{
    size_t last_slash = filepath.find_last_of("\\/");
    if (last_slash != std::string::npos)
    {
        std::string dir = filepath.substr(0, last_slash);
        bool result = _mkdir(dir.c_str());
    }

    const char* mode = binary ? "wb" : "w";
    FILE* file = nullptr;
    if (fopen_s(&file, filepath.c_str(), mode) != 0 or !file)
    {
        return false;
    }

    size_t written = fwrite(content.c_str(), 1, content.length(), file);
    fclose(file);

    return written == content.length();
}

bool create_directory(const std::string& path)
{
    std::filesystem::create_directories(path);
    return true;
}

HttpResponse handle_registration(const HttpRequest& request, const std::string& ip)
{
    HttpResponse response;

    std::string login = request.login;
    std::transform(login.begin(), login.end(), login.begin(), [](unsigned char c) { return std::tolower(c); });

    std::string password = request.password;
    std::transform(password.begin(), password.end(), password.begin(), [](unsigned char c) { return std::tolower(c); });

    try
    {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO accounts (ip, accounts) VALUES (?, ?);", -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, 0);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (check_passwords(request.login))
        {
            nlohmann::json j;
            j["Status"] = "Busy";
            response.headers += "Content-Type: application/json";
            response.body = j.dump();
        }
        else if (!check_accounts(ip))
        {
            nlohmann::json j;
            j["Status"] = "Many";
            response.headers += "Content-Type: application/json";
            response.body = j.dump();
        }
        else if (check(request.login) and request.login.length() <= 8 and request.password.length() <= 8 and !request.login.empty() and !request.password.empty() and login == request.login and password == request.password)
        {
            try
            {
                sqlite3_stmt* stmt;
                sqlite3_prepare_v2(db, "INSERT INTO passwords (login, password) VALUES (?, ?);", -1, &stmt, nullptr);
                sqlite3_bind_text(stmt, 1, request.login.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, request.password.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);

                std::string path_upload = std::string(config[UPLOAD]) + "\\" + request.login;
                create_directory(path_upload);

                const char* sql_upd = "UPDATE accounts SET accounts = accounts + 1 WHERE ip = ?;";
                sqlite3_prepare_v2(db, sql_upd, -1, &stmt, nullptr);
                sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
            catch (...)
            {

            }

            response.body = "";
        }
        else
        {
            nlohmann::json j;
            j["Status"] = "Wrong";
            response.body = j.dump();
        }
    }
    catch (...)
    {

    }

    response.headers = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(response.body.length()) + "\r\nContent-Type: application/json\r\n\r\n";
    return response;
}

HttpResponse handle_authorization(const HttpRequest& request, const std::string& ip)
{
    HttpResponse response;

    try
    {
        std::string registered = get_user_id(ip, request.login, request.password);

        if (!registered.empty() and registered == request.login)
        {
            response.body = "";
        }
        else
        {
            nlohmann::json j;
            j["Status"] = "Wrong";
            response.body = j.dump();
        }
    }
    catch (...)
    {

    }

    response.headers = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(response.body.length()) + "\r\nContent-Type: application/json\r\n\r\n";
    return response;
}

HttpResponse handle_start(const HttpRequest& request, const std::string& ip)
{
    HttpResponse response;

    try
    {
        std::string id = get_user_id(ip, request.login, request.password);
        bool registered = !id.empty();

        if (!registered)
        {
            id = generate_random_id();
            std::string path = std::string(config[UPLOAD]) + "\\" + id;
            create_directory(path);

            log_message("IP " + ip + " is not registered. Registering as " + id + "...");

            try
            {
                sqlite3_stmt* stmt;
                sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO users (ip, user_id) VALUES (?, ?);", -1, &stmt, nullptr);
                sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
            catch (...)
            {

            }
        }
        else
        {
            log_message("IP " + ip + " is registered as " + id);
        }

        nlohmann::json j;
        j["ID"] = id;
        j["Posts"] = nlohmann::json::array();

        try
        {
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db, "SELECT post_id, path FROM posts ORDER BY post_id DESC;", -1, &stmt, nullptr);
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                const unsigned char* path = sqlite3_column_text(stmt, 1);
                if (path)
                {
                    int pid = sqlite3_column_int(stmt, 0);
                    std::string like_id = std::to_string(pid);

                    int views = get_views_count(like_id);
                    std::string liked = check_like(like_id, ip, request.login, request.password) ? "true" : "false";
                    int likes_count = count_likes(like_id);
                    std::string comments = get_comments_count(like_id);
                    std::string file_time = get_file_time(reinterpret_cast<const char*>(path));
                    
                    j["Posts"].push_back({
                        {"ID", pid},
                        {"Liked", liked.c_str()},
                        {"Likes", likes_count},
                        {"Views", views},
                        {"Comments", comments.c_str()},
                        {"Time", file_time.c_str()},
                        {"Path", reinterpret_cast<const char*>(path)}
                    });

                    increment_views(std::to_string(pid));
                }
            }
            sqlite3_finalize(stmt);
        }
        catch (...)
        {

        }

        response.body = j.dump();
    }
    catch (...)
    {

    }

    response.headers = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(response.body.length()) + "\r\nContent-Type: application/json\r\n\r\n";

    return response;
}

HttpResponse handle_comments(const std::string& like_id)
{
    HttpResponse response;

    try
    {
        nlohmann::json j;
        j["Comments"] = nlohmann::json::array();

        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "SELECT path FROM comments WHERE post_id = ? ORDER BY comment_id DESC;", -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, std::stoi(like_id));
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char* p = sqlite3_column_text(stmt, 0);
            if (p)
            {
                j["Comments"].push_back(reinterpret_cast<const char*>(p));
            }
        }
        sqlite3_finalize(stmt);

        response.body = j.dump();
    }
    catch (...)
    {

    }

    response.headers = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(response.body.length()) + "\r\nContent-Type: application/json\r\n\r\n";
    return response;
}

HttpResponse handle_like_action(const HttpRequest& request, const std::string& ip, const std::string& like_id, bool is_like)
{
    HttpResponse response;
    response.body = "";

    if (is_like) add_like(like_id, ip, request.login, request.password);
    else remove_like(like_id, ip, request.login, request.password);

    response.headers = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    return response;
}

int insertIntoDynamicTable(sqlite3* db, const std::string& id, long long post_id, const std::string& path)
{
    std::string sql = "INSERT INTO \"" + id + "\" (post_id, path) VALUES (?, ?);";

    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    result = sqlite3_bind_int(stmt, 1, post_id);
    result = sqlite3_bind_text(stmt, 2, path.c_str(), -1, SQLITE_STATIC);
    result = sqlite3_step(stmt);
    result = sqlite3_finalize(stmt);

    return SQLITE_OK;
}

HttpResponse handle_new_post(const HttpRequest& request, const std::string& ip)
{
    log_message("IP " + ip + " created a new post");
    HttpResponse response;
    response.body = "";

    try
    {
        std::string extension;
        size_t dot_pos = request.filename.find_last_of('.');
        if (dot_pos != std::string::npos)
        {
            extension = request.filename.substr(dot_pos + 1);
        }

        if (extension == "txt" and request.file_content.length() > (1024 * 16))
        {
            response.headers = "HTTP/1.1 413 Content Too Large\r\nContent-Length: 0\r\n\r\n";
            return response;
        }

        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO times (ip, time) VALUES (?, ?);", -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, std::time(nullptr) - 60 * 60 * 24 / atoi(config[POSTS]));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        stmt;
        sqlite3_prepare_v2(db, "SELECT time FROM times WHERE ip = ?;", -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        int time = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        if (time > (std::time(nullptr) - 60 * 60 * 24 / atoi(config[POSTS])))
        {
            nlohmann::json j;
            j["Time"] = time + 60 * 60 * 24 / atoi(config[POSTS]) - std::time(nullptr);
            response.body = j.dump();
            response.headers = "HTTP/1.1 429 Too Many Requests\r\nContent-Length: ";
            response.headers += response.body.length();
            response.headers += "\r\n\r\n";
            return response;
        }

        if (request.filename.empty() or request.file_content.empty())
        {
            response.headers = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
            return response;
        }

        std::string id = get_user_id(ip, request.login, request.password);
        if (id.empty())
        {
            id = generate_random_id();
            std::string path = std::string(config[UPLOAD]) + "\\" + id;
            create_directory(path);

            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO users (ip, user_id) VALUES (?, ?);", -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        std::string create_table = "CREATE TABLE IF NOT EXISTS '" + id + "' (post_id INTEGER, path TEXT);";
        sqlite3_exec(us, create_table.c_str(), nullptr, nullptr, nullptr);

        std::string folder_posts = std::string(config[UPLOAD]) + "\\" + id;
        create_directory(folder_posts);

        std::string path = folder_posts + "\\" + request.filename;

        write_file(path, request.file_content, true);

        const std::vector<std::string> allowed_extensions = { "png", "jpg", "jpeg", "gif", "ico", "mp4", "txt" };
        if (std::find(allowed_extensions.begin(), allowed_extensions.end(), extension) == allowed_extensions.end())
        {
            response.headers = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
            return response;
        }

        stmt;
        sqlite3_prepare_v2(db, "INSERT INTO posts (path) VALUES (?);", -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        long long post_id = sqlite3_last_insert_rowid(db);
        insertIntoDynamicTable(us, id, post_id, path);
        sqlite3_finalize(stmt);

        int new_id = (int)sqlite3_last_insert_rowid(db);

        sqlite3_prepare_v2(db, "INSERT INTO views (post_id, count) VALUES (?, 0);", -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, new_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        stmt;
        sqlite3_prepare_v2(db, "UPDATE times SET time = ? WHERE ip = ?;", -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, std::time(nullptr));
        sqlite3_bind_text(stmt, 2, ip.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        stmt;
        sqlite3_prepare_v2(db, "DELETE FROM posts WHERE post_id IN (SELECT post_id FROM posts ORDER BY post_id ASC LIMIT 1) AND (SELECT COUNT(*) FROM posts) > ?;", -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, atoi(config[LIMIT]));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        stmt;
        sqlite3_prepare_v2(us, "DELETE FROM ? WHERE post_id IN (SELECT post_id FROM posts ORDER BY post_id ASC LIMIT 1) AND (SELECT COUNT(*) FROM posts) > ?;", -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, atoi(config[LIMIT]));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        response.headers = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        return response;
    }
    catch (...)
    {

    }

    response.headers = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    return response;
}

HttpResponse handle_new_comment(const HttpRequest& request, const std::string& ip)
{
    log_message("IP " + ip + " created a new comment");
    HttpResponse response;
    response.body = "";

    try
    {
        std::string extension;
        size_t dot_pos = request.filename.find_last_of('.');
        if (dot_pos != std::string::npos)
        {
            extension = request.filename.substr(dot_pos + 1);
        }

        if (extension == "txt" and request.file_content.length() > (1024 * 16))
        {
            response.headers = "HTTP/1.1 413 Content Too Large\r\nContent-Length: 0\r\n\r\n";
            return response;
        }

        if (request.comment.empty() or request.filename.empty() or request.file_content.empty())
        {
            response.headers = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
            return response;
        }

        std::string id = get_user_id(ip, request.login, request.password);
        if (id.empty())
        {
            id = generate_random_id();
            std::string path = std::string(config[UPLOAD]) + "\\" + id;
            create_directory(path);

            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO users (ip, user_id) VALUES (?, ?);", -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        std::string folder_posts = std::string(config[UPLOAD]) + "\\" + id;
        create_directory(folder_posts);

        std::string path = folder_posts + "\\" + request.filename;

        if (!write_file(path, request.file_content, true))
        {
            response.headers = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
            return response;
        }

        const std::vector<std::string> allowed_extensions = { "png", "jpg", "jpeg", "gif", "ico", "mp4", "txt" };
        if (std::find(allowed_extensions.begin(), allowed_extensions.end(), extension) == allowed_extensions.end())
        {
            response.headers = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
            return response;
        }

        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "INSERT INTO comments (post_id, path) VALUES (?, ?);", -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, std::stoi(request.comment));
        sqlite3_bind_text(stmt, 2, path.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        response.headers = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        return response;
    }
    catch (...)
    {

    }

    response.headers = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    return response;
}

HttpResponse handle_static_file(const std::string& path)
{
    HttpResponse response;
    std::string response_headers;

    std::string full_path = path;
    std::transform(full_path.begin(), full_path.end(), full_path.begin(), [](unsigned char c) { return std::tolower(c); });

    if (!full_path.empty() and full_path[0] == '/')
    {
        full_path = full_path.substr(1);
    }

    size_t dot_pos = full_path.find_last_of('.');
    bool is_binary = true;
    std::string ext;
    if (dot_pos != std::string::npos)
    {
        ext = full_path.substr(dot_pos + 1);
    }

    if (full_path.empty())
    {
        full_path = config[INDEX];
    }

    const std::vector<std::string> exceptions_types = { "html", "css", "js", "txt" };
    if (std::find(exceptions_types.begin(), exceptions_types.end(), ext) != exceptions_types.end() or ext == "")
    {
        is_binary = false;
    }

    const std::vector<std::string> exceptions_files = { "config.txt", "comma.exe" };
    if (std::find(exceptions_files.begin(), exceptions_files.end(), full_path) != exceptions_files.end())
    {
        ext = "html";
        is_binary = false;
        response.body = read_file(config[UNDEFINED], is_binary);
        response_headers = "HTTP/1.1 404 Not Found\r\n";
    }
    else
    {
        response.body = read_file(full_path, is_binary);
        response_headers = "HTTP/1.1 200 OK\r\n";
        if (response.body == "")
        {
            is_binary = false;
            response.body = read_file(config[UNDEFINED], is_binary);
            response_headers = "HTTP/1.1 404 Not Found\r\n";
        }
    }

    if (is_binary and !response.body.empty())
    {
        std::string content_type = "multipart/form-data";

        if (ext == "mp4")
        {
            content_type = "video/mp4";
        }
        else if (ext == "mp3")
        {
            content_type = "audio/mp3";
        }
        else if (ext == "png")
        {
            content_type = "image/png";
        }
        else if (ext == "jpg" or ext == "jpeg")
        {
            content_type = "image/jpeg";
        }
        else if (ext == "gif")
        {
            content_type = "image/gif";
        }
        else if (ext == "ico")
        {
            content_type = "image/x-icon";
        }

        response_headers += "Content-Type: " + content_type + "\r\n";
    }
    else if (!is_binary and !response.body.empty())
    {
        std::string content_type = "text/plain";

        if (ext == "html" or ext == "")
        {
            content_type = "text/html";
        }
        else if (ext == "css")
        {
            content_type = "text/css";
        }
        else if (ext == "js")
        {
            content_type = "text/javascript";
        }

        response_headers += "Content-Type: " + content_type + "\r\n";
    }

    response_headers += "Content-Length: " + std::to_string(response.body.length()) + "\r\n";
    response_headers += "\r\n";

    response.headers = response_headers;

    return response;
}

HttpResponse handle_profile(const HttpRequest& request, const std::string& ip, const std::string& like_id)
{
    HttpResponse response;

    try
    {
        log_message("IP " + ip + " loaded profile " + like_id);

        nlohmann::json j;
        j["ID"] = like_id;
        j["Developer"] = (get_user_id(ip, request.login, request.password) == config[DEVELOPER]) ? "true" : "false";
        j["Posts"] = nlohmann::json::array();

        try
        {
            std::string command = "SELECT post_id, path FROM \"" + like_id + "\" ORDER BY post_id DESC;";

            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(us, command.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, like_id.c_str(), -1, SQLITE_STATIC);
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                const unsigned char* path = sqlite3_column_text(stmt, 1);
                if (path)
                {
                    int pid = sqlite3_column_int(stmt, 0);
                    std::string like_id = std::to_string(pid);

                    int views = get_views_count(like_id);
                    std::string liked = check_like(like_id, ip, request.login, request.password) ? "true" : "false";
                    int likes_count = count_likes(like_id);
                    std::string comments = get_comments_count(like_id);
                    std::string file_time = get_file_time(reinterpret_cast<const char*>(path));

                    j["Posts"].push_back({
                        {"ID", pid},
                        {"Liked", liked.c_str()},
                        {"Likes", likes_count},
                        {"Views", views},
                        {"Comments", comments.c_str()},
                        {"Time", file_time.c_str()},
                        {"Path", reinterpret_cast<const char*>(path)}
                        });
                }
            }
            sqlite3_finalize(stmt);
        }
        catch (...)
        {

        }

        response.body = j.dump();
    }
    catch (...)
    {

    }

    response.headers = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(response.body.length()) + "\r\nContent-Type: application/json\r\n\r\n";

    return response;
}

HttpResponse handle_message(const std::string& message, const std::string& ip, const std::string& login, const std::string& password, const std::string& path)
{
    try
    {
        HttpResponse response;
        std::string command;
        std::string like_id;

        nlohmann::json j = nlohmann::json::parse(message);

        if (j.contains("Command"))
        {
            command = j["Command"];
        }
        else if (j.contains("Comments"))
        {
            command = "Comments";
            like_id = j["Comments"];
        }
        else if (j.contains("Like"))
        {
            command = "Like";
            like_id = j["Like"];
        }
        else if (j.contains("Unlike"))
        {
            command = "Unlike";
            like_id = j["Unlike"];
        }
        else if (j.contains("Profile"))
        {
            command = "Profile";
            like_id = j["Profile"];
        }
        else if (j.contains("Connect"))
        {
            command = "Connect";
            like_id = j["Connect"];
        }
        else if (j.contains("Delete"))
        {
            command = "Delete";
            like_id = j["Delete"];
        }

        if (command == "Start..." and path == config[API])
        {
            HttpRequest request;
            request.login = login;
            request.password = password;
            return handle_start(request, ip);
        }
        else if (command == "Create" and path == config[API])
        {
            int step = 0;
            while (step < atoi(config[ROOMS]) and clients[step].size() > 0)
            {
                step++;
            }

            if (step < atoi(config[ROOMS]) and rooms <= atoi(config[ROOMS]))
            {
                rooms++;
                nlohmann::json j;
                j["Developer"] = (get_user_id(ip, login, password) == (std::string)config[DEVELOPER]) ? "true" : "false";
                j["Room"] = step + 1;
                response.body = j.dump();
                response.headers = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(response.body.length()) + "\r\nContent-Type: application/json\r\n\r\n";
                return response;
            }
            else
            {
                nlohmann::json j;
                j["Room"] = 0;
                response.body = j.dump();
                response.headers = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(response.body.length()) + "\r\nContent-Type: application/json\r\n\r\n";
                return response;
            }
        }
        else if ((command == "Reg" or command == "Auth") and path == config[API])
        {
            HttpRequest request;
            request.login = login;
            request.password = password;

            if (command == "Reg")
            {
                log_message("IP " + ip + " tried to register as " + login);
                return handle_registration(request, ip);
            }
            else
            {
                log_message("IP " + ip + " tried to log in as " + login);
                return handle_authorization(request, ip);
            }
        }
        else if ((command == "Post" or command == "Comments" or command == "Like" or command == "Unlike") and path == config[API])
        {
            HttpRequest request;
            request.login = login;
            request.password = password;

            if (command == "Comments")
            {
                return handle_comments(like_id);
            }
            else if (command == "Like")
            {
                return handle_like_action(request, ip, like_id, true);
            }
            else if (command == "Unlike")
            {
                return handle_like_action(request, ip, like_id, false);
            }
        }
        else if (command == "Profile" and path == config[API])
        {
            HttpRequest request;
            request.login = login;
            request.password = password;
            return handle_profile(request, ip, like_id);
        }
        else if (command == "Connect" and path == config[API])
        {
            int room = atoi(like_id.c_str());

            if (room > 0 and room <= atoi(config[ROOMS]))
            {
                if (clients[room - 1].size() == 0)
                {
                    nlohmann::json j;
                    j["Status"] = "Not found";
                    response.body = j.dump();
                    response.headers = "HTTP/1.1 404 Not Found\r\nContent-Length: " + std::to_string(response.body.length()) + "\r\nContent-Type: application/json\r\n\r\n";
                    return response;
                }

                nlohmann::json j;
                j["Developer"] = (get_user_id(ip, login, password) == (std::string)config[DEVELOPER]) ? "true" : "false";
                j["Status"] = "Found";
                response.body = j.dump();
                response.headers = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(response.body.length()) + "\r\nContent-Type: application/json\r\n\r\n";
                return response;
            }

            nlohmann::json j;
            j["Status"] = "Not found";
            response.body = j.dump();
            response.headers = "HTTP/1.1 404 Not Found\r\nContent-Length: " + std::to_string(response.body.length()) + "\r\nContent-Type: application/json\r\n\r\n";
            return response;
        }
        else if (command == "Delete" and (get_user_id(ip, login, password) == config[DEVELOPER] or get_user_id(ip, login, password) == like_id) and path == config[API])
        {
            std::string sql = "DELETE FROM posts WHERE path LIKE '%\\";
            sql += like_id.c_str();
            sql += "\\%';";
            sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);

            sql = "DELETE FROM comments WHERE path LIKE '%\\";
            sql += like_id.c_str();
            sql += "\\%';";
            sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);

            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db, "DELETE FROM likes WHERE user_id = ?;", -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, like_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            std::filesystem::remove_all("upload\\" + like_id);

            sql = "DROP TABLE IF EXISTS '";
            sql += like_id.c_str();
            sql += "';";
            sqlite3_exec(us, sql.c_str(), nullptr, nullptr, nullptr);

            stmt;
            sqlite3_prepare_v2(db, "DELETE FROM passwords WHERE login = ?;", -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, like_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            stmt;
            sqlite3_prepare_v2(db, "DELETE FROM users WHERE user_id = ?;", -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, like_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            if (get_user_id(ip, login, password) != config[DEVELOPER])
            {
                const char* sql_upd = "UPDATE accounts SET accounts = accounts - 1 WHERE ip = ?;";
                sqlite3_prepare_v2(db, sql_upd, -1, &stmt, nullptr);
                sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }

            nlohmann::json j;
            j["Status"] = "Deleted";
            response.body = j.dump();
            response.headers = "HTTP/1.1 200 OK\r\nContent-Length: 20\r\nContent-Type: application/json\r\n\r\n";
            return response;
        }
        else if (command == "Stop..." and get_user_id(ip, login, password) == config[DEVELOPER] and path == config[ADMIN])
        {
            nlohmann::json j;
            j["Status"] = "Stop";
            response.body = j.dump();
            response.headers = "HTTP/1.1 200 OK\r\nContent-Length: 17\r\nContent-Type: application/json\r\n\r\n";
            return response;
        }
        else
        {
            response.headers = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
            return response;
        }
    }
    catch (...)
    {

    }
    
    return handle_static_file(message);
}

bool send_response(SSL* ssl, const HttpResponse& response)
{
    std::string full_response;

    if (response.headers.find("\r\n\r\n") == response.headers.length() - 4)
    {
        full_response = response.headers + response.body;
    }
    else if (response.headers.find("Content-Length: ") != std::string::npos)
    {
        size_t cl_pos = response.headers.find("Content-Length: ") + 16;
        size_t cl_end = response.headers.find("\r\n", cl_pos);
        if (cl_end != std::string::npos)
        {
            std::string cl_value = response.headers.substr(cl_pos, cl_end - cl_pos);
            if (cl_value.empty() or cl_value.find_first_not_of(" \t") == std::string::npos)
            {
                std::string correct_header = response.headers.substr(0, cl_pos) + std::to_string(response.body.length()) + response.headers.substr(cl_pos);
                full_response = correct_header + response.body;
            }
            else
            {
                full_response = response.headers + response.body;
            }
        }
        else
        {
            full_response = response.headers + response.body;
        }
    }
    else
    {
        full_response = response.headers + std::to_string(response.body.length()) + "\r\n\r\n" + response.body;
    }

    int total_sent = 0;
    int to_send = (int)full_response.length();
    const char* data = full_response.c_str();

    while (total_sent < to_send)
    {
        int sent = SSL_write(ssl, data + total_sent, to_send - total_sent);
        if (sent <= 0)
        {
            return false;
        }
        total_sent += sent;
    }

    return true;
}

bool moved_permanently(SSL* ssl, HttpResponse response, SOCKET client)
{
    if (!send_response(ssl, response))
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        closesocket(client);
        return 0;
    }
}

void close_ws(SSL* ssl, SOCKET client)
{
    ws_send(ssl, "{\"type\":\"status\",\"status\":\"error\"}");
    SSL_shutdown(ssl);
    SSL_free(ssl);
    closesocket(client);
}

struct ClientArgs {
    SOCKET socket;
    SSL* ssl;
};

int handle_https(SOCKET client)
{
    SSL* ssl = SSL_new(ssl_ctx);
    if (!ssl)
    {
        closesocket(client);
        return 0;
    }
    SSL_set_fd(ssl, (int)client);
    if (SSL_accept(ssl) <= 0)
    {
        SSL_free(ssl);
        closesocket(client);
        return 0;
    }

    const int buffer_size = 1024 * 1024;
    std::string request_data;
    request_data.reserve(buffer_size);
    std::vector<char> buffer(buffer_size);

    int received = SSL_read(ssl, buffer.data(), buffer_size);
    if (received <= 0)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        closesocket(client);
        return 0;
    }

    request_data.append(buffer.data(), received);
    HttpRequest http_request = parse_http_request(request_data);
    std::string ip = get_client_ip(client);

    if (http_request.content_length > 0)
    {
        int headers_size = (int)request_data.find("\r\n\r\n") + 4;
        int total_received = received;
        int body_received = total_received - headers_size;

        while (body_received < http_request.content_length and total_received < FILE_SIZE)
        {
            received = SSL_read(ssl, buffer.data(), buffer_size);
            if (received <= 0)
            {
                break;
            }
            request_data.append(buffer.data(), received);
            total_received += received;
            body_received = total_received - headers_size;
        }

        http_request = parse_http_request(request_data);
    }

    auto upgrade_it = http_request.headers.find("upgrade");
    if (http_request.path.starts_with(config[ROOM]) and upgrade_it != http_request.headers.end() and upgrade_it->second == "websocket")
    {
        int room = 0;

        std::string sec_websocket_key;
        auto key_it = http_request.headers.find("sec-websocket-key");
        if (key_it != http_request.headers.end())
        {
            sec_websocket_key = key_it->second;
        }

        std::string accept_key = sec_websocket_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        unsigned char sha[20];
        SHA1(reinterpret_cast<const unsigned char*>(accept_key.c_str()), accept_key.size(), sha);
        char b64[32];
        int b64_len = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(b64), sha, 20);
        b64[b64_len] = '\0';
        std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: " + std::string(b64) + "\r\n\r\n";
        SSL_write(ssl, response.c_str(), (int)response.size());
        {
            size_t pos = http_request.path.rfind('/');
            if (pos == std::string::npos) {
                close_ws(ssl, client);
                return 0;
            }
            room = atoi(http_request.path.substr(pos + 1).c_str()) - 1;
            if (room < 0 or room >= atoi(config[ROOMS]))
            {
                close_ws(ssl, client);
                return 0;
            }

            auto it = std::find(creators.begin(), creators.end(), ip);

            if (clients[room].size() == 0 and it != creators.end()) {
                close_ws(ssl, client);
                return 0;
            }

            clients[room].push_back(std::shared_ptr<SSL>(ssl, [](SSL*) {}));
            if (clients[room].size() == 1)
            {
                creators.push_back(ip);
            }
            else if (clients[room].size() == 2)
            {
                ws_send(clients[room][0].get(), "{\"type\":\"role\",\"role\":\"offerer\"}");
                ws_send(clients[room][1].get(), "{\"type\":\"role\",\"role\":\"answerer\"}");
            }
            else if (clients[room].size() > 2)
            {
                ws_send(ssl, "{\"type\":\"status\",\"status\":\"occupied\"}");
                auto it = std::find_if(clients[room].begin(), clients[room].end(), [ssl](const std::shared_ptr<SSL>& s) { return s.get() == ssl; });
                clients[room].erase(it);
                close_ws(ssl, client);
                return 0;
            }
        }

        std::string msg;
        while (ws_recv(ssl, msg))
        {
            try
            {
                nlohmann::json j = nlohmann::json::parse(msg);
                if (j.contains("type") and !(j.contains("password")))
                {
                    std::string type = j["type"];

                    if (type == "offer" or type == "answer" or type == "ice_candidate" or type == "demo")
                    {
                        auto it = std::find_if(clients[room].begin(), clients[room].end(), [ssl](const std::shared_ptr<SSL>& s) { return s.get() == ssl; });
                        if (it != clients[room].end())
                        {
                            int idx = (int)std::distance(clients[room].begin(), it);
                            int other_idx = (idx == 0) ? 1 : 0;
                            if (other_idx < (int)clients[room].size())
                            {
                                ws_send(clients[room][other_idx].get(), msg);
                            }
                        }
                    }
                }
                else if (j.contains("type") and j.contains("password"))
                {
                    std::string type = j["type"];
                    std::string password = j["password"];

                    sqlite3_stmt* stmt;
                    sqlite3_prepare_v2(db, "SELECT password FROM passwords WHERE login = ?;", -1, &stmt, nullptr);
                    sqlite3_bind_text(stmt, 1, config[DEVELOPER], -1, SQLITE_STATIC);
                    sqlite3_step(stmt);
                    const char* admin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

                    if (type == "message" and std::string(admin) == password)
                    {
                        auto it = std::find_if(clients[room].begin(), clients[room].end(), [ssl](const std::shared_ptr<SSL>& s) { return s.get() == ssl; });
                        if (it != clients[room].end())
                        {
                            int idx = (int)std::distance(clients[room].begin(), it);
                            int other_idx = (idx == 0) ? 1 : 0;
                            if (other_idx < (int)clients[room].size())
                            {
                                ws_send(clients[room][other_idx].get(), msg);
                            }
                        }
                    }

                    sqlite3_finalize(stmt);
                }
            }
            catch (...)
            {

            }
        }

        {
            auto it = std::find_if(clients[room].begin(), clients[room].end(), [ssl](const std::shared_ptr<SSL>& s) { return s.get() == ssl; });
            if (it != clients[room].end())
            {
                clients[room].erase(it);
                auto it = std::find(creators.begin(), creators.end(), ip);
                if (it != creators.end()) {
                    int index = it - creators.begin();
                    std::swap(creators[index], creators.back());
                    creators.pop_back();
                }
                if (clients[room].size() == 1)
                {
                    ws_send(clients[room][0].get(), "{\"type\":\"peer_disconnected\"}");
                    rooms--;
                }
                rooms--;
            }
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        closesocket(client);
        return 0;
    }

    HttpResponse response;

    if (http_request.path.empty() or http_request.path == "/" or http_request.path == config[INDEX])
    {
        response = handle_static_file("");
        log_message("IP " + ip + " loaded index.html");
    }
    else if (http_request.path == config[UPLOAD] or http_request.path == config[API] or http_request.path == config[ADMIN])
    {
        if (!http_request.filename.empty() and !http_request.file_content.empty() and http_request.filename.find("/") == std::string::npos and http_request.filename.find("\\") == std::string::npos)
        {
            if (!http_request.comment.empty() and http_request.path == config[UPLOAD])
            {
                response = handle_new_comment(http_request, ip);
            }
            else
            {
                response = handle_new_post(http_request, ip);
            }
        }
        else if (!http_request.body.empty() and http_request.filename.find("/") == std::string::npos and http_request.filename.find("\\") == std::string::npos and (http_request.path == config[API] or http_request.path == config[ADMIN]))
        {
            std::string login = http_request.login;
            std::string password = http_request.password;

            try
            {
                nlohmann::json j = nlohmann::json::parse(http_request.body);
                login = j["Login"];
                password = j["Password"];
            }
            catch (...)
            {

            }

            response = handle_message(http_request.body, ip, login, password, http_request.path);
        }
        else if (http_request.filename.find("/") != std::string::npos or http_request.filename.find("\\") != std::string::npos)
        {
            response.headers = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        }
        else
        {
            if (!http_request.path.empty())
            {
                if (http_request.path.back() != '/')
                {
                    response.headers = "HTTP/1.1 301 Moved Permanently\r\nLocation: https://";
                    response.headers += config[DOMAIN];
                    response.headers += "/";
                    response.headers += http_request.path;
                    response.headers += "/\r\n\r\n";

                    moved_permanently(ssl, response, client);
                }
            }
        }
    }
    else if (http_request.path.starts_with(config[PROFILE]) and http_request.path.find('.') == std::string::npos and http_request.path != config[PROFILE])
    {
        std::string file = (std::string)config[PROFILE] + "/" + (std::string)config[PROFILE] + ".html";
        response = handle_static_file(file);
    }
    else
    {
        if (!http_request.path.empty() and !http_request.path.starts_with(config[UPLOAD]) and http_request.path.find('.') == std::string::npos)
        {
            if (http_request.path.back() != '/')
            {
                response.headers = "HTTP/1.1 301 Moved Permanently\r\nLocation: https://";
                response.headers += config[DOMAIN];
                response.headers += "/";
                response.headers += http_request.path;
                response.headers += "/\r\n\r\n";

                moved_permanently(ssl, response, client);
            }
            else
            {
                http_request.path.pop_back();
            }
        }

        if (http_request.path.find('.') == std::string::npos)
        {
            http_request.path = http_request.path + "/" + http_request.path + ".html";
        }

        if (http_request.path.find('.') != std::string::npos and http_request.path.back() == '/')
        {
            http_request.path.pop_back();

            response.headers = "HTTP/1.1 301 Moved Permanently\r\nLocation: https://";
            response.headers += config[DOMAIN];
            response.headers += "/";
            response.headers += http_request.path;
            response.headers += "\r\n\r\n";

            moved_permanently(ssl, response, client);
        }

        response = handle_static_file(http_request.path);
    }

    bool stop_server = false;
    try
    {
        nlohmann::json j = nlohmann::json::parse(response.body);
        std::string status;
        if (j.contains("Status"))
        {
            status = j["Status"];
        }
        if (status == "Stop")
        {
            stop_server = true;
        }
    }
    catch (...)
    {

    }

    if (!send_response(ssl, response))
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        closesocket(client);
        if (stop_server)
        {
            SSL_CTX_free(ssl_ctx);
            sqlite3_close(db);
            WSACleanup();
            exit(0);
        }
        return 0;
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    closesocket(client);

    if (stop_server)
    {
        SSL_CTX_free(ssl_ctx);
        sqlite3_close(db);
        WSACleanup();
        exit(0);
    }
    return 0;
}

static bool is_banned(const std::string& ip)
{
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, "SELECT 1 FROM banned WHERE ip = ? LIMIT 1;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);

    bool found = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return found;
}

static int accept_https(SOCKET server)
{
    thrd_t threads[16];
    int thread_count = 0;

    while (loop)
    {
        SOCKET client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET)
        {
            continue;
        }

        std::string ip = get_client_ip(client);

        if (is_banned(ip))
        {
            closesocket(client);
            continue;
        }

        int result = thrd_create(&threads[thread_count % 16], (thrd_start_t)handle_https, (SOCKET*)client);
        if (result == thrd_success)
        {
            thrd_detach(threads[thread_count % 16]);
            thread_count++;
        }
        else
        {
            closesocket(client);
        }
    }

    return 0;
}

static int accept_http(SOCKET server)
{
    thrd_t threads[16];
    int thread_count = 0;

    while (loop)
    {
        SOCKET client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET)
        {
            continue;
        }

        std::string ip = get_client_ip(client);

        if (is_banned(ip))
        {
            closesocket(client);
            continue;
        }

        std::string response = "HTTP/1.1 301 Moved Permanently\r\nLocation: https://";
        response += config[DOMAIN];
        response += "/\r\n\r\n";

        recv(client, NULL, NULL, NULL);
        send(client, response.c_str(), (int)response.length(), NULL);
        closesocket(client);
    }

    return 0;
}

static void load_config()
{
    FILE* file = nullptr;
    if (fopen_s(&file, "config.txt", "r") != 0 or !file)
    {
        return;
    }

    int save = 0;
    while (save < 256)
    {
        char line[256];
        if (!fgets(line, sizeof(line), file)) break;

        size_t len = strlen(line);
        if (len > 0 and line[len - 1] == '\n')
        {
            line[len - 1] = '\0';
        }

        char* pos_result = strstr(line, ": ");
        if (pos_result)
        {
            int pos = (int)(pos_result - line) + 2;
            strncpy_s(config[save], line + pos, 255);
        }
        else
        {
            strncpy_s(config[save], line, 255);
        }

        save++;
    }
    fclose(file);
}

static void init_databases()
{
    sqlite3_open(config[DATABASE], &db);
    sqlite3_open(config[USERS], &us);

    const char* create_tables =
        "CREATE TABLE IF NOT EXISTS banned (id INTEGER PRIMARY KEY AUTOINCREMENT, ip TEXT);"
        "CREATE TABLE IF NOT EXISTS users (ip TEXT PRIMARY KEY, user_id TEXT);"
        "CREATE TABLE IF NOT EXISTS passwords (login TEXT PRIMARY KEY, password TEXT);"
        "CREATE TABLE IF NOT EXISTS posts (post_id INTEGER PRIMARY KEY AUTOINCREMENT, path TEXT);"
        "CREATE TABLE IF NOT EXISTS likes (post_id INTEGER, user_id TEXT, PRIMARY KEY (post_id, user_id));"
        "CREATE TABLE IF NOT EXISTS views (post_id INTEGER PRIMARY KEY, count INTEGER DEFAULT 0);"
        "CREATE TABLE IF NOT EXISTS comments (comment_id INTEGER PRIMARY KEY AUTOINCREMENT, post_id INTEGER, path TEXT);"
        "CREATE TABLE IF NOT EXISTS accounts (ip TEXT, accounts INTEGER);"
        "CREATE TABLE IF NOT EXISTS times (ip TEXT, time INTEGER);"
        "CREATE UNIQUE INDEX idx_ip ON accounts(ip);"
        "CREATE UNIQUE INDEX idx_ip ON times(ip);";

    sqlite3_exec(db, create_tables, nullptr, nullptr, nullptr);
}

int main()
{
    WSADATA data;
    if (WSAStartup(0x101, &data))
    {
        return 1;
    }

    load_config();

    clients.resize(atoi(config[ROOMS]));

    creators.resize(atoi(config[ROOMS]));

    create_directory(config[UPLOAD]);

    init_databases();

    SOCKET https = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addres_https{};
    addres_https.sin_family = AF_INET;
    addres_https.sin_addr.s_addr = htonl(INADDR_ANY);
    addres_https.sin_port = htons(atoi(config[HTTPS]));

    bind(https, (sockaddr*)&addres_https, sizeof(addres_https));
    listen(https, SOMAXCONN);

    SOCKET http = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addres_http{};
    addres_http.sin_family = AF_INET;
    addres_http.sin_addr.s_addr = htonl(INADDR_ANY);
    addres_http.sin_port = htons(atoi(config[HTTP]));

    bind(http, (sockaddr*)&addres_http, sizeof(addres_http));
    listen(http, SOMAXCONN);

    FILE* file = nullptr;
    if (fopen_s(&file, config[LOG], "a") == 0 and file)
    {
        fprintf(file, "\n\n\n");
        fclose(file);
    }

    log_time();
    std::cout << "Start...\n";

    if (fopen_s(&file, config[LOG], "a") == 0 and file)
    {
        fprintf(file, "Start...\n");
        fclose(file);
    }

    log_time();
    std::cout << "Version of Web-Server: " << __DATE__ << '\n';

    if (fopen_s(&file, config[LOG], "a") == 0 and file)
    {
        fprintf(file, "Version of Web-Server: %s\n", __DATE__);
        fclose(file);
    }

    loop = true;
    thrd_t threads[2];

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (ssl_ctx)
    {
        if (SSL_CTX_use_certificate_file(ssl_ctx, config[CRT], SSL_FILETYPE_PEM) > 0 and SSL_CTX_use_PrivateKey_file(ssl_ctx, config[KEY], SSL_FILETYPE_PEM) > 0)
        {
            log_time();
            std::cout << "The Web-Server port for Comma is: " << atoi(config[HTTPS]) << "/" << atoi(config[HTTP]) << "\n\n";

            if (fopen_s(&file, config[LOG], "a") == 0 and file)
            {
                fprintf(file, "The Web-Server port for Comma is: %i/%i\n\n", atoi(config[HTTPS]), atoi(config[HTTP]));
                fclose(file);
            }

            thrd_create(threads, (thrd_start_t)accept_https, (SOCKET*)(intptr_t)https);
            thrd_create(threads, (thrd_start_t)accept_http, (SOCKET*)(intptr_t)http);
        }
        else
        {
            log_time();
            std::cout << "Failed to load certificate or key\n";

            if (fopen_s(&file, config[LOG], "a") == 0 and file)
            {
                fprintf(file, "Failed to load certificate or key\n");
                fclose(file);
            }

            log_time();
            std::cout << "The Web-Server port for Comma is: " << atoi(config[HTTP]) << "\n\n";

            if (fopen_s(&file, config[LOG], "a") == 0 and file)
            {
                fprintf(file, "The Web-Server port for Comma is: %i\n\n", atoi(config[HTTP]));
                fclose(file);
            }

            thrd_create(threads, (thrd_start_t)accept_http, (SOCKET*)(intptr_t)http);
        }
    }

    int key;
    do
    {
        key = _getch();
    } while (key != 27);

    loop = false;
    closesocket(https);
    SSL_CTX_free(ssl_ctx);
    sqlite3_close(db);
    WSACleanup();
    return 0;
}