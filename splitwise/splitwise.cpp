
#include <bits/stdc++.h>

using namespace std;

// Money is stored in paise.
// Rs. 100.50 = 10050 paise.
using Money = long long;

Money rupees(long long amount) {
    return amount * 100;
}

string formatMoney(Money amount) {
    ostringstream out;

    if (amount < 0) {
        out << "-";
        amount = -amount;
    }

    out << "Rs. " << amount / 100
        << "." << setw(2) << setfill('0') << amount % 100;

    return out.str();
}

// ==========================================
// 1. USER
// ==========================================

class User {
public:
    string userId;
    string name;
    string email;

    User(string id, string name, string email)
        : userId(id), name(name), email(email) {}

    void display() const {
        cout << name << " (" << userId << ")" << endl;
    }
};

// ==========================================
// 2. SPLIT
// ==========================================

class Split {
public:
    User* user;
    Money amount;

    Split(User* user, Money amount)
        : user(user), amount(amount) {}
};

// ==========================================
// 3. SPLIT DETAILS
// ==========================================

struct SplitDetails {

    // Exact amounts are stored in paise.
    unordered_map<string, Money> exactAmounts;

    // Percentages are stored in basis points.
    // 50% = 5000 basis points.
    // 30% = 3000 basis points.
    unordered_map<string, int> percentages;
};

// ==========================================
// 4. SPLIT STRATEGY (ABSTRACT CLASS)
// ==========================================

class SplitStrategy {

public:
    virtual vector<Split> calculateSplits(
        Money totalAmount,
        const vector<User*>& participants,
        const SplitDetails& details
    ) const = 0;

    virtual ~SplitStrategy() = default;
};

// ==========================================
// 5. EQUAL SPLIT
// ==========================================

class EqualSplit : public SplitStrategy {

public:

    vector<Split> calculateSplits(
        Money totalAmount,
        const vector<User*>& participants,
        const SplitDetails& details
    ) const override {

        if (participants.empty()) {
            throw invalid_argument(
                "At least one participant is required"
            );
        }

        vector<Split> splits;

        Money base = totalAmount / participants.size();

        Money allocated = 0;

        for (int i = 0; i < participants.size(); i++) {

            Money share;

            // Assign remaining paise to the last participant.
            if (i == participants.size() - 1) {
                share = totalAmount - allocated;
            }
            else {
                share = base;
                allocated += share;
            }

            splits.emplace_back(participants[i], share);
        }

        return splits;
    }
};

// ==========================================
// 6. EXACT SPLIT
// ==========================================

class ExactSplit : public SplitStrategy {

public:

    vector<Split> calculateSplits(
        Money totalAmount,
        const vector<User*>& participants,
        const SplitDetails& details
    ) const override {

        if (details.exactAmounts.size() != participants.size()) {
            throw invalid_argument(
                "Provide an exact amount for every participant"
            );
        }

        vector<Split> splits;

        Money total = 0;

        for (User* user : participants) {

            auto it = details.exactAmounts.find(user->userId);

            if (it == details.exactAmounts.end()) {
                throw invalid_argument(
                    "Missing exact amount for participant"
                );
            }

            Money amount = it->second;

            if (amount < 0) {
                throw invalid_argument(
                    "Split amount cannot be negative"
                );
            }

            splits.emplace_back(user, amount);

            total += amount;
        }

        if (total != totalAmount) {
            throw invalid_argument(
                "Exact split amounts must equal expense amount"
            );
        }

        return splits;
    }
};

// ==========================================
// 7. PERCENTAGE SPLIT
// ==========================================

class PercentageSplit : public SplitStrategy {

public:

    vector<Split> calculateSplits(
        Money totalAmount,
        const vector<User*>& participants,
        const SplitDetails& details
    ) const override {

        if (details.percentages.size() != participants.size()) {
            throw invalid_argument(
                "Provide percentage for every participant"
            );
        }

        int totalPercentage = 0;

        for (User* user : participants) {

            auto it = details.percentages.find(user->userId);

            if (it == details.percentages.end()) {
                throw invalid_argument(
                    "Missing percentage for participant"
                );
            }

            if (it->second < 0) {
                throw invalid_argument(
                    "Percentage cannot be negative"
                );
            }

            totalPercentage += it->second;
        }

        if (totalPercentage != 10000) {
            throw invalid_argument(
                "Percentages must add up to 100%"
            );
        }

        vector<Split> splits;

        Money allocated = 0;

        for (int i = 0; i < participants.size(); i++) {

            User* user = participants[i];

            Money share;

            if (i == participants.size() - 1) {
                share = totalAmount - allocated;
            }
            else {

                int percentage = details.percentages.at(
                    user->userId
                );

                // Round to the nearest paise.
                share = (totalAmount * percentage + 5000) / 10000;

                allocated += share;
            }

            splits.emplace_back(user, share);
        }

        return splits;
    }
};

