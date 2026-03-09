#include <bits/stdc++.h>
using namespace std;
#define ll long long
#define pii pair<int, int>
#define lowbit(x) (x & -x)

// 模板来源 https://leetcode.cn/circle/discuss/mOr1u6/
// 根据题目用 FenwickTree<int> t(n) 或者 FenwickTree<long long> t(n) 初始化
template <typename T>
class FenwickTree {
    vector<T> tree;

public:
    // 使用下标 1 到 n
    FenwickTree(int n) : tree(n + 1) {}

    // a[i] 增加 val
    // 1 <= i <= n
    // 时间复杂度 O(log n)
    void update(int i, T val) {
        for (; i < tree.size(); i += i & -i) {
            tree[i] += val;
        }
    }

    // 求前缀和 a[1] + ... + a[i]
    // 1 <= i <= n
    // 时间复杂度 O(log n)
    T pre(int i) const {
        T res = 0;
        for (; i > 0; i &= i - 1) {
            res += tree[i];
        }
        return res;
    }

    // 求区间和 a[l] + ... + a[r]
    // 1 <= l <= r <= n
    // 时间复杂度 O(log n)
    T query(int l, int r) const {
        if (r < l) {
            return 0;
        }
        return pre(r) - pre(l - 1);
    }
};

class Solution {
public:
    long long countOperationsToEmptyArray(vector<int>& nums) {
        int n = nums.size();

        FenwickTree<int> f(n);
        vector<pair<int, int>> ret(n);
        for (int i = 0; i < n; ++i) {
            ret[i] = {nums[i], i};
        }

        sort(ret.begin(), ret.end());

        for (int i = 1; i <= n; ++i) {
            f.update(i, 1);
        }
        ll ans = 0;
        int cur = 0;
        for (int i = 0; i < n; ++i) {
            int pos = ret[i].second + 1;
            if (pos >= cur) {
                ans += 1LL * f.query(cur, pos);
            } else {
                ans += 1LL * f.query(cur, n) + f.query(1, pos);
            }
            cur = pos;
        }
        return ans;
    }
};

// void solve() {}

// int main() {
//     ios::sync_with_stdio(false);
//     cin.tie(nullptr);
//     cout.tie(nullptr);
//     int t = 1;
//     // cin>>t;
//     while (t--) {
//         solve();
//     }
//     return 0;
// }