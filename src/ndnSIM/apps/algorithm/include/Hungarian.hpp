
#include<bits/stdc++.h>

class Hungarian {
public:

    void init_labels();

    void update_labels();

    void add_to_tree(int x, int prev_iousx);

    void augment(std::vector<int> &allocation);

    int hungarian(std::vector<int> &allocation);

    std::vector<int> assignmentProblem(int Arr[], int N);

    // Hungarian variables
    std::vector<std::vector<int>> cost; // cost matrix
    int n, max_match; // n workers and n jobs
    std::vector<int> lx, ly; // labels of X and Y parts
    std::vector<int> xy; // xy[x] - vertex that is matched with x
    std::vector<int> yx; // yx[y] - vertex that is matched with y
    std::vector<bool> S, T; // sets S and T in algorithm
    std::vector<int> slack; // as in the algorithm description
    std::vector<int> slackx; // slackx[y] such a vertex, that
    std::vector<int> prev_ious; // array for memorizing alternating path
    std::vector<int> q;
};