// ==========================================
// 8. EXPENSE
// ==========================================

class Group;

class Expense {

public:
    string expenseId;
    string description;
    Money amount;

    User* paidBy;

    vector<Split> splits;

    Group* group;

    Expense(
        string id,
        string description,
        Money amount,
        User* paidBy,
        vector<Split> splits,
        Group* group
    )
        : expenseId(id),
          description(description),
          amount(amount),
          paidBy(paidBy),
          splits(move(splits)),
          group(group) {}
};

// ==========================================
// 9. GROUP
// ==========================================

class Group {

public:
    string groupId;
    string name;

    unordered_map<string, User*> members;

    vector<Expense*> expenses;

    Group(string id, string name)
        : groupId(id), name(name) {}

    void addMember(User* user) {
        members[user->userId] = user;
    }
};

// ==========================================
// 10. BALANCE SHEET
// ==========================================

class BalanceSheet {

private:

    // balances[debtor][creditor] = amount
    //
    // Example:
    // balances["U1"]["U2"] = 50000
    //
    // U1 owes U2 Rs. 500.

    unordered_map<
        string,
        unordered_map<string, Money>
    > balances;

public:

    void addDebt(
        User* debtor,
        User* creditor,
        Money amount
    ) {

        if (amount < 0) {
            throw invalid_argument("Debt cannot be negative");
        }

        if (debtor->userId == creditor->userId || amount == 0) {
            return;
        }

        string debtorId = debtor->userId;
        string creditorId = creditor->userId;

        Money current = balances[debtorId][creditorId];

        Money reverse = balances[creditorId][debtorId];

        // Include previous debt in the same direction.
        Money net = current + amount - reverse;

        if (net >= 0) {

            balances[debtorId][creditorId] = net;

            balances[creditorId][debtorId] = 0;
        }
        else {

            balances[creditorId][debtorId] = -net;

            balances[debtorId][creditorId] = 0;
        }
    }

    Money getBalance(
        const string& debtorId,
        const string& creditorId
    ) const {

        auto it = balances.find(debtorId);

        if (it == balances.end()) {
            return 0;
        }

        auto jt = it->second.find(creditorId);

        if (jt == it->second.end()) {
            return 0;
        }

        return jt->second;
    }

    void showBalances() const {

        cout << "\n--- Outstanding Balances ---\n";

        for (const auto& debtor : balances) {

            for (const auto& creditor : debtor.second) {

                if (creditor.second > 0) {

                    cout << debtor.first
                         << " owes "
                         << creditor.first
                         << ": "
                         << formatMoney(creditor.second)
                         << endl;
                }
            }
        }
    }

    void settle(
        const string& debtorId,
        const string& creditorId,
        Money amount
    ) {

        Money current = getBalance(debtorId, creditorId);

        if (amount <= 0 || amount > current) {
            throw invalid_argument(
                "Invalid settlement amount"
            );
        }

        balances[debtorId][creditorId] = current - amount;
    }

    const auto& getBalances() const {
        return balances;
    }
};

// ==========================================
// 11. EXPENSE MANAGER
// ==========================================

class ExpenseManager {

private:

    unordered_map<string, unique_ptr<User>> users;

    unordered_map<string, unique_ptr<Group>> groups;

    vector<unique_ptr<Expense>> expenses;

    unordered_set<string> expenseIds;

    BalanceSheet balanceSheet;

public:

    // --------------------------------------
    // ADD USER
    // --------------------------------------

    User* addUser(
        string id,
        string name,
        string email
    ) {

        if (users.count(id)) {
            throw invalid_argument("User already exists");
        }

        auto user = make_unique<User>(
            id, name, email
        );

        User* userPtr = user.get();

        users[id] = move(user);

        return userPtr;
    }

