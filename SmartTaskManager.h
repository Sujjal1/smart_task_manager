#ifndef SMART_TASK_MANAGER_H
#define SMART_TASK_MANAGER_H

#include <iostream>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <sqlite3.h>

using namespace std;

class Task {
public:
    string id;
    string description;
    string deadlineDetails; // A string representation of the deadline input (for display)
    int priority;
    string category;
    string status;
    long remainingHours; // computed remaining time in hours

    // Constructor with remainingHours and deadlineDetails.
    Task(string i, string d, int p, string c, string s, long rH, string dlDetails)
        : id(i), description(d), priority(p), category(c), status(s), remainingHours(rH), deadlineDetails(dlDetails) {}
};

class AVLTree {
private:
    struct Node {
        Task task;
        int key; // key is task.priority (uniqueness is maintained by shifting priorities)
        Node* left;
        Node* right;
        int height;
        
        Node(Task t) : task(t), key(t.priority), left(nullptr), right(nullptr), height(1) {}
    };

    Node* root;
    unordered_map<string, Node*> idMap; // fast lookup based on id

    int getHeight(Node* node) { return node ? node->height : 0; }
    int getBalance(Node* node) { return node ? getHeight(node->left) - getHeight(node->right) : 0; }

    void updateHeight(Node* node) {
        if (node)
            node->height = 1 + max(getHeight(node->left), getHeight(node->right));
    }

    Node* rotateLeft(Node* node) {
        Node* newRoot = node->right;
        node->right = newRoot->left;
        newRoot->left = node;
        updateHeight(node);
        updateHeight(newRoot);
        return newRoot;
    }

    Node* rotateRight(Node* node) {
        Node* newRoot = node->left;
        node->left = newRoot->right;
        newRoot->right = node;
        updateHeight(node);
        updateHeight(newRoot);
        return newRoot;
    }

    Node* rotateLeftRight(Node* node) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    Node* rotateRightLeft(Node* node) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    Node* balance(Node* node) {
        int balanceFactor = getBalance(node);
        if (balanceFactor > 1) {
            if (getBalance(node->left) < 0)
                return rotateLeftRight(node);
            else
                return rotateRight(node);
        }
        if (balanceFactor < -1) {
            if (getBalance(node->right) > 0)
                return rotateRightLeft(node);
            else
                return rotateLeft(node);
        }
        return node;
    }

    // Standard BST insert using task.priority as key.
    Node* insertHelper(Node* node, Task task) {
        if (!node) {
            Node* newNode = new Node(task);
            idMap[task.id] = newNode;
            return newNode;
        }
        if (task.priority < node->key)
            node->left = insertHelper(node->left, task);
        else if (task.priority > node->key)
            node->right = insertHelper(node->right, task);
        else
            throw runtime_error("Duplicate priority encountered.");
        
        updateHeight(node);
        return balance(node);
    }

    Node* findMin(Node* node) {
        while (node && node->left)
            node = node->left;
        return node;
    }

    Node* deleteHelper(Node* node, int key) {
        if (!node)
            return nullptr;
        if (key < node->key)
            node->left = deleteHelper(node->left, key);
        else if (key > node->key)
            node->right = deleteHelper(node->right, key);
        else {
            idMap.erase(node->task.id);
            if (!node->left || !node->right) {
                Node* temp = node->left ? node->left : node->right;
                delete node;
                return temp;
            } else {
                Node* minRight = findMin(node->right);
                node->task = minRight->task;
                node->key = minRight->key;
                node->right = deleteHelper(node->right, minRight->key);
            }
        }
        updateHeight(node);
        return balance(node);
    }

    void inOrder(Node* node, vector<Task>& tasks) {
        if (!node) return;
        inOrder(node->left, tasks);
        tasks.push_back(node->task);
        inOrder(node->right, tasks);
    }

    // Clears all nodes in the AVL tree.
    void clear(Node* node) {
        if (!node) return;
        clear(node->left);
        clear(node->right);
        delete node;
    }

public:
    AVLTree() : root(nullptr) {}

    ~AVLTree() {
        clear(root);
        idMap.clear();
    }

    // Inserts a task (task.priority should be set correctly beforehand)
    void insert(Task task) {
        if (idMap.find(task.id) != idMap.end())
            throw runtime_error("Task with the same ID already exists.");
        root = insertHelper(root, task);
    }

