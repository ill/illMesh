#include <iostream>

using namespace std;

void showHelp() {
    cout << "Help coming soon" << endl;
}

int main(int argc, const char * argv) {
    //process command line args
    if(argc <= 1) {
        cout << "No arguments specified" << endl;
        showHelp();
        return 1;
    }
}