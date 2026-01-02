#ifndef CPP_PERSONAL_CLOUD_DB_MANAGER_H
#define CPP_PERSONAL_CLOUD_DB_MANAGER_H
#include <sqlite3.h>
#include <string>

class DBManager {
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
};

#endif //CPP_PERSONAL_CLOUD_DB_MANAGER_H
