#include <bits/stdc++.h>
using namespace std;    

// 简洁的模2除法实现
string mod2Divide(string dividend, string divisor) {
    int n = dividend.length();
    int m = divisor.length();
    
    string temp = dividend;
    for (int i = 0; i <= n - m; i++) {
        if (temp[i] == '1') {
            for (int j = 0; j < m; j++) {
                temp[i + j] = temp[i + j] == divisor[j] ? '0' : '1';
            }
        }
    }
    
    return temp.substr(n - m + 1);
}

int main() {
    string data ;
    string poly ;
    cin>> data >> poly;
    
    cout << "余数: " << mod2Divide(data, poly) << endl;
    return 0;
}