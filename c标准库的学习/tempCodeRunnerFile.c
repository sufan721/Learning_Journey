#include <stdio.h>
#include <unistd.h>

int main(){
    int ret = 10;
    while(ret >= 0){
        printf("倒计时：%d", ret);
        fflush(stdout);
        sleep(1);
        ret--;
    }
    printf("\n");
    return 0;
}