    // --------------------------------------
    // CREATE GROUP
    // --------------------------------------

    Group* createGroup(
        string id,
        string name
    ) {

        if (groups.count(id)) {
            throw invalid_argument("Group already exists");
        }

        auto group = make_unique<Group>(id, name);

        Group* groupPtr = group.get();

        groups[id] = move(group);

        return groupPtr;
    }

    // --------------------------------------
    // ADD EXPENSE
    // --------------------------------------

    Expense* addExpense(
        string expenseId,
        string description,
        Money amount,
        User* paidBy,
        const vector<User*>& participants,
        const SplitStrategy& strategy,
        const SplitDetails& details = {},
        Group* group = nullptr
    ) {

        if (amount <= 0) {
            throw invalid_argument("Expense must be positive");
        }

        if (expenseIds.count(expenseId)) {
            throw invalid_argument("Expense ID already exists");
        }

        if (participants.empty()) {
            throw invalid_argument("Participants cannot be empty");
        }

        unordered_set<string> participantIds;

        bool payerFound = false;

        for (User* user : participants) {

            if (user == nullptr ||
                !users.count(user->userId) ||
                users.at(user->userId).get() != user) {

                throw invalid_argument("Unknown participant");
            }

            if (!participantIds.insert(user->userId).second) {
                throw invalid_argument("Duplicate participant");
            }

            if (user == paidBy) {
                payerFound = true;
            }
        }

        if (paidBy == nullptr ||
            !users.count(paidBy->userId) ||
            users.at(paidBy->userId).get() != paidBy) {

            throw invalid_argument("Unknown payer");
        }

        if (!payerFound) {
            throw invalid_argument(
                "Payer must be a participant"
            );
        }

        if (group != nullptr) {

            auto it = groups.find(group->groupId);

            if (it == groups.end() ||
                it->second.get() != group) {

                throw invalid_argument("Unknown group");
            }

            for (User* user : participants) {

                if (!group->members.count(user->userId)) {
                    throw invalid_argument(
                        "Participant is not a group member"
                    );
                }
            }
        }

        // Strategy Pattern:
        // The manager does not know which split algorithm
        // is being used.

        vector<Split> splits = strategy.calculateSplits(
            amount,
            participants,
            details
        );

        Money calculatedTotal = 0;

        for (const Split& split : splits) {
            calculatedTotal += split.amount;
        }

        if (calculatedTotal != amount) {
            throw invalid_argument(
                "Splits do not equal expense amount"
            );
        }

        auto expense = make_unique<Expense>(
            expenseId,
            description,
            amount,
            paidBy,
            splits,
            group
        );

        Expense* expensePtr = expense.get();

        // Update outstanding balances.

        for (const Split& split : splits) {

            if (split.user != paidBy) {

                balanceSheet.addDebt(
                    split.user,
                    paidBy,
                    split.amount
                );
            }
        }

        expenses.push_back(move(expense));

        expenseIds.insert(expenseId);

        if (group != nullptr) {
            group->expenses.push_back(expensePtr);
        }

        cout << "\nExpense added: "
             << description
             << " - "
             << formatMoney(amount)
             << endl;

        return expensePtr;
    }

    // --------------------------------------
    // SETTLE DEBT
    // --------------------------------------

    void settleDebt(
        string debtorId,
        string creditorId,
        Money amount
    ) {

        balanceSheet.settle(
            debtorId,
            creditorId,
            amount
        );

        cout << "\nSettlement successful: "
             << formatMoney(amount)
             << endl;
    }

    // --------------------------------------
    // SHOW BALANCES
    // --------------------------------------

    void showBalances() const {
        balanceSheet.showBalances();
    }

    // --------------------------------------
    // DEBT SIMPLIFICATION
    // --------------------------------------

