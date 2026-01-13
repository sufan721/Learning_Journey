// #include <bits/stdc++.h>
// using namespace std;    

// // 简洁的模2除法实现
// string mod2Divide(string dividend, string divisor) {
//     int n = dividend.length();
//     int m = divisor.length();
    
//     string temp = dividend;
//     for (int i = 0; i <= n - m; i++) {
//         if (temp[i] == '1') {
//             for (int j = 0; j < m; j++) {
//                 temp[i + j] = temp[i + j] == divisor[j] ? '0' : '1';
//             }
//         }
//     }
    
//     return temp.substr(n - m + 1);
// }

// int main() {
//     string data ;
//     string poly ;
//     cin>> data >> poly;
    
//     cout << "余数: " << mod2Divide(data, poly) << endl;
//     return 0;
// }

// #include <bits/stdc++.h>
// using namespace std;

// int main() {
//     int n;
//     cin >> n;
//     vector<int> p(n + 1);

//     for (int i = 0; i <= n; i++) {
//         cin >> p[i];
//     }
//     vector<vector<int>> dp(n + 1, vector<int>(n + 1, 0));
//     for (int len = 2; len <= n; len++) {
//         for (int i = 1; i <= n - len + 1; i++) {
//             int j = i + len - 1;
//             dp[i][j] = INT_MAX;
//             for (int k = i; k < j; k++) {
//                 dp[i][j] = min(dp[i][j], dp[i][k] + dp[k + 1][j] + p[i - 1] * p[k] * p[j]);
//             }
//         }
//     }
//     cout << dp[1][n];
    
//     return 0;
// }

#include <bits/stdc++.h>
using namespace std;
vector<pair<string ,int>> ret;
string max_pre(string s){
    set<pair<int,string>> f;
    for(auto [t,pre]:ret){
        bool k=true;
        for(int i=0;i<pre;++i){
            if(s[i]!=t[i]){
                k=false;
            }
        }
        if(k){
            f.insert({pre,t});
        }
    }
    if(f.empty()) return "";
    return f.rbegin()->second;
}
int main(){

}