#include <bits/stdc++.h>
using namespace std;
#define ll long long
#define pii pair<int, int>
#define lowbit(x) (x & -x)
#define int long long

void solve() {
    // cout<<endl;
    // cout<<"------------------------------"<<endl;
}

signed main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.tie(nullptr);
    int t = 1;
    // cin>>t;
    while (t--) {
        solve();
    }
    return 0;
}


//hot 100
class Solution {
public:
    vector<int> twoSum(vector<int>& nums, int target) {
        unordered_map<int, int> ret;
        int n = nums.size();
        for (int i = 0; i < n; ++i) {
            if (ret.count(target - nums[i])) {
                return {ret[target - nums[i]], i};
            }
            ret[nums[i]] = i;
        }
        return {-1, -1};
    }
};
