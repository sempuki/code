#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

using namespace std;

const static std::string utterance = "neigh";

int main() {
    string input;
    cin >> input;

    size_t count = 0;
    vector<string> voices;

    for (auto ch : input) {
        switch (ch) {
            case 'n':
                voices.push_back(utterance.substr(1));
                break;

            case 'e':
                for (string& voice : voices) {
                    if (voice.front() == 'e') {
                        voice.erase(0, 1);
                        break;
                    }
                }
                break;

            case 'i':
                for (string& voice : voices) {
                    if (voice.front() == 'i') {
                        voice.erase(0, 1);
                        break;
                    }
                }
                break;

            case 'g':
                for (string& voice : voices) {
                    if (voice.front() == 'g') {
                        voice.erase(0, 1);
                        break;
                    }
                }
                break;

            case 'h':
                for (string& voice : voices) {
                    if (voice.front() == 'h') {
                        voice.erase(0, 1);
                        break;
                    }
                }
                break;

            default:
                cout << "Invalid" << endl;
                return -1;
        }

        voices.erase(
            remove_if(voices.begin(), voices.end(),
                      [](const string& voice) { return voice.empty(); }),
            voices.end());

        count = max(count, voices.size());
    }

    cout << count << endl;
}
