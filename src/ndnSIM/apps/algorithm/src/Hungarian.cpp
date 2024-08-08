#include "../include/Hungarian.hpp"
#include <algorithm>

void Hungarian::init_labels()
{
    for (int x = 0; x < n; x++)
        for (int y = 0; y < n; y++)
            lx[x] = std::max(lx[x], cost[x][y]);
}


void Hungarian::update_labels()
{
    int x, y;
    int delta = 999999999; //init delta as infinity
    for (y = 0; y < n; y++) //calculate delta using slack
        if (!T[y])
            delta = std::min(delta, slack[y]);
    for (x = 0; x < n; x++) //update X labels
        if (S[x])
            lx[x] -= delta;
    for (y = 0; y < n; y++) //update Y labels
        if (T[y])
            ly[y] += delta;
    for (y = 0; y < n; y++) //update slack array
        if (!T[y])
            slack[y] -= delta;
}


void Hungarian::add_to_tree(int x, int prev_iousx)
//x - current vertex,prev_iousx - vertex from X before x in the alternating path, //so we add edges (prev_iousx, xy[x]), (xy[x], x)
{
    S[x] = true; //add x to S
    // prev_ious[x] = prev_iousx; //we need this when augmenting
    for (int y = 0; y < n; y++) //update slacks, because we add new vertex to S
        if (lx[x] + ly[y] - cost[x][y] < slack[y]) {
            slack[y] = lx[x] + ly[y] - cost[x][y];
            slackx[y] = x;
        }
}


void Hungarian::augment(std::vector<int> &allocation) //main function of the algorithm
{
    if (max_match == n) return; //check whether matching is already perfect
    int x, y, root; //just counters and root vertex
    int wr = 0, rd = 0; //q - queue for bfs, wr,rd - write and read

    for (x = 0; x < n; x++) //finding root of the tree
    {
        if (xy[x] == -1) {
            q[wr++] = root = x;
            prev_ious[x] = -2;
            S[x] = true;
            break;
        }
    }

    for (y = 0; y < n; y++) //initializing slack array
    {
        slack[y] = lx[root] + ly[y] - cost[root][y];
        slackx[y] = root;
    }

    //second part of augment() function
    while (true) //main cycle
    {
        while (rd < wr) //building tree with bfs cycle
        {
            x = q[rd++]; //current vertex from X part
            for (y = 0; y < n; y++) //iterate through all edges in equality graph
                if (cost[x][y] == lx[x] + ly[y] && !T[y]) {
                    if (yx[y] == -1) break; //an exposed vertex in Y found, so
                    //augmenting path exists!
                    T[y] = true; //else just add y to T,
                    q[wr++] = yx[y]; //add vertex yx[y], which is matched
                    //with y, to the queue
                    add_to_tree(yx[y], x); //add edges (x,y) and (y,yx[y]) to the tree
                }
            if (y < n)
                break; //augmenting path found!
        }
        if (y < n)
            break; //augmenting path found!

        update_labels(); //augmenting path not found, so improve labeling
        wr = rd = 0;
        for (y = 0; y < n; y++)
            //in this cycle we add edges that were added to the equality graph as a
            //result of improving the labeling, we add edge (slackx[y], y) to the tree if
            //and only if !T[y] && slack[y] == 0, also with this edge we add another one
            //(y, yx[y]) or augment the matching, if y was exposed
            if (!T[y] && slack[y] == 0) {
                if (yx[y] == -1) //exposed vertex in Y found - augmenting path exists!
                {
                    x = slackx[y];
                    break;
                } else {
                    T[y] = true; //else just add y to T,
                    if (!S[yx[y]]) {
                        q[wr++] = yx[y]; //add vertex yx[y], which is matched with
                        //y, to the queue
                        add_to_tree(yx[y], slackx[y]); //and add edges (x,y) and (y,
                        //yx[y]) to the tree
                    }
                }
            }
        if (y < n) break; //augmenting path found!
    }

    if (y < n) //we found augmenting path!
    {
        max_match++; //increment matching
        //in this cycle we inverse edges along augmenting path
        for (int cx = x, cy = y, ty; cx != -2; cx = prev_ious[cx], cy = ty) {
            ty = xy[cx];
            yx[cy] = cx;
            xy[cx] = cy;
        }
        augment(allocation); //recall function, go to step 1 of the algorithm
    }
}//end of augment() function

int Hungarian::hungarian(std::vector<int> &allocation) {

    int ret = 0; //weight of the optimal matching
    max_match = 0; //number of vertices in current matching
    init_labels(); //step 0
    augment(allocation); //steps 1-3

    for (int x = 0; x < n; x++) {
        ret += cost[x][xy[x]];
        allocation[x * n + xy[x]] = 1;
    }//forming answer there

    return ret;
}

std::vector<int> Hungarian::assignmentProblem(int Arr[], int N)
{
    n = N;
    cost.resize(n, std::vector<int>(n));
    lx.resize(n, 0);
    ly.resize(n, 0);
    xy.resize(n, -1);
    yx.resize(n, -1);
    S.resize(n, false);
    T.resize(n, false);
    slack.resize(n, 0);
    slackx.resize(n, 0);
    prev_ious.resize(n, -1);
    q.resize(n, 0);

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            cost[i][j] = -1 * Arr[i * n + j];

    std::vector<int> allocation(N * N, 0);
    hungarian(allocation);
    return allocation;
}
// End of Hungarian algorithm