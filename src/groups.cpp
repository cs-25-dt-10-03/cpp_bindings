#include "../include/groups.h"

Fo_Group::Fo_Group(int group_id) : id(group_id) {}

void Fo_Group::addFlexOffer(const Flexoffer& fo) {flexoffers.push_back(fo);}

const std::vector<Flexoffer>& Fo_Group::getFlexOffers() const {return flexoffers;}

int Fo_Group::getGroupId() const {return id;}