    // Rebuilds the entire tree from a sorted vector of tasks.
    void rebuild(const vector<Task>& tasks) {
        clear(root);
        root = nullptr;
        idMap.clear();
        for (const auto& t : tasks) {
            root = insertHelper(root, t);
        }
    }

    // Update task status directly (this does not reassign priority order)
    void updateTaskStatus(string id, string newStatus) {
        if (idMap.find(id) == idMap.end())
            throw runtime_error("Task ID not found.");
        idMap[id]->task.status = newStatus;
    }

    void deleteTask(string id) {
        if (idMap.find(id) == idMap.end())
            throw runtime_error("Task ID not found.");
        int key = idMap[id]->key;
        root = deleteHelper(root, key);
    }

    Task* search(string id) {
        return idMap.find(id) != idMap.end() ? &idMap[id]->task : nullptr;
    }

    vector<Task> listTasks() {
        vector<Task> tasks;
        inOrder(root, tasks);
        return tasks;
    }
};

class DBManager {
public:
    // Opens the SQLite database.
    static sqlite3* openDatabase() {
        sqlite3* db;
        if (sqlite3_open("database.db", &db) != SQLITE_OK) {
            throw runtime_error("Can't open database: " + string(sqlite3_errmsg(db)));
        }
        return db;
    }

    // Closes the SQLite database.
    static void closeDatabase(sqlite3* db) {
        sqlite3_close(db);
    }

    // Executes an SQL statement.
    static void executeSQL(sqlite3* db, const string &sql) {
        char *errMsg = nullptr;
        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            string error = errMsg;
            sqlite3_free(errMsg);
            throw runtime_error("SQL error: " + error);
        }
    }

    // Inserts a Task into the database.
    static void insertTask(const Task &task) {
        sqlite3* db = openDatabase();
        string sql = "INSERT INTO data (id, description, deadlineDetails, priority, category, status, remainingHours) VALUES ('" +
                     task.id + "', '" + task.description + "', '" + task.deadlineDetails + "', " + to_string(task.priority) + ", '" +
                     task.category + "', '" + task.status + "', " + to_string(task.remainingHours) + ");";
        executeSQL(db, sql);
        closeDatabase(db);
    }

    // Updates a Task in the database.
    static void updateTask(const Task &task) {
        sqlite3* db = openDatabase();
        string sql = "UPDATE data SET description='" + task.description + "', deadlineDetails='" + task.deadlineDetails +
                     "', priority=" + to_string(task.priority) + ", category='" + task.category +
                     "', status='" + task.status + "', remainingHours=" + to_string(task.remainingHours) +
                     " WHERE id='" + task.id + "';";
        executeSQL(db, sql);
        closeDatabase(db);
    }

    // Deletes a Task from the database.
    static void deleteTask(const string &id) {
        sqlite3* db = openDatabase();
        string sql = "DELETE FROM data WHERE id='" + id + "';";
        executeSQL(db, sql);
        closeDatabase(db);
    }

    // Loads all tasks from the database.
    static vector<Task> loadTasks() {
        vector<Task> tasks;
        sqlite3* db = openDatabase();
        string sql = "SELECT id, description, deadlineDetails, priority, category, status, remainingHours FROM data;";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
            closeDatabase(db);
            throw runtime_error("Failed to prepare statement.");
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            string id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            string description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            string deadlineDetails = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            int priority = sqlite3_column_int(stmt, 3);
            string category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            string status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            long remainingHours = sqlite3_column_int64(stmt, 6);
            tasks.push_back(Task(id, description, priority, category, status, remainingHours, deadlineDetails));
        }
        sqlite3_finalize(stmt);
        closeDatabase(db);
        return tasks;
    }

    // Rebuilds the database table to match the current tasks.
    static void rebuildTasks(const vector<Task> &tasks) {
        sqlite3* db = openDatabase();
        string sql = "DELETE FROM data;";
        executeSQL(db, sql);
        for (const auto &task : tasks) {
            sql = "INSERT INTO data (id, description, deadlineDetails, priority, category, status, remainingHours) VALUES ('" +
                  task.id + "', '" + task.description + "', '" + task.deadlineDetails + "', " + to_string(task.priority) + ", '" +
                  task.category + "', '" + task.status + "', " + to_string(task.remainingHours) + ");";
            executeSQL(db, sql);
        }
        closeDatabase(db);
    }
};

#endif

