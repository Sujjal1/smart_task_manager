#include "SmartTaskManager.h"
#include <limits>
#include <cctype>
#include <vector>
#include <algorithm>
#include <sqlite3.h>

// Helper function to safely read an integer
int getIntInput(const string &prompt) {
    int value;
    while (true) {
        cout << prompt;
        cin >> value;
        if(cin.fail()){
            cout << "Invalid input. Please enter a number.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        } else {
            break;
        }
    }
    return value;
}

// Helper function to get a yes/no answer from the user
char getYesNoInput(const string &prompt) {
    char answer;
    while (true) {
        cout << prompt;
        cin >> answer;
        answer = tolower(answer);
        if(answer == 'y' || answer == 'n') {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return answer;
        } else {
            cout << "Invalid input. Please enter y or n.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
    return 'n'; // default (should never reach here)
}

int main() {
    AVLTree tree;
    // Load tasks from the database and rebuild the AVL tree.
    vector<Task> tasks;
    try {
        tasks = DBManager::loadTasks();
    } catch (const runtime_error &e) {
        cout << "Error loading tasks from database: " << e.what() << "\n";
    }
    tree.rebuild(tasks);

    while (true) {
        cout << "\n----- SMART TASK MANAGER -----\n";
        cout << "1. Insert Task\n2. Update Task Status\n3. Delete Task\n4. Search Task\n5. List Tasks\n6. Exit\nChoice: ";
        int choice;
    while (true) {
        cout << "Enter your choice (1-6): ";
        if (!(cin >> choice)) {
            cout << "Invalid input. Please enter a number between 1 and 6." << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        if (choice < 1 || choice > 6) {
            cout << "Choice should be from 1 to 6. Try Again." << endl;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // clear any extra input
        break;
    }

        if (choice == 1) {
            string id;
            cout << "Enter Task ID: ";
            getline(cin, id);

            // Validate unique ID.
            if (tree.search(id) != nullptr) {
                cout << "Error: A task with this ID already exists.\n";
                continue;
            }
            
            string desc;
            cout << "Enter Description: ";
            getline(cin, desc);
            
            string category;
            cout << "Enter Category: ";
            getline(cin, category);
            
            // Ask if the task is complete at insertion.
            string status = "incomplete";
            
            // Ask for deadline details only if task is incomplete.
            long totalRemainingHours = 0;
            string deadlineStr = "N/A";
            if (status == "incomplete") {
                int years = getIntInput("Enter remaining years (0 if this year): ");
                int months = getIntInput("Enter remaining months (0 if this month): ");
                int days = getIntInput("Enter remaining days (0 if today): ");
                int hours = 0;
                if (years == 0 && months == 0 && days == 0) {
                    hours = getIntInput("Enter remaining hours: ");
                }
                totalRemainingHours = static_cast<long>(years)*365*24 + static_cast<long>(months)*30*24 + static_cast<long>(days)*24 + hours;
                deadlineStr = to_string(years) + " year(s), " + to_string(months) + " month(s), " +
                              to_string(days) + " day(s)";
                if (years == 0 && months == 0 && days == 0)
                    deadlineStr += ", " + to_string(hours) + " hour(s)";
            }
            
            // Get current tasks (sorted by priority).
            tasks = tree.listTasks();
            int newPriority = 1;
            if (status == "complete") {
                if (!tasks.empty()) {
                    newPriority = tasks.back().priority + 1;
                }
            } else {
                if (!tasks.empty()) {
                    if (totalRemainingHours < tasks.front().remainingHours) {
                        newPriority = 1;
                        for (auto &t : tasks) {
                            t.priority++;
                        }
                    }
                    else if (totalRemainingHours >= tasks.back().remainingHours) {
                        newPriority = tasks.back().priority + 1;
                    }
                    else {
                        for (auto &t : tasks) {
                            if (totalRemainingHours < t.remainingHours) {
                                newPriority = t.priority;
                                break;
                            }
                        }
                        for (auto &t : tasks) {
                            if (t.priority >= newPriority) {
                                t.priority++;
                            }
                        }
                    }
                }
            }
            
            Task newTask(id, desc, newPriority, category, status, totalRemainingHours, deadlineStr);
            tasks.push_back(newTask);
            sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
                return a.priority < b.priority;
            });
            
            tree.rebuild(tasks);
            try {
                DBManager::rebuildTasks(tasks);
            } catch (const runtime_error &e) {
                cout << "Database error: " << e.what() << "\n";
            }
            cout << "Task inserted successfully with priority " << newPriority << "!\n";
        }
        else if (choice == 2) {
            // Update task status with repositioning.
            string id;
            cout << "Enter Task ID: ";
            getline(cin, id);
            Task* taskPtr = tree.search(id);
            if (!taskPtr) {
                cout << "Task not found.\n";
                continue;
            }
            char completeAnswer = getYesNoInput("Do you want to mark as complete? (y/n): ");
            string newStatus = (completeAnswer == 'y') ? "complete" : "incomplete";
            
            tasks = tree.listTasks();
            bool found = false;
            for (auto &t : tasks) {
                if (t.id == id) {
                    t.status = newStatus;
                    found = true;
                    break;
                }
            }
            if (!found) {
                cout << "Task not found in list.\n";
                continue;
            }
            
            // Reorder tasks: separate incomplete and complete.
            vector<Task> incompleteTasks, completeTasks;
            for (auto &t : tasks) {
                if (t.status == "incomplete")
                    incompleteTasks.push_back(t);
                else
                    completeTasks.push_back(t);
            }
            sort(incompleteTasks.begin(), incompleteTasks.end(), [](const Task &a, const Task &b) {
                return a.remainingHours < b.remainingHours;
            });
            sort(completeTasks.begin(), completeTasks.end(), [](const Task &a, const Task &b) {
                return a.priority < b.priority;
            });
            
            tasks.clear();
            tasks.insert(tasks.end(), incompleteTasks.begin(), incompleteTasks.end());
            tasks.insert(tasks.end(), completeTasks.begin(), completeTasks.end());
            
            int newPriority = 1;
            for (auto &t : tasks) {
                t.priority = newPriority++;
            }
            tree.rebuild(tasks);
            try {
                DBManager::rebuildTasks(tasks);
            } catch (const runtime_error &e) {
                cout << "Database error: " << e.what() << "\n";
            }
            cout << "Task status updated and priorities re-assigned!\n";
        }
        else if (choice == 3) {
            string id;
            cout << "Enter Task ID to delete: ";
            getline(cin, id);
            try {
                tree.deleteTask(id);
                // Update database.
                DBManager::deleteTask(id);
                cout << "Task deleted.\n";
            } catch (const runtime_error& e) {
                cout << e.what() << "\n";
            }
            tasks = tree.listTasks();
        }
        else if (choice == 4) {
            string id;
            cout << "Enter Task ID to search: ";
            getline(cin, id);
            Task* task = tree.search(id);
            if (task) {
                cout << "Found Task:\n";
                cout << "ID: " << task->id << "\nDescription: " << task->description 
                     << "\nDeadline: " << task->deadlineDetails << "\nPriority: " << task->priority 
                     << "\nCategory: " << task->category << "\nStatus: " << task->status << "\n";
            } else {
                cout << "Task not found.\n";
            }
        }
        else if (choice == 5) {
            tasks = tree.listTasks();
            cout << "\n----- TASK LIST -----\n";
            for (const auto& t : tasks) {
                cout << "Priority " << t.priority << " | ID: " << t.id << " | Desc: " << t.description 
                     << " | Deadline: " << t.deadlineDetails << " | Status: " << t.status << "\n";
            }
        }
        else if (choice == 6) {
            cout << "Exiting Task Manager. Goodbye!\n";
            break;
        }
        else {
            cout << "Invalid choice. Try again.\n";
        }
    }
    return 0;
}