    vector<tuple<string, string, Money>> simplifyDebts(
        vector<string> userIds = {}
    ) {

        if (userIds.empty()) {

            for (const auto& user : users) {
                userIds.push_back(user.first);
            }
        }

        unordered_set<string> selected;

        for (const string& id : userIds) {

            if (!users.count(id)) {
                throw invalid_argument("Unknown user");
            }

            if (!selected.insert(id).second) {
                throw invalid_argument("Duplicate user ID");
            }
        }

        unordered_map<string, Money> net;

        // Step 1: Calculate net balances.

        for (const string& debtor : userIds) {

            for (const auto& entry :
                 balanceSheet.getBalances().find(debtor) !=
                     balanceSheet.getBalances().end()
                 ? balanceSheet.getBalances().at(debtor)
                 : unordered_map<string, Money>{}) {

                string creditor = entry.first;
                Money amount = entry.second;

                if (selected.count(creditor)) {

                    net[debtor] -= amount;
                    net[creditor] += amount;
                }
            }
        }

        // Step 2: Separate debtors and creditors.

        vector<pair<string, Money>> debtors;
        vector<pair<string, Money>> creditors;

        for (const string& id : userIds) {

            if (net[id] < 0) {
                debtors.push_back({id, -net[id]});
            }
            else if (net[id] > 0) {
                creditors.push_back({id, net[id]});
            }
        }

        // Step 3: Match debtors and creditors.

        vector<tuple<string, string, Money>> transactions;

        int i = 0;
        int j = 0;

        while (i < debtors.size() &&
               j < creditors.size()) {

            Money amount = min(
                debtors[i].second,
                creditors[j].second
            );

            transactions.push_back({
                debtors[i].first,
                creditors[j].first,
                amount
            });

            debtors[i].second -= amount;
            creditors[j].second -= amount;

            if (debtors[i].second == 0) {
                i++;
            }

            if (creditors[j].second == 0) {
                j++;
            }
        }

        return transactions;
    }

    BalanceSheet& getBalanceSheet() {
        return balanceSheet;
    }
};

// ==========================================
// 12. MAIN FUNCTION
// ==========================================

int main() {

    ExpenseManager manager;

    // Create users.

    User* anish = manager.addUser(
        "U1", "Anish", "anish@example.com"
    );

    User* rahul = manager.addUser(
        "U2", "Rahul", "rahul@example.com"
    );

    User* priya = manager.addUser(
        "U3", "Priya", "priya@example.com"
    );

    User* amit = manager.addUser(
        "U4", "Amit", "amit@example.com"
    );

    vector<User*> users = {
        anish, rahul, priya, amit
    };

    // Create group.

    Group* group = manager.createGroup(
        "G1", "Goa Trip"
    );

    for (User* user : users) {
        group->addMember(user);
    }

    // --------------------------------------
    // EXPENSE 1: EQUAL SPLIT
    // --------------------------------------

    EqualSplit equalSplit;

    manager.addExpense(
        "E1",
        "Hotel",
        rupees(4000),
        anish,
        users,
        equalSplit,
        {},
        group
    );

    // --------------------------------------
    // EXPENSE 2: EXACT SPLIT
    // --------------------------------------

    ExactSplit exactSplit;

    SplitDetails exactDetails;

    exactDetails.exactAmounts = {
        {"U1", rupees(1000)},
        {"U2", rupees(1000)},
        {"U3", rupees(1000)}
    };

    manager.addExpense(
        "E2",
        "Dinner",
        rupees(3000),
        rahul,
        {anish, rahul, priya},
        exactSplit,
        exactDetails,
        group
    );

    // --------------------------------------
    // EXPENSE 3: PERCENTAGE SPLIT
    // --------------------------------------

    PercentageSplit percentageSplit;

    SplitDetails percentageDetails;

    percentageDetails.percentages = {
        {"U1", 5000},  // 50%
        {"U2", 3000},  // 30%
        {"U3", 2000}   // 20%
    };

    manager.addExpense(
        "E3",
        "Cab",
        rupees(1000),
        priya,
        {anish, rahul, priya},
        percentageSplit,
        percentageDetails,
        group
    );

    // --------------------------------------
    // SHOW BALANCES
    // --------------------------------------

    manager.showBalances();

    // --------------------------------------
    // PARTIAL SETTLEMENT
    // --------------------------------------

    manager.settleDebt(
        "U2",
        "U3",
        rupees(100)
    );

    // --------------------------------------
    // DEBT SIMPLIFICATION
    // --------------------------------------

    cout << "\n--- Simplified Transactions ---\n";

    auto transactions = manager.simplifyDebts();

    for (const auto& transaction : transactions) {

        cout << get<0>(transaction)
             << " pays "
             << get<1>(transaction)
             << ": "
             << formatMoney(get<2>(transaction))
             << endl;
    }

    return 0;
}
