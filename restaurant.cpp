// ============================================================
//   RESTAURANT ORDERING SYSTEM — C++
//   Features: Menu Display, Cart, Orders, Billing, Admin
// ============================================================

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <limits>

using namespace std;

// ─── ANSI Color Codes ────────────────────────────────────────
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BG_BLUE "\033[44m"

// ─── Data Structures ─────────────────────────────────────────

struct MenuItem {
    int         id;
    string      name;
    string      category;
    double      price;
    string      description;
    bool        available;
    int         prepTime; // minutes
};

struct OrderItem {
    MenuItem    item;
    int         quantity;
    string      specialRequest;
    double      subtotal() const { return item.price * quantity; }
};

enum class OrderStatus {
    PENDING,
    CONFIRMED,
    PREPARING,
    READY,
    SERVED,
    CANCELLED
};

string statusToString(OrderStatus s) {
    switch(s) {
        case OrderStatus::PENDING:    return "⏳ PENDING";
        case OrderStatus::CONFIRMED:  return "✅ CONFIRMED";
        case OrderStatus::PREPARING:  return "🍳 PREPARING";
        case OrderStatus::READY:      return "🔔 READY";
        case OrderStatus::SERVED:     return "🍽  SERVED";
        case OrderStatus::CANCELLED:  return "❌ CANCELLED";
        default: return "UNKNOWN";
    }
}

struct Order {
    int                   id;
    int                   tableNumber;
    string                customerName;
    vector<OrderItem>     items;
    OrderStatus           status;
    time_t                createdAt;
    double                taxRate   = 0.16; // 16% VAT (Kenya)
    double                serviceCharge = 0.10; // 10%

    double subtotal() const {
        double total = 0;
        for (auto& i : items) total += i.subtotal();
        return total;
    }
    double tax()     const { return subtotal() * taxRate; }
    double service() const { return subtotal() * serviceCharge; }
    double total()   const { return subtotal() + tax() + service(); }

    string createdAtStr() const {
        char buf[20];
        strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&createdAt));
        return string(buf);
    }
};

// ─── Global State ─────────────────────────────────────────────
vector<MenuItem> menu;
vector<Order>    orders;
int              nextOrderId   = 1001;
int              nextMenuId    = 1;
const string     ADMIN_PIN     = "1234";

// ─── Helpers ──────────────────────────────────────────────────

void clearScreen() {
    cout << "\033[2J\033[1;1H";
}

void printLine(char c = '─', int len = 60) {
    cout << string(len, c) << "\n";
}

