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

class Solution {
public:
    vector<vector<string>> groupAnagrams(vector<string>& strs) {
        unordered_map<string, vector<string>> ret;
        for (string x : strs) {
            string y = x;
            sort(y.begin(), y.end());
            ret[y].push_back(x);
        }
        vector<vector<string>> ans;

        for (auto [_, x] : ret) {
            ans.push_back(x);
        }

        return ans;
    }
};

class Solution {
public:
    int longestConsecutive(vector<int>& nums) {
        int ans = 0;
        unordered_set<int> ret(nums.begin(), nums.end());

        for (int x : nums) {
            if (ret.count(x - 1)) {
                continue;
            }
            int y = x + 1;
            while (ret.count(y)) {
                y++;
            }
            ans = max(ans, y - x);
            if (ans * 2 >= nums.size()) {
                break;  // 记得优化
            }
        }
        return ans;
    }
};

class Solution {
public:
    void moveZeroes(vector<int>& nums) {
        int l = 0;
        for (int& x : nums) {
            if (x) {
                swap(x, nums[l]);
                l++;
            }
        }
    }
};

class Solution {
public:
    int maxArea(vector<int>& height) {
        int ans = 0;
        int n = height.size();
        int l = 0, r = n - 1;

        while (l < r) {
            int len = r - l;
            ans = max(ans, len * min(height[l], height[r]));
            if (height[l] < height[r]) {
                l++;
            } else {
                r--;
            }
        }
        return ans;
    }
};

class Solution {
public:
    vector<vector<int>> threeSum(vector<int>& nums) {
        sort(nums.begin(), nums.end());

        set<vector<int>> cnt;
        int n = nums.size();
        for (int k = 2; k < n; k++) {
            unordered_map<int, int> ret;
            if (k + 1 < n && nums[k] == nums[k + 1]) {
                continue;
            }
            for (int i = 0; i < k; ++i) {
                if (ret.count(-nums[k] - nums[i])) {
                    cnt.insert({-nums[k] - nums[i], nums[i], nums[k]});
                }
                ret[nums[i]] = i;
            }
        }
        vector<vector<int>> ans{cnt.begin(), cnt.end()};
        return ans;
    }
};

class Solution {
public:
    int trap(vector<int>& height) {
        int n = height.size();
        vector<int> pre(n);
        vector<int> suf(n);

        pre[0] = height[0];
        for (int i = 1; i < n; ++i) {
            pre[i] = max(pre[i - 1], height[i]);
        }

        suf[n - 1] = height[n - 1];
        for (int i = n - 2; i >= 0; --i) {
            suf[i] = max(suf[i + 1], height[i]);
        }

        int ans = 0;

        for (int i = 1; i < n - 1; i++) {
            ans += max(min(suf[i], pre[i]) - height[i], 0LL);
        }
        return ans;
    }
};

class Solution {
public:
    int lengthOfLongestSubstring(string s) {
        int n = s.size();
        int ans = 0, l = 0;
        unordered_map<char, int> ret;

        for (int r = 0; r < n; r++) {
            char x = s[r];
            ret[x]++;
            while (ret[x] > 1) {
                ret[s[l++]]--;
            }

            ans = max(ans, l - r + 1);
        }
        return ans;
    }
};


class Solution {
public:
    vector<int> findAnagrams(string s, string p) {
        unordered_map<char, int> ret;
        for (auto x : p) {
            ret[x]++;
        }

        vector<int> ans;
        unordered_map<char, int> f;
        for (int i = 0; i < p.size() && i < s.size(); i++) {
            f[s[i]]++;
        }
        if (f == ret)
            ans.push_back(0);
        for (int i = p.size(); i < s.size(); ++i) {

            f[s[i]]++;
            f[s[i - p.size()]]--;
            if (f[s[i - p.size()]] == 0) {
                f.erase(s[i - p.size()]);
            }

            if (f == ret)
                ans.push_back(i - p.size() + 1);
        }
        return ans;
    }
};




class Solution {
public:
    int subarraySum(vector<int>& nums, int k) {
        int ans = 0;
        unordered_map<int, int> ret = {{0, 1}};
        int pre = 0;
        for(int x :nums){
            pre+=x ;
            ans += ret.count(pre-k) ? ret[pre-k] : 0;
            ret[pre]++;
        }
        return ans ;
    }
};