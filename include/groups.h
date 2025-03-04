#ifndef GROUP_H
#define GROUP_H

#include <vector>
#include "flexoffer.h"

using namespace std;
class Fo_Group {
private:
    int id;
    vector<Flexoffer> flexoffers;

public:
    Fo_Group(int group_id);

    void addFlexOffer(const Flexoffer &fo);
    const vector<Flexoffer>& getFlexOffers() const;
    int getGroupId() const;
};
#endif // GROUP_H

