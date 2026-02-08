class Solution:
    def twoSum(self, nums: List[int], target: int) -> List[int]:
        ret = {}
        for i, x in enumerate:
            if target - x in ret:
                return [ret[target - x], i]
            ret[x] = i
class Solution:
    def shuffle(self, nums: List[int], n: int) -> List[int]:
        ans = []
        k = len(nums)/2
        for i in range(0,len(nums)/2):  
            ans.append(nums[i])
            ans.append(nums[i+k])
        return ans
    

class Solution:
    def findMaxConsecutiveOnes(self, nums: List[int]) -> int:
        ans = 0
        for i in range (len(nums)):
            j  =i
            while(i<len(nums) and nums[i] == 1):
                i+=1
            ans=max(ans, i -j )
        return ans


