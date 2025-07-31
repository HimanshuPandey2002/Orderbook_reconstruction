#pragma once
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

struct MBORecord {
    string ts_recv;
    string ts_event;
    int rtype;
    int publisher_id;
    int instrument_id;
    char action;
    char side;
    double price;
    int size;
    int channel_id;
    long order_id;
    int flags;
    long ts_in_delta;
    long sequence;
    string symbol;
};

struct MBPRecord {
    string ts_recv;
    string ts_event;
    int rtype;
    int publisher_id;
    int instrument_id;
    char action;
    char side;
    int depth;
    double price;
    int size;
    int flags;
    long ts_in_delta;
    long sequence;
    
    // Price levels (bid_px_00, bid_sz_00, etc.)
    vector<double> bid_prices;
    vector<int> bid_sizes;
    vector<int> bid_counts;
    vector<double> ask_prices;
    vector<int> ask_sizes;
    vector<int> ask_counts;
    
    string symbol;
    long order_id;
    
    MBPRecord() : bid_prices(10, 0.0), bid_sizes(10, 0), bid_counts(10, 0),
                  ask_prices(10, 0.0), ask_sizes(10, 0), ask_counts(10, 0) {}
};

class OrderBook {
private:
    // Use maps for efficient price-level operations
    // Key: price, Value: {total_size, order_count}
    map<double, pair<int, int>, greater<double>> bids; // descending order
    map<double, pair<int, int>> asks; // ascending order
    
    // Track individual orders for cancellations
    map<long, pair<double, int>> order_tracker; // order_id -> {price, size}
    
public:
    void addOrder(char side, double price, int size, long order_id);
    void cancelOrder(long order_id, char side, double price, int size);
    void handleTrade(char side, double price, int size);
    MBPRecord generateMBP(const MBORecord& mbo_record);
    void clear();
    void printBook() const;
};

class CSVProcessor {
public:
    static vector<MBORecord> readMBO(const string& filename);
    static void writeMBP(const vector<MBPRecord>& records, const string& filename);
    static MBORecord parseMBOLine(const string& line);
    static string formatMBPLine(const MBPRecord& record, int index);
};