#include "orderbook.h"
#include <algorithm>
#include <iomanip>

using namespace std;

void OrderBook::addOrder(char side, double price, int size, long order_id) {
    if (side == 'B') {
        bids[price].first += size;
        bids[price].second += 1;
    } else if (side == 'A') {
        asks[price].first += size;
        asks[price].second += 1;
    }
    
    // Track the order for potential cancellation
    if (order_id != 0) {
        order_tracker[order_id] = {price, size};
    }
}

void OrderBook::cancelOrder(long order_id, char side, double price, int size) {
    // Remove from the appropriate side
    if (side == 'B' && bids.count(price)) {
        bids[price].first -= size;
        bids[price].second -= 1;
        if (bids[price].first <= 0 || bids[price].second <= 0) {
            bids.erase(price);
        }
    } else if (side == 'A' && asks.count(price)) {
        asks[price].first -= size;
        asks[price].second -= 1;
        if (asks[price].first <= 0 || asks[price].second <= 0) {
            asks.erase(price);
        }
    }
    
    // Remove from order tracker
    order_tracker.erase(order_id);
}

void OrderBook::handleTrade(char side, double price, int size) {
    // For trades, we remove liquidity from the book
    // The side in the MBO data might be incorrect, so we need to check both sides
    if (side == 'B' && asks.count(price)) {
        // Trade hit the ask side
        asks[price].first -= size;
        if (asks[price].first <= 0) {
            asks.erase(price);
        }
    } else if (side == 'A' && bids.count(price)) {
        // Trade hit the bid side
        bids[price].first -= size;
        if (bids[price].first <= 0) {
            bids.erase(price);
        }
    }
}

MBPRecord OrderBook::generateMBP(const MBORecord& mbo_record) {
    MBPRecord mbp;
    
    // Copy basic fields
    mbp.ts_recv = mbo_record.ts_recv;
    mbp.ts_event = mbo_record.ts_event;
    mbp.rtype = 10; // MBP type
    mbp.publisher_id = mbo_record.publisher_id;
    mbp.instrument_id = mbo_record.instrument_id;
    mbp.action = mbo_record.action;
    mbp.side = mbo_record.side;
    mbp.depth = 0;
    mbp.price = mbo_record.price;
    mbp.size = mbo_record.size;
    mbp.flags = mbo_record.flags;
    mbp.ts_in_delta = mbo_record.ts_in_delta;
    mbp.sequence = mbo_record.sequence;
    mbp.symbol = mbo_record.symbol;
    mbp.order_id = mbo_record.order_id;
    
    // Fill bid levels (top 10)
    int level = 0;
    for (auto it = bids.begin(); it != bids.end() && level < 10; ++it, ++level) {
        mbp.bid_prices[level] = it->first;
        mbp.bid_sizes[level] = it->second.first;
        mbp.bid_counts[level] = it->second.second;
    }
    
    // Fill ask levels (top 10)
    level = 0;
    for (auto it = asks.begin(); it != asks.end() && level < 10; ++it, ++level) {
        mbp.ask_prices[level] = it->first;
        mbp.ask_sizes[level] = it->second.first;
        mbp.ask_counts[level] = it->second.second;
    }
    
    return mbp;
}

void OrderBook::clear() {
    bids.clear();
    asks.clear();
    order_tracker.clear();
}

void OrderBook::printBook() const {
    cout << "=== ORDER BOOK ===" << endl;
    cout << "ASKS:" << endl;
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        cout << "  " << fixed << setprecision(2) 
                  << it->first << " x " << it->second.first 
                  << " (" << it->second.second << " orders)" << endl;
    }
    cout << "BIDS:" << endl;
    for (const auto& bid : bids) {
        cout << "  " << fixed << setprecision(2) 
                  << bid.first << " x " << bid.second.first 
                  << " (" << bid.second.second << " orders)" << endl;
    }
    cout << "==================" << endl;
}

