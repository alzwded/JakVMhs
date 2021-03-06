#include <map>
#include <string>
#include <list>
#include <utility>
#include <algorithm>

extern "C" {
#include "sn.h"
}

namespace {

class SN
{
    std::map<unsigned short, std::string> map_;
    std::list<std::pair<unsigned short, unsigned short> > holes_;
    unsigned short max_;
public:
    SN()
    : map_()
    , holes_()
    , max_(0)
    {}

    unsigned short Add(std::string const& s)
    {
        auto inserted = map_.insert(std::make_pair(GetNewKey(), s));
        return inserted.first->first;
    }

    std::string const& Get(unsigned short w)
    {
        static std::string blank("");
        auto found = map_.find(w);
        if(found == map_.end()) return blank;
        return found->second;
    }

    void Erase(unsigned short w)
    {
        auto found = map_.find(w);
        if(found == map_.end()) return;

        DisposeOfKey(found->first);
        map_.erase(found);
    }

    void Reset()
    {
        map_.clear();
        holes_.clear();
        max_ = 0;
    }

private:
    unsigned short GetNewKey()
    {
        unsigned short num = 0;
        unsigned short* pnum = NULL;
        if(holes_.size()) {
            auto p = holes_.begin();
            num = (*p).first;
            pnum = &num;
            ++(*p).first;
            if(p->first == p->second) {
                holes_.erase(holes_.begin());
            }
        }

        if(pnum) {
            return *pnum;
        } else {
            return max_++;
        }
    }

    void DisposeOfKey(unsigned short w)
    {
        if(w == max_ - 1) {
            --max_;
            return;
        }

        bool inserted = false;
        std::for_each(holes_.begin(), holes_.end(), [&](decltype(holes_)::value_type& o){
                if(w == o.second) {
                    inserted = true;
                    o.second++;
                } else if(o.first - 1 == w) {
                    inserted = true;
                    o.first--;
                }
            });

        if(!inserted) {
            decltype(holes_)::value_type item;
            item.first = w;
            item.second = w + 1;
            holes_.push_back(item);
            holes_.sort([&](decltype(holes_)::value_type const& a, decltype(holes_)::value_type const& b){
                    return a.first < b.first;
                });
        }
        
        for(auto i = holes_.begin(); i != holes_.end();) {
            auto prev = i++;
            if(i == holes_.end()) break;
            if(prev->second >= i->first) {
                prev->second = i->second;
                holes_.erase(i);
                i = prev;
            }
        }
    }
};

} // namespace

static SN SNMgr;

unsigned short SN_assign(char const* s)
{
    std::string str(s);
    return SNMgr.Add(s);
}

char const* SN_get(unsigned short w)
{
    std::string const& got = SNMgr.Get(w);
    return got.c_str();
}

void SN_dispose(unsigned short w)
{
    SNMgr.Erase(w);
}

void SN_reset()
{
    SNMgr.Reset();
}
