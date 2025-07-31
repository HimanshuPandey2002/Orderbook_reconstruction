#include "orderbook.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace std;

void test_basic_add_orders() {
    cout << "Testing basic add orders..." << endl;
    
    OrderBook book;
    
    // Add some bid orders
    book.addOrder('B', 10.50, 100, 1001);
    book.addOrder('B', 10.25, 200, 1002);
    
    // Add some ask orders  
    book.addOrder('A', 10.75, 150, 1003);
    book.addOrder('A', 11.00, 100, 1004);
    
    // Create a dummy MBO record for testing
    MBORecord dummy_mbo = {};
    dummy_mbo.ts_recv = "2025-07-17T08:05:03.360677248Z";
    dummy_mbo.ts_event = "2025-07-17T08:05:03.360677248Z";
    dummy_mbo.rtype = 160;
    dummy_mbo.publisher_id = 2;
    dummy_mbo.instrument_id = 1108;
    dummy_mbo.action = 'A';
    dummy_mbo.side = 'B';
    dummy_mbo.price = 10.50;
    dummy_mbo.size = 100;
    dummy_mbo.flags = 130;
    dummy_mbo.ts_in_delta = 165200;
    dummy_mbo.sequence = 851012;
    dummy_mbo.symbol = "ARL";
    dummy_mbo.order_id = 1001;
    
    MBPRecord mbp = book.generateMBP(dummy_mbo);
    
    // Check that we have the right number of levels
    assert(mbp.bid_prices[0] == 10.50);  // Best bid
    assert(mbp.bid_sizes[0] == 100);
    assert(mbp.bid_prices[1] == 10.25);  // Second best bid
    assert(mbp.bid_sizes[1] == 200);
    
    assert(mbp.ask_prices[0] == 10.75);  // Best ask
    assert(mbp.ask_sizes[0] == 150);
    assert(mbp.ask_prices[1] == 11.00);  // Second best ask
    assert(mbp.ask_sizes[1] == 100);
    
    cout << "✓ Basic add orders test passed" << endl;
}

void test_cancel_orders() {
    cout << "Testing order cancellations..." << endl;
    
    OrderBook book;
    
    // Add orders
    book.addOrder('B', 10.50, 100, 1001);
    book.addOrder('B', 10.25, 200, 1002);
    book.addOrder('A', 10.75, 150, 1003);
    
    // Cancel one order
    book.cancelOrder(1001, 'B', 10.50, 100);
    
    MBORecord dummy_mbo = {};
    dummy_mbo.action = 'C';
    dummy_mbo.side = 'B';
    dummy_mbo.price = 10.50;
    dummy_mbo.size = 100;
    dummy_mbo.symbol = "TEST";
    
    MBPRecord mbp = book.generateMBP(dummy_mbo);
    
    // Best bid should now be 10.25
    assert(mbp.bid_prices[0] == 10.25);
    assert(mbp.bid_sizes[0] == 200);
    
    // Second level should be empty
    assert(mbp.bid_prices[1] == 0.0);
    assert(mbp.bid_sizes[1] == 0);
    
    cout << "✓ Cancel orders test passed" << endl;
}

void test_trade_handling() {
    std::cout << "Testing trade handling..." << std::endl;
    
    OrderBook book;
    
    // Set up order book
    book.addOrder('B', 10.50, 100, 1001);
    book.addOrder('A', 10.75, 150, 1002);
    
    // Simulate a trade that hits the ask side (removes liquidity from asks)
    book.handleTrade('A', 10.75, 50);  // Remove 50 from ask side at 10.75
    
    MBORecord dummy_mbo = {};
    dummy_mbo.action = 'T';
    dummy_mbo.side = 'A';
    dummy_mbo.price = 10.75;
    dummy_mbo.size = 50;
    dummy_mbo.symbol = "TEST";
    
    MBPRecord mbp = book.generateMBP(dummy_mbo);
    
    // Ask size should be reduced to 100 (150 - 50 = 100)
    assert(mbp.ask_prices[0] == 10.75);
    assert(mbp.ask_sizes[0] == 100);  // 150 - 50 = 100
    
    // Bid should remain unchanged
    assert(mbp.bid_prices[0] == 10.50);
    assert(mbp.bid_sizes[0] == 100);
    
    std::cout << "✓ Trade handling test passed" << std::endl;
}

