#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPConnection::send_seg(){
    while(!_sender.segments_out().empty()){
        if(_receiver.ackno().has_value()){
            _sender.segments_out().front().header().ack = 1;
            _sender.segments_out().front().header().ackno = _receiver.ackno().value();
            _sender.segments_out().front().header().win = _receiver.window_size();
        }
        segments_out().push(_sender.segments_out().front());
        _sender.segments_out().pop();
    }
}

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return total_time_since_lastAccept; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    total_time_since_lastAccept = 0;
    if (seg.header().rst == 1) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        connection_active = false;
        return;
    }

    _receiver.segment_received(seg);
    if(seg.header().ack == 1) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED) {
        connect();
        return;
    }


    if (seg.length_in_sequence_space() != 0) {
        _sender.send_empty_segment();
    }

    if (_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0) &&
            seg.header().seqno == _receiver.ackno().value() - 1) {
            _sender.send_empty_segment();
    }

    send_seg();
}

bool TCPConnection::active() const { return connection_active; }

size_t TCPConnection::write(const string &data) {
    size_t result = _sender.stream_in().write(data);
    _sender.fill_window();
    send_seg();
    return result;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if(_sender.get_fin() == 0 && _receiver.stream_out().input_ended())
        _linger_after_streams_finish = false;
    _sender.tick(ms_since_last_tick);
    total_time_since_lastAccept += ms_since_last_tick;
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        TCPSegment seg;
        seg.header().rst = 1;
        seg.header().seqno = _sender.next_seqno();

        segments_out().push(seg);
        connection_active = false;
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        return;
    }

    if(_receiver.unassembled_bytes() == 0 && _receiver.stream_out().input_ended() && bytes_in_flight() == 0
     && _sender.stream_in().input_ended() && total_time_since_lastAccept >= 10 * _cfg.rt_timeout && _linger_after_streams_finish) {
        connection_active = false;
     }

    if(_receiver.unassembled_bytes() == 0 && _receiver.stream_out().input_ended() && bytes_in_flight() == 0
     && _sender.stream_in().input_ended() && !_linger_after_streams_finish) {
        connection_active = false;
     }


    send_seg();
}

void TCPConnection::end_input_stream() {
    if(_sender.get_fin() == 0 && _receiver.stream_out().input_ended())
        _linger_after_streams_finish = false;
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_seg();
}

void TCPConnection::connect() {
    _sender.fill_window();
    connection_active = true;
    send_seg();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            TCPSegment seg;
            seg.header().rst = 1;
            seg.header().seqno = _sender.next_seqno();
            _sender.segments_out().push(seg);
            connection_active = false;
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            send_seg();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
