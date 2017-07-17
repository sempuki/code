#include <iostream>
#include <memory>
#include <vector>
using namespace std;

struct Node {
    size_t count = 1;
    shared_ptr<Node> next;
};

Node* last(Node* node) {
    if (node) {
        while (node->next) {
            node = node->next.get();
        }
    }
    return node;
}

int main() {
    std::string command, eol;
    size_t n, q, i, j;
    cin >> n >> q;

    vector<Node> nodes;
    nodes.resize(n);

    while (q--) {
        cin >> command;
        if (command == "Q") {
            cin >> i;
            cout << last(&nodes[i - 1])->count << '\n';
        } else if (command == "M") {
            cin >> i >> j;
            auto lasti = last(&nodes[i - 1]);
            auto lastj = last(&nodes[j - 1]);
            auto next = std::make_shared<Node>();
            next->count = lasti->count + lastj->count;
            lasti->next = lastj->next = next;
        }
    }

    return 0;
}
