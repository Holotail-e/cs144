#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), asm_seg(), vflag(),/*vline_tree({0,0})*/ eof_flag(false), eofi(0), unasm_bytes(1e8), asm_bytes(0), maxsize(0), offset(0), maxindex(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //DUMMY_CODE(data, index, eof);
    //if(_output.bytes_read() != offset) offset = _output.bytes_read();

    //vline_tree.push_back({index, index+data.size()});
    //sort(vline_tree.begin(), vline_tree.end(), )
    //data.find(-1)
    uint64_t lastidx = data.size() + index;
    maxindex = lastidx > maxindex ? lastidx : maxindex;
    uint64_t _ofs = _capacity + offset;
    if(lastidx > maxsize)
        maxsize = lastidx > _ofs ? lastidx : _ofs;
    if(eof) {
        //asm_seg.append(maxsize-asm_seg.size(), -1);
        //asm_seg.resize(maxsize);
        eofi = lastidx;
        eof_flag = true;
    }
    
    /*
    if(vline_tree[0].second > index) {
        if(vline_tree[0].second < lastidx)
            vline_tree[0].second = lastidx > maxsize ? maxsize : lastidx;
        else return;                                                    //have been assembled
    }
    int ii {-1}, jj {-1};
    bool first_left {false}, last_right {false}; 
    for(int x {0}; x < vline_tree.size(); vline_tree) {    
        if(vline_tree[x].first > index && !first_left){
            ii = x;
            first_left = true;
        }
        if(vline_tree[x].first < index && vline_tree[x].second < lastidx)
        {
            ii = x;
        }
        if(vline_tree[x].first < index && vline_tree[x].second > lastidx)
            return;
        if(vline_tree[x].second > lastidx && vline_tree[x].first > index) {
            jj = x;
            last_right = true;
        }
        if(vline_tree[x].first > index && vline_tree[x].second < lastidx && x == vline_tree.size()-1){
            jj = x;
        }
    }
    if(ii == -1 && jj == -1) {
        vline_tree.push_back({index, lastidx});
    }
    else if(ii != -1 && jj == -1) {
        vline_tree[ii].second = maxsize > lastidx ? lastidx : maxsize;
    }
    else if(ii == -1 && jj != -1) {

    }
    else {

    }
    */
    
    if(!data.size() && !eof) return;                                    //data == 0 and not at eof
    if(asm_seg.size() < maxsize) { 
        asm_seg.append(maxsize-asm_seg.size(), 0);               //asm_seg.resize(maxsize-asm_seg.size());
        //vflag.append(maxsize-asm_seg.size(),0);
        //for(uint64_t i = 0; i < maxsize-asm_seg.size(); i++) .push_back(0);
        vflag.resize(maxsize);
    }

    //if(asm_seg.size() < maxsize) {
    //    asm_seg.resize(maxsize);
    //    vflag.resize(maxsize);
    //}
    if((_ofs < lastidx) && (_ofs > index)) {
        asm_seg.replace(index, _ofs-index, data.substr(0, _ofs-index));
        for(uint64_t i = index; i < _ofs; i++) vflag[i] = 1;
        //vflag.replace(index, _ofs-index, std::string(1,0,_ofs-index));
    }
    else {
        asm_seg.replace(index, data.size(), data);
        for(uint64_t i = index; i < lastidx; i++) vflag[i] = 1;
        //vflag.replace(index, data.size(), std::string('1',0,data.size()));
    }
    uint64_t i { asm_bytes };
    if(vflag.end() != vflag.begin()) while(vflag[i]) i++;
    //while(static_cast<int>(asm_seg[i]) != -1) i++;
    if(i != asm_bytes) {
        uint64_t bwcnt = _output.remaining_capacity() > (i-asm_bytes) ? (i-asm_bytes) : _output.remaining_capacity();
        _output.write(asm_seg.substr(asm_bytes, bwcnt));
        i = asm_bytes + bwcnt;
    }
    //_output.write(asm_seg.substr(asm_bytes, i - asm_bytes));
    asm_bytes = i;
    offset = asm_bytes;
    int64_t uncnt = 0;
    for(uint64_t j = asm_bytes; j < maxindex; j++) if(vflag[j]) uncnt++;
    unasm_bytes = uncnt;
    if(eof_flag && empty()) _output.end_input();
    return;
}

size_t StreamReassembler::unassembled_bytes() const { return unasm_bytes; }

bool StreamReassembler::empty() const { 
    if((unasm_bytes != 0) && (unasm_bytes != 1e8)) return false; 
    else return true;
}
