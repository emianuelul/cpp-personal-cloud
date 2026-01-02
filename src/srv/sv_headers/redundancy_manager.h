#ifndef CPP_PERSONAL_CLOUD_REDUNDANCY_MANAGER_H
#define CPP_PERSONAL_CLOUD_REDUNDANCY_MANAGER_H

#include <iostream>
#include <ostream>
#include <sqlite3.h>
#include <string>

#include "picosha2.h"

class RedundancyManager {
public:
    static bool initDatabase() {
        sqlite3 *db;

        int rc = sqlite3_open("./storage/cloud.db", &db);
        if (rc != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
            return false;
        }

        std::string sql =
                "CREATE TABLE IF NOT EXISTS file_hashes ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "username TEXT NOT NULL, "
                "filename TEXT NOT NULL, "
                "hash TEXT NOT NULL, "
                "timestamp TEXT DEFAULT (strftime('%d/%m/%Y', 'now')), "
                "UNIQUE(username, filename)"
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

    static std::string calculateHash(std::string file_path) {
        std::ifstream file(file_path, std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "Can't open file for hashing: " << file_path << '\n';
            return "";
        }

        std::vector<unsigned char> hash(picosha2::k_digest_size);
        picosha2::hash256(file, hash.begin(), hash.end());
        file.close();

        return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
    }

    static bool saveFileHash(std::string user, std::string file_name, std::string hash) {
        sqlite3 *db;
        sqlite3_open("./storage/cloud.db", &db);

        std::string sql = "INSERT OR REPLACE INTO file_hashes (username, filename, hash) VALUES (?,?,?);";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, file_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, hash.c_str(), -1, SQLITE_TRANSIENT);

        int rc = sqlite3_step(stmt);

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        std::cout << "Saved to db\n";

        return rc == SQLITE_DONE;
    }

    static bool verifyFileIntegrity(std::string file_path, std::string db_hash) {
        if (db_hash.empty()) {
            std::cout << "No hash found in DB for this file... Skipping integrity check\n";
            return true;
        }

        std::string current_hash = calculateHash(file_path);

        return current_hash == db_hash;
    }

    static bool repairFromBackup(std::string primary_path, std::string backup_path) {
        if (!std::filesystem::exists(backup_path)) {
            std::cerr << "Backup file doesn't exist: " << backup_path << '\n';
            return false;
        }

        try {
            std::filesystem::copy_file(backup_path, primary_path,
                                       std::filesystem::copy_options::overwrite_existing);

            std::cout << "File repaired from backup: " << primary_path << '\n';
            return true;
        } catch (const std::filesystem::filesystem_error &e) {
            std::cerr << "Repair failed: " << e.what() << '\n';
            return false;
        }
    }

    static std::string getStoredHash(std::string user, std::string file_name) {
        sqlite3 *db;
        sqlite3_open("./storage/cloud.db", &db);

        std::string sql = "SELECT hash FROM file_hashes WHERE username = ? AND filename = ?;";

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, file_name.c_str(), -1, SQLITE_TRANSIENT);

        std::string hash = "";

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char *result = sqlite3_column_text(stmt, 0);
            if (result) {
                hash = std::string(reinterpret_cast<const char *>(result));
            }
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return hash;
    }
};

#endif