void pressEnter() {
    cout << "\n" << YELLOW << "  Press ENTER to continue..." << RESET;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

void printHeader(const string& title) {
    clearScreen();
    cout << BOLD << CYAN;
    printLine('═');
    int pad = (58 - (int)title.size()) / 2;
    cout << "║" << string(pad, ' ') << title << string(60-2-pad-(int)title.size(), ' ') << "║\n";
    printLine('═');
    cout << RESET << "\n";
}

int getIntInput(const string& prompt, int minVal, int maxVal) {
    int val;
    while (true) {
        cout << YELLOW << prompt << RESET;
        if (cin >> val && val >= minVal && val <= maxVal) {
            cin.ignore();
            return val;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << RED << "  ✗ Invalid input. Enter a number between "
             << minVal << " and " << maxVal << ".\n" << RESET;
    }
}

// ─── Menu Initialisation ──────────────────────────────────────

void initMenu() {
    // STARTERS
    menu.push_back({nextMenuId++, "Samosa (2 pcs)",      "Starters", 250,  "Crispy pastry with spiced minced meat", true, 5});
    menu.push_back({nextMenuId++, "Soup of the Day",     "Starters", 350,  "Ask your waiter for today's selection",  true, 8});
    menu.push_back({nextMenuId++, "Chicken Wings",       "Starters", 650,  "6 pcs, served with garlic dip",          true, 12});
    menu.push_back({nextMenuId++, "Bruschetta",          "Starters", 400,  "Toasted bread, tomatoes, basil",         true, 7});
    // MAINS
    menu.push_back({nextMenuId++, "Nyama Choma (500g)",  "Mains",    1200, "Char-grilled beef, kachumbari",          true, 25});
    menu.push_back({nextMenuId++, "Grilled Tilapia",     "Mains",    900,  "Whole tilapia, chips & salad",           true, 20});
    menu.push_back({nextMenuId++, "Chicken Biryani",     "Mains",    850,  "Fragrant basmati, spiced chicken",       true, 20});
    menu.push_back({nextMenuId++, "Beef Burger",         "Mains",    750,  "200g patty, cheese, fries included",     true, 15});
    menu.push_back({nextMenuId++, "Veggie Pasta",        "Mains",    650,  "Penne, marinara, seasonal vegetables",   true, 12});
    menu.push_back({nextMenuId++, "Lamb Chops (3 pcs)",  "Mains",    1400, "Herb-marinated, served with mash",       true, 22});
    // DESSERTS
    menu.push_back({nextMenuId++, "Chocolate Lava Cake", "Desserts", 550,  "Warm, gooey center, vanilla ice cream",  true, 10});
    menu.push_back({nextMenuId++, "Fruit Salad",         "Desserts", 350,  "Seasonal fresh fruits, honey drizzle",   true, 5});
    menu.push_back({nextMenuId++, "Cheesecake Slice",    "Desserts", 450,  "New York style, berry compote",          true, 5});
    // DRINKS
    menu.push_back({nextMenuId++, "Fresh Juice",         "Drinks",   200,  "Mango, Orange, Passion or Watermelon",   true, 3});
    menu.push_back({nextMenuId++, "Soft Drink (330ml)",  "Drinks",   150,  "Coke, Sprite, Fanta",                    true, 1});
    menu.push_back({nextMenuId++, "Mineral Water",       "Drinks",   100,  "Still or Sparkling",                     true, 1});
    menu.push_back({nextMenuId++, "Kenyan Coffee",       "Drinks",   250,  "Freshly brewed, milk on the side",       true, 5});
    menu.push_back({nextMenuId++, "Masala Chai",         "Drinks",   180,  "Spiced tea, full cream milk",             true, 5});
}

// ─── Display Menu ─────────────────────────────────────────────

void displayMenu(const string& filterCategory = "") {
    printHeader("📋  OUR MENU");

    vector<string> categories;
    for (auto& item : menu) {
        if (find(categories.begin(), categories.end(), item.category) == categories.end())
            categories.push_back(item.category);
    }

    for (auto& cat : categories) {
        if (!filterCategory.empty() && filterCategory != cat) continue;

        cout << BOLD << MAGENTA << "  ┌─ " << cat << " ─";
        cout << string(max(0, 50 - (int)cat.size()), '─') << "┐\n" << RESET;

        for (auto& item : menu) {
            if (item.category != cat) continue;
            if (!filterCategory.empty() && !item.available) continue;

            string avail = item.available ? GREEN + string("✓") : RED + string("✗");
            cout << "  │ " << CYAN << setw(3) << item.id << RESET
                 << "  " << avail << RESET
                 << "  " << BOLD << left << setw(22) << item.name << RESET
                 << "  " << WHITE << left << setw(25) << item.description.substr(0, 24) << RESET
                 << "  " << GREEN << "KSh " << right << setw(6) << fixed << setprecision(0) << item.price << RESET
                 << "\n";
        }
        cout << BOLD << MAGENTA << "  └" << string(56, '─') << "┘\n\n" << RESET;
    }
}

// ─── Customer: Place New Order ────────────────────────────────

void placeOrder() {
    printHeader("🛒  PLACE NEW ORDER");

    string name;
    cout << YELLOW << "  Customer Name: " << RESET;
    getline(cin, name);
    if (name.empty()) name = "Guest";

    int table = getIntInput("  Table Number (1-20): ", 1, 20);

    Order order;
    order.id          = nextOrderId++;
    order.customerName = name;
    order.tableNumber = table;
    order.status      = OrderStatus::PENDING;
    order.createdAt   = time(nullptr);

    while (true) {
        displayMenu();
        cout << YELLOW << "  Enter Item ID to add (0 = done): " << RESET;
        int itemId;
        cin >> itemId;
        cin.ignore();

        if (itemId == 0) break;

        auto it = find_if(menu.begin(), menu.end(),
                          [itemId](const MenuItem& m){ return m.id == itemId; });

        if (it == menu.end()) {
            cout << RED << "  ✗ Item not found.\n" << RESET;
            continue;
        }
        if (!it->available) {
            cout << RED << "  ✗ Sorry, " << it->name << " is currently unavailable.\n" << RESET;
            continue;
        }

        int qty = getIntInput("  Quantity: ", 1, 20);

        cout << YELLOW << "  Special request (leave blank for none): " << RESET;
        string req;
        getline(cin, req);

        // Check if already in order
        bool found = false;
        for (auto& oi : order.items) {
            if (oi.item.id == itemId) {
                oi.quantity += qty;
                if (!req.empty()) oi.specialRequest = req;
                found = true;
                break;
            }
        }
        if (!found) {
            order.items.push_back({*it, qty, req});
        }

        cout << GREEN << "  ✓ Added: " << qty << "x " << it->name << "\n" << RESET;
    }

    if (order.items.empty()) {
        cout << YELLOW << "\n  No items added. Order cancelled.\n" << RESET;
        pressEnter();
        return;
    }

    // Show order summary before confirming
    cout << "\n" << BOLD << CYAN << "  ─── ORDER SUMMARY ───────────────────────────\n" << RESET;
    cout << "  Customer : " << BOLD << name << RESET << "\n";
    cout << "  Table    : " << BOLD << table << RESET << "\n\n";

    for (auto& oi : order.items) {
        cout << "  " << oi.quantity << "x " << left << setw(22) << oi.item.name
             << " KSh " << right << setw(7) << fixed << setprecision(2) << oi.subtotal();
        if (!oi.specialRequest.empty())
            cout << "  " << YELLOW << "[" << oi.specialRequest << "]" << RESET;
        cout << "\n";
    }

    cout << "\n";
    printLine();
    cout << "  Subtotal      : KSh " << right << setw(10) << fixed << setprecision(2) << order.subtotal() << "\n";
    cout << "  VAT (16%)     : KSh " << right << setw(10) << order.tax()     << "\n";
    cout << "  Service (10%) : KSh " << right << setw(10) << order.service() << "\n";
    printLine();
    cout << BOLD << GREEN
         << "  TOTAL         : KSh " << right << setw(10) << order.total() << "\n"
         << RESET;
    printLine();

    cout << YELLOW << "\n  Confirm order? (y/n): " << RESET;
    char confirm;
    cin >> confirm;
    cin.ignore();

    if (tolower(confirm) == 'y') {
        order.status = OrderStatus::CONFIRMED;
        orders.push_back(order);
        cout << GREEN << "\n  ✓ Order #" << order.id << " placed successfully!\n"
             << "  Estimated wait: ~" << 25 << " minutes.\n" << RESET;
    } else {
        cout << YELLOW << "\n  Order cancelled.\n" << RESET;
    }

    pressEnter();
}

// ─── View Order Status ────────────────────────────────────────

void viewOrderStatus() {
    printHeader("📦  ORDER STATUS");

    if (orders.empty()) {
        cout << YELLOW << "  No orders yet.\n" << RESET;
        pressEnter();
        return;
    }

    cout << YELLOW << "  Enter Order ID (0 = view all): " << RESET;
    int id;
    cin >> id;
    cin.ignore();

    bool found = false;
    for (auto& o : orders) {
        if (id != 0 && o.id != id) continue;
        found = true;

        cout << "\n  " << BOLD << CYAN << "Order #" << o.id << RESET
             << "  Table " << o.tableNumber
             << "  |  " << o.customerName
             << "  |  " << o.createdAtStr()
             << "  |  " << statusToString(o.status) << "\n";

        for (auto& oi : o.items) {
            cout << "    · " << oi.quantity << "x " << left << setw(22) << oi.item.name
                 << "  KSh " << right << setw(8) << fixed << setprecision(2) << oi.subtotal() << "\n";
        }
        cout << "    " << BOLD << "TOTAL: KSh " << o.total() << RESET << "\n";
        printLine('-', 55);
    }

    if (!found) cout << RED << "  Order #" << id << " not found.\n" << RESET;
    pressEnter();
}

// ─── Print Bill / Receipt ─────────────────────────────────────

void printBill() {
    printHeader("🧾  PRINT BILL");

    cout << YELLOW << "  Enter Order ID: " << RESET;
    int id;
    cin >> id;
    cin.ignore();

    Order* target = nullptr;
    for (auto& o : orders) {
        if (o.id == id) { target = &o; break; }
    }

    if (!target) {
        cout << RED << "  ✗ Order not found.\n" << RESET;
        pressEnter();
        return;
    }

    cout << "\n";
    printLine('═');
    cout << BOLD << setw(38) << "🍽  KARIBU RESTAURANT\n" << RESET;
    cout << setw(40) << "Nairobi, Kenya | +254 700 000 000\n";
    cout << setw(38) << "www.kariburestaurant.co.ke\n";
    printLine('═');
    cout << "  Receipt No : " << BOLD << "#" << target->id   << RESET << "\n";
    cout << "  Customer   : " << BOLD << target->customerName << RESET << "\n";
    cout << "  Table      : " << BOLD << target->tableNumber  << RESET << "\n";
    cout << "  Date/Time  : " << BOLD << target->createdAtStr()<< RESET << "\n";
    printLine();
    cout << BOLD << left << setw(24) << "  ITEM"
         << setw(8) << "QTY"
         << setw(10) << "PRICE"
         << right << setw(12) << "SUBTOTAL\n" << RESET;
    printLine();

    for (auto& oi : target->items) {
        cout << "  " << left << setw(23) << oi.item.name
             << setw(8) << oi.quantity
             << "KSh " << setw(8) << fixed << setprecision(2) << oi.item.price
             << "KSh " << right << setw(8) << oi.subtotal() << "\n";
        if (!oi.specialRequest.empty())
            cout << "    " << YELLOW << "* " << oi.specialRequest << RESET << "\n";
    }

    printLine();
    cout << right;
    cout << setw(52) << "Subtotal : KSh " << setw(10) << target->subtotal() << "\n";
    cout << setw(52) << "VAT 16%  : KSh " << setw(10) << target->tax()     << "\n";
    cout << setw(52) << "Service  : KSh " << setw(10) << target->service() << "\n";
    printLine('═');
    cout << BOLD << GREEN
         << setw(52) << "TOTAL    : KSh " << setw(10) << target->total()
         << RESET << "\n";
    printLine('═');
    cout << "\n" << setw(40) << "Thank you for dining with us!\n";
    cout << setw(36) << "Asante sana! 🙏\n\n";

    if (target->status != OrderStatus::SERVED)
        target->status = OrderStatus::SERVED;

    pressEnter();
}

// ─── Admin Panel ──────────────────────────────────────────────

void updateOrderStatus() {
    printHeader("🔄  UPDATE ORDER STATUS");

    cout << YELLOW << "  Enter Order ID: " << RESET;
    int id; cin >> id; cin.ignore();

    Order* target = nullptr;
    for (auto& o : orders) if (o.id == id) { target = &o; break; }

    if (!target) { cout << RED << "  ✗ Not found.\n" << RESET; pressEnter(); return; }

    cout << "  Current status: " << statusToString(target->status) << "\n\n";
    cout << "  1. CONFIRMED\n  2. PREPARING\n  3. READY\n  4. SERVED\n  5. CANCELLED\n";
    int choice = getIntInput("  Select new status: ", 1, 5);

    OrderStatus newStatus[] = {
        OrderStatus::CONFIRMED, OrderStatus::PREPARING,
        OrderStatus::READY, OrderStatus::SERVED, OrderStatus::CANCELLED
    };
    target->status = newStatus[choice - 1];
    cout << GREEN << "  ✓ Status updated to " << statusToString(target->status) << "\n" << RESET;
    pressEnter();
}

void adminAddMenuItem() {
    printHeader("➕  ADD MENU ITEM");

    MenuItem item;
    item.id        = nextMenuId++;
    item.available = true;

    cout << YELLOW << "  Name: "        << RESET; getline(cin, item.name);
    cout << YELLOW << "  Category (Starters/Mains/Desserts/Drinks): " << RESET;
    getline(cin, item.category);
    cout << YELLOW << "  Description: " << RESET; getline(cin, item.description);
    cout << YELLOW << "  Price (KSh): " << RESET; cin >> item.price;
    item.prepTime = getIntInput("  Prep time (min): ", 1, 60);
    cin.ignore();

    menu.push_back(item);
    cout << GREEN << "\n  ✓ " << item.name << " added with ID #" << item.id << "\n" << RESET;
    pressEnter();
}

void adminToggleAvailability() {
    printHeader("🔀  TOGGLE ITEM AVAILABILITY");
    displayMenu();
    int id = getIntInput("  Enter Item ID: ", 1, nextMenuId);
    for (auto& item : menu) {
        if (item.id == id) {
            item.available = !item.available;
            cout << (item.available ? GREEN : RED)
                 << "  " << item.name << " is now "
                 << (item.available ? "AVAILABLE ✓" : "UNAVAILABLE ✗")
                 << RESET << "\n";
            pressEnter();
            return;
        }
    }
    cout << RED << "  ✗ Item not found.\n" << RESET;
    pressEnter();
}

void adminSalesReport() {
    printHeader("📊  SALES REPORT");

    if (orders.empty()) {
        cout << YELLOW << "  No orders to report.\n" << RESET;
        pressEnter();
        return;
    }

    double totalRevenue = 0;
    int    totalOrders  = 0;
    map<string, int> popularity;

    for (auto& o : orders) {
        if (o.status == OrderStatus::CANCELLED) continue;
        totalRevenue += o.total();
        totalOrders++;
        for (auto& oi : o.items)
            popularity[oi.item.name] += oi.quantity;
    }

    cout << BOLD << GREEN
         << "  Total Revenue  : KSh " << fixed << setprecision(2) << totalRevenue << "\n"
         << RESET;
    cout << "  Total Orders   : " << totalOrders << "\n";
    cout << "  Average Bill   : KSh "
         << (totalOrders ? totalRevenue / totalOrders : 0) << "\n\n";

    cout << BOLD << CYAN << "  ─── Most Popular Items ────────────────\n" << RESET;

    // Sort by popularity
    vector<pair<string,int>> sorted(popularity.begin(), popularity.end());
    sort(sorted.begin(), sorted.end(), [](auto& a, auto& b){ return a.second > b.second; });

    int rank = 1;
    for (auto& [name, qty] : sorted) {
        if (rank > 5) break;
        cout << "  " << rank++ << ". " << left << setw(25) << name
             << "  " << qty << " orders\n";
    }

    pressEnter();
}

void adminPanel() {
    printHeader("🔐  ADMIN LOGIN");
    cout << YELLOW << "  Enter Admin PIN: " << RESET;
    string pin;
    cin >> pin;
    cin.ignore();

    if (pin != ADMIN_PIN) {
        cout << RED << "\n  ✗ Incorrect PIN. Access denied.\n" << RESET;
        pressEnter();
        return;
    }

    while (true) {
        printHeader("⚙️   ADMIN PANEL");
        cout << "  1. Update Order Status\n"
                "  2. Add Menu Item\n"
                "  3. Toggle Item Availability\n"
                "  4. View Sales Report\n"
                "  5. View All Orders\n"
                "  0. Back to Main Menu\n\n";

        int choice = getIntInput("  Select: ", 0, 5);

        switch (choice) {
            case 1: updateOrderStatus();   break;
            case 2: adminAddMenuItem();    break;
            case 3: adminToggleAvailability(); break;
            case 4: adminSalesReport();    break;
            case 5: {
                printHeader("📋  ALL ORDERS");
                if (orders.empty()) { cout << YELLOW << "  No orders.\n" << RESET; pressEnter(); break; }
                for (auto& o : orders) {
                    cout << "  #" << o.id << " | Table " << o.tableNumber
                         << " | " << o.customerName
                         << " | " << o.createdAtStr()
                         << " | " << statusToString(o.status)
                         << " | KSh " << fixed << setprecision(2) << o.total() << "\n";
                }
                pressEnter();
                break;
            }
            case 0: return;
        }
    }
}

// ─── Main Menu ────────────────────────────────────────────────

void mainMenu() {
    while (true) {
        printHeader("🍽   KARIBU RESTAURANT — ORDERING SYSTEM");

        cout << BOLD << CYAN
             << "  Welcome! Please select an option:\n\n" << RESET;
        cout << "  " << CYAN << "1" << RESET << "  📋  View Menu\n";
        cout << "  " << CYAN << "2" << RESET << "  🛒  Place Order\n";
        cout << "  " << CYAN << "3" << RESET << "  📦  Check Order Status\n";
        cout << "  " << CYAN << "4" << RESET << "  🧾  Print Bill\n";
        cout << "  " << CYAN << "5" << RESET << "  ⚙️   Admin Panel\n";
        cout << "  " << CYAN << "0" << RESET << "  🚪  Exit\n\n";

        int choice = getIntInput("  Enter choice: ", 0, 5);

        switch (choice) {
            case 1:
                displayMenu();
                pressEnter();
                break;
            case 2: placeOrder();       break;
            case 3: viewOrderStatus();  break;
            case 4: printBill();        break;
            case 5: adminPanel();       break;
            case 0:
                clearScreen();
                cout << BOLD << GREEN
                     << "\n  Asante sana! Thank you for using Karibu Restaurant System.\n\n"
                     << RESET;
                return;
        }
    }
}

// ─── Entry Point ──────────────────────────────────────────────

int main() {
    initMenu();
    mainMenu();
    return 0;
}