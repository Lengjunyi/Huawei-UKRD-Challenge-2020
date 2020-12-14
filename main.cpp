#include <iostream>
#include <cstdio>
#include <vector>
#include <omp.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <deque>
#include <chrono>
#include <deque>
using namespace std;
using namespace chrono;
const int MAXS = 100*1024*1024;
const int MAXN = 70000;
int max_node = 0;

vector <int> neighbors[MAXN];
void analyse(char *buf,int len)
{
	for (char *p=buf;*p;p++){
        if (*p == '(') {
            int u = 0, v = 0;
            ++p;
            do {
                u = u*10+*p-'0';
                ++p;
            } while (*p != ',');
            ++p;
            do {
                v= v * 10 +*p-'0';
                ++p;
            } while (*p != ')');
            if (u > max_node) max_node = u;
            if (v > max_node) max_node = v;
            neighbors[u].push_back(v);
        }
    }
}
void fread_analyse(char fileloc[])
{
    int fd = open(fileloc, O_RDONLY);
    int size = lseek(fd, 0, SEEK_END);
    char* buf = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    analyse(buf,size);
    close(fd);
    munmap(buf,size);
}


struct node{int u,next;};

auto time_start =  system_clock::now();
void count_time(){
    auto end   = system_clock::now();
    auto duration = duration_cast<microseconds>(end - time_start);
    cout <<  "Time: " << double(duration.count()) * microseconds::period::num / microseconds::period::den << "s since start" << endl;
}

int main(int argc,char** argv)
{
    fread_analyse(argv[1]);
    count_time();
    // main algorithm
    double Centrality[MAXN]; memset(Centrality,0,sizeof(Centrality));
    int num_thread = stoi(argv[2]);
    
    omp_set_num_threads(num_thread);
    static omp_lock_t lock;
    #pragma omp parallel
    {
        // Assigning range of nodes to be calculated by each thread
        int id = omp_get_thread_num();
        int start = id * max_node / num_thread;
        int end = (id+1) * max_node / num_thread;
        if (id == num_thread - 1){
            end = max_node + 1;
        }
        int o[MAXN]; // # of ways from origin to a certain node
        int d[MAXN]; // distance from origin
        int Q[MAXN]; // 'Queue' of nodes
        int head[MAXN]; // `head` and `NodesBefore` ~= linked list of nodes
        node NodeBefore[MAXN*3];
        double epsilon[MAXN];
        double Centrality_TEMP[MAXN];memset(Centrality_TEMP, 0, sizeof(Centrality_TEMP));
        for (int s=start; s < end; s++){
            int count_start = 0;
            memset(head, 0, sizeof(head));
            memset(o,  0, sizeof(o)); o[s]=1;
            memset(d, -1, sizeof(d)); d[s]=0;
            memset(Q, -1, sizeof(Q)); Q[0]=s;
            int Q_index = 0;
            deque<int> QQ;
            for (int i: QQ){
                
            }
            for (int Q_i = 0; Q[Q_i]>-1; Q_i++){
                int v = Q[Q_i];
                for (int i = 0; i < neighbors[v].size(); i++){
                    int w = neighbors[v][i];
                    // w found for the first time?
                    if (d[w] < 0){
                        Q[++Q_index] = w;
                        d[w] = d[v] + 1;
                    }
                    if (d[w] == d[v] + 1){
                        o[w] += o[v];
                        NodeBefore[++count_start] = (node){v,head[w]};
                        head[w] = count_start;
                    }
                }
            }
            memset(epsilon, 0, sizeof(epsilon));
            for (; Q_index > 0; Q_index--){
                int w = Q[Q_index];
                for (int i = head[w]; i > 0; i=NodeBefore[i].next){
                    int v = NodeBefore[i].u;
                    epsilon[v] += (1.0+epsilon[w])*(o[v])/o[w];
                }
                Centrality_TEMP[w] += epsilon[w];
            }
        }
        omp_set_lock(&lock);
        for (int i = 0; i <= max_node; i++){
            Centrality[i] += Centrality_TEMP[i];
        }
        omp_unset_lock(&lock);
    }
    count_time();
    // main algorithm ends
    double min = Centrality[0];
    double max = Centrality[0];
    for (int i=1; i<=max_node;i++){
        if (Centrality[i]>max) max=Centrality[i];
        if (Centrality[i]<min) min=Centrality[i];
    }
    double coefficient = 1/(max-min);
    #pragma omp parallel for
    for(int i=0;i<=max_node;i++){
        Centrality[i] = (Centrality[i]-min)*coefficient;
    }
    auto *fp = fopen("ans.txt","w");
    fprintf(fp,"[");
    int i = 0;
    for(i=0;i<max_node;i++){
        fprintf(fp,"(%d,%.2f),",i,Centrality[i]);
    }
    fprintf(fp,"(%d,%.2f)]",i,Centrality[i]);
    fclose(fp);
    count_time();
    return 0;
}
