#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

Time_control::Time_control(const uint16_t retx_timeout)
    : initial_rto{retx_timeout}
    , now_rto{retx_timeout}
    , sum_time{0}{}

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _ackno{_isn}
    , windowsize{1}
    , consec_trans_time{0}
    , time_control(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return not_ack; }

void TCPSender::fill_window() {
    TCPSegment seg;
    if(!_syn){
        seg.header().syn = true;
        seg.header().seqno = wrap(_next_seqno, _isn);
        _next_seqno++;
        not_ack ++;
        _segments_out.push(seg);
        _segments_store.push(seg);
        if(!time_control.state){
            time_control.state = true;
            time_control.sum_time = 0;
        }
        _syn = true;
    }

    if(!_fin && stream_in().eof() && (unwrap(_ackno, _isn, _next_seqno) + windowsize > _next_seqno)){
        _fin = true;
        seg.header().fin = true;
        seg.header().seqno = wrap(_next_seqno, _isn);
        _next_seqno ++;
        not_ack ++;
        _segments_out.push(seg);
        _segments_store.push(seg);
        if(!time_control.state){
            time_control.state = true;
            time_control.sum_time = 0;
        }
    }

    while(!_stream.buffer_empty() && (unwrap(_ackno, _isn, _next_seqno) + windowsize > _next_seqno)){
        if(_fin)
            return;
        int length1 = _stream.buffer_size();
        int length2 = unwrap(_ackno, _isn, _next_seqno) + windowsize - _next_seqno;
        int final_length = 0;
        if(length2 >= final_length)
            final_length = length2;
        if(final_length > length1)
            final_length = length1;
        if(final_length > static_cast<int>(TCPConfig::MAX_PAYLOAD_SIZE))
            final_length = static_cast<int>(TCPConfig::MAX_PAYLOAD_SIZE);
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.payload() = Buffer(_stream.read(final_length));
        if(!_fin && stream_in().eof() && (final_length+1) <= length2){
            _fin = true;
            seg.header().fin = true;
        }
        _next_seqno += seg.length_in_sequence_space();
        not_ack += seg.length_in_sequence_space();
        _segments_out.push(seg);
        _segments_store.push(seg);
        if(!time_control.state){
            time_control.state = true;
            time_control.sum_time = 0;
        }
    }
}
    


//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 

    uint64_t new_absolute_index = unwrap(ackno, _isn, _next_seqno);
    uint64_t old_absolute_index = unwrap(_ackno, _isn, _next_seqno);

    if(new_absolute_index > _next_seqno || new_absolute_index < old_absolute_index)
        return;
    if(new_absolute_index > old_absolute_index){
        time_control.now_rto = time_control.initial_rto;
        time_control.sum_time = 0;
        consec_trans_time = 0;
    }

    TCPSender::_ackno = ackno;
    TCPSender::windowsize = window_size;

    while(!_segments_store.empty()){
        if(unwrap(_segments_store.front().header().seqno, _isn, _next_seqno) + _segments_store.front().length_in_sequence_space() <= new_absolute_index){
            not_ack -= _segments_store.front().length_in_sequence_space();
            _segments_store.pop();
        } else{
            break;
        }
    }

    if(!bytes_in_flight()){
        time_control.state = false;
    }

    if(windowsize == 0){
        empty_window = true;
        windowsize = 1;
    } else{
        empty_window = false;
    }

    fill_window();
 }

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if(!time_control.state)
        return;
    
    time_control.sum_time += ms_since_last_tick;
    if(time_control.sum_time >= time_control.now_rto){
        _segments_out.push(_segments_store.front());
        if(!empty_window){
            consec_trans_time ++;
            time_control.now_rto = time_control.now_rto * 2;
        }
        time_control.sum_time = 0;
    }
 }

unsigned int TCPSender::consecutive_retransmissions() const { return TCPSender::consec_trans_time; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

WrappingInt32 TCPSender::get_ackno() {return TCPSender::_ackno;}

uint16_t TCPSender::get_windowsize() {return TCPSender::windowsize;}

void TCPSender::modify_ackno(uint32_t n) {TCPSender::_ackno = WrappingInt32(n);}

void TCPSender::modify_windowsize(uint16_t n) {TCPSender::windowsize = n;}
