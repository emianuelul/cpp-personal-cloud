#ifndef CPP_PERSONAL_CLOUD_DB_MANAGER_H
#define CPP_PERSONAL_CLOUD_DB_MANAGER_H
#include <random>
#include <sqlite3.h>
#include <string>
#include <picosha2.h>

class DBManager {
private:
    static std::string generate_salt(size_t length = 16) {
        static const char chars[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
        std::string salt;
        for (size_t i = 0; i < length; ++i) {
            salt += chars[dis(gen)];
        }
        return salt;
    }

    static std::string hash_password(const std::string &password, const std::string &salt) {
        std::string salted_password = salt + password;
        std::string hash_hex;
        picosha2::hash256_hex_string(salted_password, hash_hex);
        return salt + ":" + hash_hex;
    }

public:
    static bool removeFile(std::string user, std::string filename) {
        sqlite3 *db;
        sqlite3_open("./storage/cloud.db", &db);

        sqlite3_stmt *stmt;

        std::string sql = "DELETE FROM file_hashes WHERE username = ? AND filename = ?;";
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, filename.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Error deleting " << filename << " from db\n";
            return false;
        }

        std::cout << "Deleted " << filename << " from db\n";

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return true;
    }

    static bool initUsers() {
        sqlite3 *db;

        int rc = sqlite3_open("./storage/cloud.db", &db);
        if (rc != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
            return false;
        }

        std::string sql =
                "CREATE TABLE IF NOT EXISTS users ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "username TEXT NOT NULL UNIQUE, "
                "password_hash TEXT NOT NULL, "
                "created_at TEXT DEFAULT (strftime('%d/%m/%Y', 'now')) "
                ");";

        char *err_msg = nullptr;
        rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << err_msg << '\n';
            sqlite3_free(err_msg);
            sqlite3_close(db);
            return false;
        }

        sqlite3_close(db);
        return true;
    }

    static bool try_register(std::string user, std::string pass) {
        sqlite3 *db;

        int rc = sqlite3_open("./storage/cloud.db", &db);
        if (rc != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
            return false;
        }

        std::string salt = generate_salt();
        std::string password_hash = hash_password(pass, salt);

        sqlite3_stmt *stmt;
        std::string sql = "INSERT INTO users (username, password_hash) VALUES (?, ?);";

        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
            sqlite3_close(db);
            return false;
        }

        sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE) {
            if (rc == SQLITE_CONSTRAINT) {
                std::cerr << "Username already exists!\n";
            } else {
                std::cerr << "Error registering user: " << sqlite3_errmsg(db) << '\n';
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return false;
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return true;
    }

    static bool try_login(std::string user, std::string pass) {
        sqlite3 *db;

        int rc = sqlite3_open("./storage/cloud.db", &db);
        if (rc != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
            return false;
        }

        sqlite3_stmt *stmt;
        std::string sql = "SELECT password_hash FROM users WHERE username = ?;";

        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
            sqlite3_close(db);
            return false;
        }

        sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_ROW) {
            std::cerr << "User not found\n";
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return false;
        }

        std::string stored_hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));

        size_t delimiter_pos = stored_hash.find(':');
        if (delimiter_pos == std::string::npos) {
            std::cerr << "Invalid password hash format\n";
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return false;
        }

        std::string salt = stored_hash.substr(0, delimiter_pos);
        std::string stored_password_hash = stored_hash.substr(delimiter_pos + 1);

        std::string input_hash = hash_password(pass, salt);
        input_hash = input_hash.substr(delimiter_pos + 1);

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        if (input_hash == stored_password_hash) {
            std::cout << "Login successful\n";
            return true;
        } else {
            std::cerr << "Invalid password\n";
            return false;
        }
    }

    static int get_user_id(const std::string &user) {
        sqlite3 *db;

        int rc = sqlite3_open("./storage/cloud.db", &db);
        if (rc != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
            return false;
        }

        sqlite3_stmt *stmt;
        std::string sql = "SELECT id FROM users WHERE username = ?;";

        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
            sqlite3_close(db);
            return false;
        }

        sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_ROW) {
            std::cerr << "User not found\n";
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return false;
        }

        int id = sqlite3_column_int(stmt, 0);

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return id;
    }

    static std::string get_user_hash(int user_id) {
        sqlite3 *db;

        int rc = sqlite3_open("./storage/cloud.db", &db);
        if (rc != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
            return "";
        }

        sqlite3_stmt *stmt;
        std::string sql = "SELECT password_hash FROM users WHERE id = ?;";

        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
            sqlite3_close(db);
            return "";
        }

        sqlite3_bind_int(stmt, 1, user_id);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_ROW) {
            std::cerr << "User not found\n";
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return "";
        }

        const char *hash_ch = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string hash = hash_ch;

        size_t delimiter_pos = hash.find(':');
        if (delimiter_pos == std::string::npos) {
            std::cerr << "Invalid password hash format\n";
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return "";
        }

        hash = hash.substr(delimiter_pos + 1);

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return hash;
    }
};

#endif //CPP_PERSONAL_CLOUD_DB_MANAGER_H
