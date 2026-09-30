#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <utility>

namespace qqsc
{
// Non-audio-thread B+ tree. Full-precision positive float energies are retained;
// no dB bins or approximate gate membership. Insertion/query touch O(log N)
// nodes and at most 128 entries in the boundary leaf. Allocation happens only
// on the consumer thread. Equal keys are valid, including at the relative gate.
template<size_t ExtraColumns = 0>
class ExactEnergyIndex
{
public:
    using Entry = std::array<float, 1 + ExtraColumns>;
    struct Sum
    {
        std::array<double, 1 + ExtraColumns> values{};
        uint64_t count = 0;
        void add(const Entry& e) noexcept
        { for(size_t i=0;i<values.size();++i) values[i]+=double(e[i]); ++count; }
        void add(const Sum& s) noexcept
        { for(size_t i=0;i<values.size();++i) values[i]+=s.values[i]; count+=s.count; }
    };
    void clear() noexcept { root.reset(); }
    Sum total() const noexcept { return root ? root->sum : Sum{}; }
    Sum above(double gate) const noexcept { return root ? suffix(*root,gate) : Sum{}; }
    void add(const Entry& e)
    {
        if(!(e[0]>0) || !std::isfinite(e[0])) return;
        if(!root) root=std::make_unique<Node>();
        if(auto split=insert(*root,e))
        {
            auto parent=std::make_unique<Node>();parent->leaf=false;parent->size=2;
            parent->children[0]=std::move(root);parent->children[1]=std::move(split);
            update(*parent);root=std::move(parent);
        }
    }
private:
    static constexpr size_t leafCapacity=128, branchCapacity=16;
    struct Node
    {
        bool leaf=true;
        size_t size=0;
        float minimum=0, maximum=0;
        Sum sum;
        std::array<Entry,leafCapacity+1> entries{};
        std::array<std::unique_ptr<Node>,branchCapacity+1> children{};
    };
    static void update(Node& n) noexcept
    {
        n.sum={};
        if(n.leaf)
        {
            for(size_t i=0;i<n.size;++i)n.sum.add(n.entries[i]);
            n.minimum=n.entries[0][0];n.maximum=n.entries[n.size-1][0];
        }
        else
        {
            for(size_t i=0;i<n.size;++i)n.sum.add(n.children[i]->sum);
            n.minimum=n.children[0]->minimum;n.maximum=n.children[n.size-1]->maximum;
        }
    }
    static std::unique_ptr<Node> insert(Node& n,const Entry& e)
    {
        if(n.leaf)
        {
            const auto end=n.entries.begin()+static_cast<std::ptrdiff_t>(n.size);
            const auto pos=std::upper_bound(n.entries.begin(),end,e[0],
                [](float key,const Entry& entry){return key<entry[0];});
            std::move_backward(pos,end,end+1);*pos=e;++n.size;
        }
        else
        {
            size_t i=0;while(i+1<n.size && e[0]>n.children[i]->maximum)++i;
            if(auto sibling=insert(*n.children[i],e))
            {
                for(size_t j=n.size;j>i+1;--j)n.children[j]=std::move(n.children[j-1]);
                n.children[i+1]=std::move(sibling);++n.size;
            }
        }
        const auto capacity=n.leaf?leafCapacity:branchCapacity;
        if(n.size<=capacity){update(n);return {};}
        auto sibling=std::make_unique<Node>();sibling->leaf=n.leaf;
        const auto middle=n.size/2;sibling->size=n.size-middle;
        for(size_t i=0;i<sibling->size;++i)
            if(n.leaf)sibling->entries[i]=n.entries[middle+i];
            else sibling->children[i]=std::move(n.children[middle+i]);
        n.size=middle;update(n);update(*sibling);return sibling;
    }
    static Sum suffix(const Node& n,double gate) noexcept
    {
        if(double(n.maximum)<=gate)return {};
        if(double(n.minimum)>gate)return n.sum;
        Sum s;
        if(n.leaf)
        {
            const auto end=n.entries.begin()+static_cast<std::ptrdiff_t>(n.size);
            auto it=std::upper_bound(n.entries.begin(),end,gate,
                [](double key,const Entry& entry){return key<double(entry[0]);});
            for(;it!=end;++it)s.add(*it);
        }
        else for(size_t i=0;i<n.size;++i)s.add(suffix(*n.children[i],gate));
        return s;
    }
    std::unique_ptr<Node> root;
};
}
