#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(seg.header().syn == true){
        ISN = seg.header().seqno.raw_value();
        connect = true;
    }

    if(!connect)
        return;

    if(seg.header().fin)
        finished = true;

    uint64_t index = unwrap(seg.header().seqno, WrappingInt32(ISN), _reassembler.stream_out().bytes_written());
    
    index = index + static_cast<uint64_t>(seg.header().syn) - 1;//converting to stream index, and make sure that first segment's index is 0
    
    _reassembler.push_substring(seg.payload().copy(), index, finished);

}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(!connect)
        return std::nullopt;
    else{
        uint64_t index = _reassembler.stream_out().bytes_written() + ((connect) ? 1:0);
        index += (finished && (_reassembler.unassembled_bytes() == 0)) ? 1 : 0;
        return wrap(index, WrappingInt32(ISN));
    }
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
