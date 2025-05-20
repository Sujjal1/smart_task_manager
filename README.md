
# 🧠 Smart Task Manager (AVL-Powered Priority Task Scheduler)

Welcome to the **Smart Task Manager**, an intelligent command-line application that allows users to manage their tasks efficiently based on **priority**, **deadline**, and **completion status**. It is built using **C++** with **SQLite** database integration and uses an **AVL Tree** to dynamically maintain task priority ordering.

---

## 📌 Features

✅ Insert new tasks with ID, description, category, and deadline  
✅ Automatically assign priorities based on how soon tasks are due  
✅ Update task status (complete/incomplete) with reordering  
✅ Delete tasks by ID  
✅ Search for tasks by ID  
✅ List all tasks sorted by priority  
✅ Data is persistently stored using SQLite and reflected in-memory using an AVL Tree

---

## 🛠️ Technologies Used

| Component      | Description                                                  |
|----------------|--------------------------------------------------------------|
| **C++**        | Core logic and application structure                         |
| **SQLite3**    | Persistent database storage (via `sqlite3.h`)                |
| **AVL Tree**   | Self-balancing BST used to maintain priority-based ordering  |
| **OOP Concepts** | Encapsulation, Inheritance (basic), Custom Classes        |
| **Standard Template Library (STL)** | Vectors, Unordered Maps, String Utilities |

---

## ⚙️ Project Structure

```
├── Main.cpp                 # CLI user interface and interaction logic
├── SmartTaskManager.h       # Contains Task class, AVLTree class, DBManager class
├── database.db              # SQLite database file (auto-managed by the app)
```

---

## 🧮 How Priority is Determined

When a new task is inserted:

- If **complete**, it is placed at the end (lowest priority).
- If **incomplete**, it is compared with other tasks:
  - Sooner deadline → higher priority (lower number)
  - Later deadline → lower priority
  - Intermediate deadline → inserted and shifts subsequent tasks down

AVL Tree ensures that insertion and retrieval maintains **O(log n)** performance while keeping tasks sorted by priority.

---

## 🧩 Classes & Modules Overview

### ✅ `Task`
Represents an individual task.
- Fields: `id`, `description`, `priority`, `category`, `status`, `remainingHours`, `deadlineDetails`

### ✅ `AVLTree`
Stores tasks in a balanced binary search tree based on priority.
- Fast insert, delete, search
- Maintains internal map (`idMap`) for direct ID-based access
- Rebuilds from sorted task list when needed

### ✅ `DBManager`
Handles interaction with `database.db` using `sqlite3.h`.
- Methods: `insertTask`, `updateTask`, `deleteTask`, `loadTasks`, `rebuildTasks`
- Automatically opens/closes database connections

---

## 💡 Sample Use Cases

```
1. Insert Task
2. Update Task Status
3. Delete Task
4. Search Task
5. List Tasks
6. Exit
```

Sample CLI Flow:
```
Enter Task ID: T1
Enter Description: Finish report
Enter Category: Work
Enter remaining years (0 if this year): 0
Enter remaining months (0 if this month): 0
Enter remaining days (0 if today): 2
Task inserted successfully with priority 1!
```

---

## 🔄 Data Persistence Workflow

1. App starts: `DBManager::loadTasks()` retrieves all tasks from `database.db`
2. AVL Tree is rebuilt using `tree.rebuild()`
3. On every insert/update/delete:
   - AVL tree is rebuilt or modified
   - `DBManager::rebuildTasks()` syncs in-memory data back to the database

---

## 🧪 How to Compile and Run

### 📦 Dependencies
Ensure `sqlite3` development libraries are installed:

```bash
sudo apt-get install libsqlite3-dev
```

### 🔧 Compile
```bash
g++ Main.cpp -o TaskManager -lsqlite3
```

### ▶️ Run
```bash
./TaskManager
```

---

## 🛡️ Error Handling & Input Validation

- Invalid numeric input is caught using `cin.fail()` and handled gracefully
- Duplicate Task IDs are rejected
- Choices outside valid menu range (1–6) prompt user again
- SQL errors are reported using exception messages

---

## 🚀 Future Improvements

- GUI frontend using Qt or Tkinter (via Python wrapper)
- Reminders/notifications for upcoming tasks
- Categorized task views (e.g., filter by category or status)
- Export/Import CSV or JSON
- Multi-user login with session tracking

---

## 👨‍💻 Authors

**Sujjal Chapagain**  
**Abhiyan Poudel**
**Hanzla Hamid**
**Arjav Lamsal**
Sophomore, Computer Science  
University of Southern Mississippi

---

## 📜 License

This project is open-source and available for educational use and extensions.