void test_csv_parsing() {
    cout << "Testing CSV parsing..." << endl;
    
    string test_line = "2025-07-17T08:05:03.360677248Z,2025-07-17T08:05:03.360677248Z,160,2,1108,A,B,5.510000000,100,0,817593,130,165200,851012,ARL";
    
    MBORecord record = CSVProcessor::parseMBOLine(test_line);
    
    assert(record.action == 'A');
    assert(record.side == 'B');
    assert(record.price == 5.51);
    assert(record.size == 100);
    assert(record.order_id == 817593);
    assert(record.symbol == "ARL");
    
    cout << "✓ CSV parsing test passed" << endl;
}

void test_mbp_formatting() {
    cout << "Testing MBP formatting..." << endl;
    
    MBPRecord record;
    record.ts_recv = "2025-07-17T08:05:03.360677248Z";
    record.ts_event = "2025-07-17T08:05:03.360677248Z";
    record.rtype = 10;
    record.publisher_id = 2;
    record.instrument_id = 1108;
    record.action = 'A';
    record.side = 'B';
    record.depth = 0;
    record.price = 5.51;
    record.size = 100;
    record.flags = 130;
    record.ts_in_delta = 165200;
    record.sequence = 851012;
    record.symbol = "ARL";
    record.order_id = 817593;
    
    // Set some bid/ask data
    record.bid_prices[0] = 5.51;
    record.bid_sizes[0] = 100;
    record.bid_counts[0] = 1;
    
    string formatted = CSVProcessor::formatMBPLine(record, 1);
    
    // Check that it contains expected elements
    assert(formatted.find("2025-07-17T08:05:03.360677248Z") != string::npos);
    assert(formatted.find("5.51") != string::npos);
    assert(formatted.find("100") != string::npos);
    assert(formatted.find("ARL") != string::npos);
    
    cout << "✓ MBP formatting test passed" << endl;
}

void test_edge_cases() {
    cout << "Testing edge cases..." << endl;
    
    OrderBook book;
    
    // Test empty book
    MBORecord dummy_mbo = {};
    dummy_mbo.symbol = "TEST";
    MBPRecord mbp = book.generateMBP(dummy_mbo);
    
    // All levels should be empty
    for (int i = 0; i < 10; i++) {
        assert(mbp.bid_prices[i] == 0.0);
        assert(mbp.bid_sizes[i] == 0);
        assert(mbp.ask_prices[i] == 0.0);
        assert(mbp.ask_sizes[i] == 0);
    }
    
    // Test cancelling non-existent order (should not crash)
    book.cancelOrder(99999, 'B', 10.50, 100);
    
    // Test trade on empty side (should not crash)
    book.handleTrade('B', 10.50, 100);
    
    cout << "✓ Edge cases test passed" << endl;
}

void run_performance_test() {
    cout << "Running performance test..." << endl;
    
    auto start = chrono::high_resolution_clock::now();
    
    OrderBook book;
    const int num_operations = 100000;
    
    // Simulate many operations
    for (int i = 0; i < num_operations; i++) {
        double price = 10.0 + (i % 100) * 0.01;
        book.addOrder('B', price, 100, i + 1000);
        
        if (i % 10 == 0) {
            MBORecord dummy_mbo = {};
            dummy_mbo.symbol = "PERF";
            book.generateMBP(dummy_mbo);
        }
        
        if (i % 100 == 0 && i > 0) {
            book.cancelOrder(i + 900, 'B', 10.0 + ((i-100) % 100) * 0.01, 100);
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    cout << "✓ Performance test completed: " << num_operations 
              << " operations in " << duration.count() << "ms" << endl;
    cout << "  Average: " << (double)duration.count() / num_operations * 1000 
              << " microseconds per operation" << endl;
}

int main() {
    cout << "===============================================" << endl;
    cout << "ORDERBOOK RECONSTRUCTION - UNIT TESTS" << endl;
    cout << "===============================================" << endl;
    
    try {
        test_basic_add_orders();
        test_cancel_orders();
        test_trade_handling();
        test_csv_parsing();
        test_mbp_formatting();
        test_edge_cases();
        run_performance_test();
        
        cout << "\n✅ ALL TESTS PASSED!" << endl;
        cout << "The orderbook reconstruction system is working correctly." << endl;
        
    } catch (const exception& e) {
        cerr << "\n❌ TEST FAILED: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "\n❌ UNKNOWN TEST FAILURE" << endl;
        return 1;
    }
    
    return 0;
}