vector<MBORecord> CSVProcessor::readMBO(const string& filename) {
    vector<MBORecord> records;
    ifstream file(filename);
    string line;
    
    // Skip header
    getline(file, line);
    
    while (getline(file, line)) {
        if (!line.empty()) {
            records.push_back(parseMBOLine(line));
        }
    }
    
    return records;
}

MBORecord CSVProcessor::parseMBOLine(const string& line) {
    MBORecord record;
    stringstream ss(line);
    string cell;
    int field = 0;
    
    while (getline(ss, cell, ',')) {
        switch (field) {
            case 0: record.ts_recv = cell; break;
            case 1: record.ts_event = cell; break;
            case 2: record.rtype = stoi(cell); break;
            case 3: record.publisher_id = stoi(cell); break;
            case 4: record.instrument_id = stoi(cell); break;
            case 5: record.action = cell.empty() ? ' ' : cell[0]; break;
            case 6: record.side = cell.empty() ? ' ' : cell[0]; break;
            case 7: record.price = cell.empty() ? 0.0 : stod(cell); break;
            case 8: record.size = cell.empty() ? 0 : stoi(cell); break;
            case 9: record.channel_id = stoi(cell); break;
            case 10: record.order_id = stol(cell); break;
            case 11: record.flags = stoi(cell); break;
            case 12: record.ts_in_delta = stol(cell); break;
            case 13: record.sequence = stol(cell); break;
            case 14: record.symbol = cell; break;
        }
        field++;
    }
    
    return record;
}

void CSVProcessor::writeMBP(const vector<MBPRecord>& records, const string& filename) {
    ofstream file(filename);
    
    // Write header
    file << ",ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,depth,price,size,flags,ts_in_delta,sequence,";
    file << "bid_px_00,bid_sz_00,bid_ct_00,ask_px_00,ask_sz_00,ask_ct_00,";
    for (int i = 1; i < 10; i++) {
        file << "bid_px_" << setfill('0') << setw(2) << i << ",";
        file << "bid_sz_" << setfill('0') << setw(2) << i << ",";
        file << "bid_ct_" << setfill('0') << setw(2) << i << ",";
        file << "ask_px_" << setfill('0') << setw(2) << i << ",";
        file << "ask_sz_" << setfill('0') << setw(2) << i << ",";
        file << "ask_ct_" << setfill('0') << setw(2) << i << ",";
    }
    file << "symbol,order_id" << endl;
    
    // Write records
    for (size_t i = 0; i < records.size(); i++) {
        file << formatMBPLine(records[i], i) << endl;
    }
}

string CSVProcessor::formatMBPLine(const MBPRecord& record, int index) {
    stringstream ss;
    
    ss << index << ",";
    ss << record.ts_recv << ",";
    ss << record.ts_event << ",";
    ss << record.rtype << ",";
    ss << record.publisher_id << ",";
    ss << record.instrument_id << ",";
    ss << record.action << ",";
    ss << record.side << ",";
    ss << record.depth << ",";
    
    // Price and size for the action
    if (record.price != 0.0) {
        ss << fixed << setprecision(8) << record.price;
    }
    ss << ",";
    ss << record.size << ",";
    ss << record.flags << ",";
    ss << record.ts_in_delta << ",";
    ss << record.sequence << ",";
    
    // Output all 10 levels for both sides
    for (int i = 0; i < 10; i++) {
        // Bid levels
        if (record.bid_prices[i] != 0.0) {
            ss << fixed << setprecision(2) << record.bid_prices[i];
        }
        ss << ",";
        ss << record.bid_sizes[i] << ",";
        ss << record.bid_counts[i] << ",";
        
        // Ask levels
        if (record.ask_prices[i] != 0.0) {
            ss << fixed << setprecision(2) << record.ask_prices[i];
        }
        ss << ",";
        ss << record.ask_sizes[i] << ",";
        ss << record.ask_counts[i] << ",";
    }
    
    ss << record.symbol << ",";
    ss << record.order_id;
    
    return ss.str();
}