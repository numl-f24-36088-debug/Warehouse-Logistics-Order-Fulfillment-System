///////////////////////////////////////////////////////////////
// Warehouse Logistics and Order Fulfilment System
///////////////////////////////////////////////////////////////
#define UNICODE
#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

/////////////////////////////////////////////////////
// QUEUE: Orders
/////////////////////////////////////////////////////
class Order {
public:
    int orderID;
    Order* next;

    Order(int id) {
        orderID = id;
        next = NULL;
    }
};

class OrderQueue {
public:
    Order* front;
    Order* rear;

    OrderQueue() {
        front = rear = NULL;
    }

    void enqueue(int id) {
        Order* temp = new Order(id);
        if (rear == NULL) {
            front = rear = temp;
            return;
        }
        rear->next = temp;
        rear = temp;
    }

    int dequeue() {
        if (front == NULL) {
            return -1;
        }
        int removedID = front->orderID;
        Order* temp = front;
        front = front->next;
        delete temp;
        if (front == NULL) rear = NULL;
        return removedID;
    }

    void display() {
        Order* t = front;
        cout << "Order Queue: ";
        while (t != NULL) {
            cout << t->orderID << " ";
            t = t->next;
        }
        cout << "\n";
    }
};

/////////////////////////////////////////////////////
// LINKED LIST: Items in a category
/////////////////////////////////////////////////////
class Item {
public:
    int id;
    string name;
    int quantity;
    Item* next;

    Item(int i, string n, int q) {
        id = i;
        name = n;
        quantity = q;
        next = NULL;
    }
};

class ItemList {
public:
    Item* head;

    ItemList() {
        head = NULL;
    }

    void addItem(int id, string name, int qty) {
        Item* temp = new Item(id, name, qty);
        temp->next = head;
        head = temp;
    }

    void display() {
        Item* t = head;
        while (t != NULL) {
            cout << "  ItemID: " << t->id << " | "
                 << t->name << " | Qty: " << t->quantity << "\n";
            t = t->next;
        }
    }
};

/////////////////////////////////////////////////////
// BINARY TREE: Categories
/////////////////////////////////////////////////////
class CategoryNode {
public:
    string categoryName;
    ItemList items;
    CategoryNode* left;
    CategoryNode* right;

    CategoryNode(string n) {
        categoryName = n;
        left = right = NULL;
    }
};

class CategoryTree {
public:
    CategoryNode* root;

    CategoryTree() {
        root = NULL;
    }

    CategoryNode* insert(CategoryNode* node, string name) {
        if (node == NULL) return new CategoryNode(name);
        if (name < node->categoryName)
            node->left = insert(node->left, name);
        else
            node->right = insert(node->right, name);
        return node;
    }

    void addCategory(string name) {
        root = insert(root, name);
    }

    CategoryNode* search(CategoryNode* node, string name) {
        if (!node) return NULL;
        if (name == node->categoryName) return node;
        if (name < node->categoryName) return search(node->left, name);
        return search(node->right, name);
    }
};

/////////////////////////////////////////////////////
// STACK: Undo actions
/////////////////////////////////////////////////////
class UndoNode {
public:
    string action;
    UndoNode* next;

    UndoNode(string a) {
        action = a;
        next = NULL;
    }
};

class UndoStack {
public:
    UndoNode* top;

    UndoStack() {
        top = NULL;
    }

    void push(string act) {
        UndoNode* temp = new UndoNode(act);
        temp->next = top;
        top = temp;
    }

    void pop() {
        if (top == NULL) return;
        UndoNode* temp = top;
        top = top->next;
        delete temp;
    }
};

///////////////////////////////////////////////////////////////
// GLOBAL OBJECTS (used by both console & GUI)
///////////////////////////////////////////////////////////////
CategoryTree categories;
OrderQueue orders;
UndoStack undoStack;

///////////////////////////////////////////////////////////////
// SYSTEM HELPER FUNCTIONS USED BY THE GUI ---
///////////////////////////////////////////////////////////////
void AddCategoryGUI(string name) {
    categories.addCategory(name);
    undoStack.push("Added category: " + name);
}
void AddItemGUI(string cat, int id, string name, int qty) {
    CategoryNode* c = categories.search(categories.root, cat);
    if (!c) return;
    c->items.addItem(id, name, qty);
    undoStack.push("Added item " + name + " to " + cat);
}
void EnqueueOrderGUI(int id) {
    orders.enqueue(id);
}

