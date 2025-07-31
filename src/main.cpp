#include "orderbook.h"
#include <iostream>
#include <chrono>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <mbo_input_file.csv>" << endl;
        return 1;
    }
    
    string input_file = argv[1];
    string output_file = "output_mbp.csv";
    
    auto start_time = chrono::high_resolution_clock::now();
    
    try {
        // Read MBO data
        cout << "Reading MBO data from: " << input_file << endl;
        auto mbo_records = CSVProcessor::readMBO(input_file);
        cout << "Read " << mbo_records.size() << " MBO records" << endl;
        
        // Process records
        OrderBook book;
        vector<MBPRecord> mbp_records;
        
        // Skip first record if it's a clear action
        size_t start_idx = 0;
        if (!mbo_records.empty() && mbo_records[0].action == 'R') {
            start_idx = 1;
            cout << "Skipping initial clear action" << endl;
        }
        
        // Track pending trades for T->F->C sequence handling
        struct PendingTrade {
            MBORecord trade_record;
            bool has_fill = false;
            bool has_cancel = false;
        };
        
        map<long, PendingTrade> pending_trades;
        
        for (size_t i = start_idx; i < mbo_records.size(); i++) {
            const auto& record = mbo_records[i];
            
            if (record.action == 'A') {
                // Add order
                book.addOrder(record.side, record.price, record.size, record.order_id);
                mbp_records.push_back(book.generateMBP(record));
                
            } else if (record.action == 'C') {
                // Check if this is part of a T->F->C sequence
                bool is_trade_cancel = false;
                for (auto& [order_id, pending] : pending_trades) {
                    if (pending.trade_record.price == record.price && 
                        pending.trade_record.side == record.side) {
                        // This cancel completes a trade sequence
                        pending.has_cancel = true;
                        is_trade_cancel = true;
                        
                        // Apply the trade (remove liquidity from opposite side)
                        char opposite_side = (record.side == 'B') ? 'A' : 'B';
                        book.handleTrade(opposite_side, record.price, pending.trade_record.size);
                        
                        // Create MBP record for the trade
                        MBORecord trade_for_mbp = pending.trade_record;
                        trade_for_mbp.action = 'T';
                        trade_for_mbp.side = opposite_side; // Correct the side
                        mbp_records.push_back(book.generateMBP(trade_for_mbp));
                        
                        // Clean up
                        pending_trades.erase(order_id);
                        break;
                    }
                }
                
                if (!is_trade_cancel) {
                    // Regular cancel
                    book.cancelOrder(record.order_id, record.side, record.price, record.size);
                    mbp_records.push_back(book.generateMBP(record));
                }
                
            } else if (record.action == 'T') {
                // Trade - check if side is 'N' (should be ignored)
                if (record.side == 'N') {
                    continue;
                }
                
                // Start tracking this trade for potential T->F->C sequence
                pending_trades[record.order_id] = {record, false, false};
                
            } else if (record.action == 'F') {
                // Fill - mark the pending trade
                if (pending_trades.count(record.order_id)) {
                    pending_trades[record.order_id].has_fill = true;
                }
                
            } else if (record.action == 'R') {
                // Clear the book
                book.clear();
                mbp_records.push_back(book.generateMBP(record));
            }
            
            // Progress indicator
            if (i % 10000 == 0) {
                cout << "Processed " << i << "/" << mbo_records.size() << " records" << endl;
            }
        }
        
        // Handle any remaining pending trades (unlikely in well-formed data)
        for (const auto& [order_id, pending] : pending_trades) {
            if (pending.has_fill) {
                // Apply the trade even if cancel is missing
                char opposite_side = (pending.trade_record.side == 'B') ? 'A' : 'B';
                book.handleTrade(opposite_side, pending.trade_record.price, pending.trade_record.size);
                
                MBORecord trade_for_mbp = pending.trade_record;
                trade_for_mbp.action = 'T';
                trade_for_mbp.side = opposite_side;
                mbp_records.push_back(book.generateMBP(trade_for_mbp));
            }
        }
        
        // Write output
        cout << "Writing " << mbp_records.size() << " MBP records to: " << output_file << endl;
        CSVProcessor::writeMBP(mbp_records, output_file);
        
        auto end_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        
        cout << "Processing completed in " << duration.count() << " ms" << endl;
        cout << "Output written to: " << output_file << endl;
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}