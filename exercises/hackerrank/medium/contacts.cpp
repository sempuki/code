#include <array>
#include <bitset>
#include <iostream>
#include <memory>
using namespace std;

class Contacts {
   public:
    void insert(const std::string& name) {
        std::unique_ptr<Node>* node = &root_;

        for (auto ch : name) {
            if (*node == nullptr) {
                *node = std::make_unique<Node>();
            }
            (*node)->prefix[ch - 'a'] += 1;
            node = &(*node)->next[ch - 'a'];
        }
    }

    size_t lookup(const std::string& partial) {
        size_t count = 0;
        const Node* node = root_.get();
        for (auto ch : partial) {
            if (node) {
                count = node->prefix[ch - 'a'];
                node = node->next[ch - 'a'].get();
            } else {
                count = 0;
                break;
            }
        }

        return count;
    }

   private:
    struct Node {
        std::array<std::unique_ptr<Node>, 26> next;
        std::array<size_t, 26> prefix;
    };

    std::unique_ptr<Node> root_;
};

int main() {
    Contacts contacts;
    size_t n;
    cin >> n;
    while (n--) {
        std::string command, parameter;
        cin >> command >> parameter;
        if (command == "add") {
            contacts.insert(parameter);
        } else if (command == "find") {
            cout << contacts.lookup(parameter) << '\n';
        }
    }

    return 0;
}