string toStr(int value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

int ProcessOrderGUI() {
    int processedID = orders.dequeue();
    if (processedID == -1) {
        return -1;
    }

    const int MAXSTACK = 1024;
    CategoryNode* stack[MAXSTACK];
    int top = -1;
    CategoryNode* curr = categories.root;

    while (curr != NULL || top != -1) {
        while (curr != NULL) {
            if (top + 1 >= MAXSTACK) break;
            stack[++top] = curr;
            curr = curr->left;
        }
        if (top < 0) break;
        curr = stack[top--];

        // search items in this category
        Item* it = curr->items.head;
        while (it) {
            if (it->id == processedID) {
                if (it->quantity > 0) {
                    it->quantity -= 1;
                    undoStack.push("Processed order: " + toStr(processedID) +
                                   " (deducted 1 from " + curr->categoryName + ")");
                } else {
                    undoStack.push("Processed order: " + toStr(processedID) +
                                   " (found in " + curr->categoryName + " but quantity was 0)");
                }
                return processedID;
            }
            it = it->next;
        }

        curr = curr->right;
    }

    undoStack.push("Processed order: " + toStr(processedID) + " (item not found in stock)");
    return processedID;
}

///////////////////////////////////////////////////////////////
//              WINAPI GUI SECTION
///////////////////////////////////////////////////////////////
HWND hCatBox, hItemName, hItemQty, hItemID, hOrderID;
HINSTANCE g_hInst;

///////////////////////////////////////////////////////////////
// HELPER FUNCTION TO COLLECT ALL DATA INTO STRING
///////////////////////////////////////////////////////////////
void CollectAllData(string &outText, CategoryNode* node) {
    if (!node) return;

    CollectAllData(outText, node->left);

    outText += "\r\n\r\nCategory: " + node->categoryName + "\n";
    Item* t = node->items.head;
    while (t) {
        outText += "\r\nItemID: " + toStr(t->id) + "\r\nName: " + t->name + "\r\nQty: " + toStr(t->quantity) + "\n";
        t = t->next;
    }
    outText += "\n";

    CollectAllData(outText, node->right);
}

///////////////////////////////////////////////////////////////
// SHOW ALL DATA WINDOW
///////////////////////////////////////////////////////////////
void ShowAllDataGUI() {
    // Collect categories and items
    string displayText = "=== Warehouse Categories and Items ===\r\n";
    CollectAllData(displayText, categories.root);

    // Orders
    displayText += "\r\n=== Current Order Queue ===\r\n";
    Order* o = orders.front;
    while (o) {
        displayText += "OrderID: " + toStr(o->orderID) + "\r\n";
        o = o->next;
    }

    // Undo Stack
    displayText += "\r\n=== Previously done action ===\r\n";
    UndoNode* u = undoStack.top;
    while (u) {
        displayText += u->action + "\r\n";
        u = u->next;
    }

    wstring ws(displayText.begin(), displayText.end());

    HWND hDisplayWnd = CreateWindowW(L"Static", L"All Data",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        250, 150, 500, 400, NULL, NULL, g_hInst, NULL);

    CreateWindowW(L"EDIT", ws.c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
        10, 30, 460, 340, hDisplayWnd, NULL, g_hInst, NULL);
}

///////////////////////////////////////////////////////////////
// GUI CALLBACK
///////////////////////////////////////////////////////////////
LRESULT CALLBACK GuiProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE:
        CreateWindowW(L"static", L"Category:",
            WS_VISIBLE | WS_CHILD, 20, 20, 100, 20, h, NULL, g_hInst, NULL);

        hCatBox = CreateWindowW(L"edit", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            100, 20, 150, 20, h, NULL, g_hInst, NULL);

        CreateWindowW(L"button", L"Add Category",
            WS_VISIBLE | WS_CHILD, 270, 20, 120, 25, h, (HMENU)1, g_hInst, NULL);

        CreateWindowW(L"static", L"Item Name:",
            WS_VISIBLE | WS_CHILD, 20, 70, 100, 20, h, NULL, g_hInst, NULL);
        hItemName = CreateWindowW(L"edit", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 70, 150, 20, h, NULL, g_hInst, NULL);

        CreateWindowW(L"static", L"Item ID:",
            WS_VISIBLE | WS_CHILD, 20, 100, 100, 20, h, NULL, g_hInst, NULL);
        hItemID = CreateWindowW(L"edit", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 100, 150, 20, h, NULL, g_hInst, NULL);

        CreateWindowW(L"static", L"Quantity:",
            WS_VISIBLE | WS_CHILD, 20, 130, 100, 20, h, NULL, g_hInst, NULL);
        hItemQty = CreateWindowW(L"edit", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 130, 150, 20, h, NULL, g_hInst, NULL);

        CreateWindowW(L"button", L"Add Item",
            WS_VISIBLE | WS_CHILD, 270, 115, 120, 25, h, (HMENU)2, g_hInst, NULL);

        CreateWindowW(L"static", L"Order ID:",
            WS_VISIBLE | WS_CHILD, 20, 200, 100, 20, h, NULL, g_hInst, NULL);
        hOrderID = CreateWindowW(L"edit", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 200, 150, 20, h, NULL, g_hInst, NULL);

        CreateWindowW(L"button", L"Add Order",
            WS_VISIBLE | WS_CHILD, 270, 200, 120, 25, h, (HMENU)3, g_hInst, NULL);

        CreateWindowW(L"button", L"Process Order",
            WS_VISIBLE | WS_CHILD, 270, 240, 120, 25, h, (HMENU)4, g_hInst, NULL);

        CreateWindowW(L"button", L"Display All Data",
            WS_VISIBLE | WS_CHILD, 150, 280, 150, 25, h, (HMENU)5, g_hInst, NULL);

        break;

    case WM_COMMAND:
        switch (LOWORD(w)) {
        case 1: { 
            wchar_t buff[100];
            GetWindowTextW(hCatBox, buff, 100);
            wstring ws(buff);
            AddCategoryGUI(string(ws.begin(), ws.end()));
            MessageBoxW(h, L"Category Added!", L"Success", MB_OK);
        } break;

        case 2: { // Add Item
            wchar_t n[100], q[50], idb[50], c[100];
            GetWindowTextW(hCatBox, c, 100);       // category to place item in
            GetWindowTextW(hItemName, n, 100);    // item name
            GetWindowTextW(hItemQty, q, 50);      // qty
            GetWindowTextW(hItemID, idb, 50);     // id

            string cat(c, c + wcslen(c));
            string name(n, n + wcslen(n));
            int qty = _wtoi(q);
            int id = _wtoi(idb);

            AddItemGUI(cat, id, name, qty);
            MessageBoxW(h, L"Item Added!", L"Success", MB_OK);
        } break;

        case 3: { // Add Order
            wchar_t buff[50];
            GetWindowTextW(hOrderID, buff, 50);
            int id = _wtoi(buff);
            EnqueueOrderGUI(id);
            MessageBoxW(h, L"Order Added!", L"Success", MB_OK);
        } break;

        case 4: { // Process Order
            int processedID = ProcessOrderGUI();

            if (processedID == -1) {
                MessageBoxW(h, L"No orders in queue!", L"Empty", MB_OK);
            } else {
                wstringstream ss;
                ss << L"Processed Order ID: " << processedID;
                MessageBoxW(h, ss.str().c_str(), L"Done", MB_OK);
            }
        } break;

        case 5: { // Display All Data
            ShowAllDataGUI();
        } break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(h, m, w, l);
}

///////////////////////////////////////////////////////////////
// RUN GUI
///////////////////////////////////////////////////////////////
int RunGUI(HINSTANCE hInst) {
    g_hInst = hInst;

    WNDCLASSW wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInst;
    wc.lpfnWndProc = GuiProc;
    wc.lpszClassName = L"WarehouseGUI";
    RegisterClassW(&wc);

    HWND h = CreateWindowW(L"WarehouseGUI", L"Warehouse Management GUI",
        WS_OVERLAPPEDWINDOW, 200, 100, 450, 350,
        NULL, NULL, hInst, NULL);

    ShowWindow(h, SW_SHOW);
    UpdateWindow(h);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

///////////////////////////////////////////////////////////////
// ENTRY POINT
///////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    return RunGUI(hInst);
}
