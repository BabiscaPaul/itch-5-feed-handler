#include <gtest/gtest.h>
#include "book_manager.h"
#include "order_book.h"

namespace {
    constexpr StockLocate LOC_AAPL = 7;
    constexpr StockLocate LOC_MSFT = 42;
    constexpr OrderRef    R1 = 1001;
    constexpr OrderRef    R2 = 1002;
    constexpr OrderRef    R3 = 1003;
    constexpr OrderRef    R4 = 1004;
    constexpr Price       BID_LOW  = 1640000;
    constexpr Price       BID_MID  = 1645000;
    constexpr Price       BID_HIGH = 1650000;
    constexpr Price       ASK_LOW  = 1660000;
    constexpr Price       ASK_MID  = 1665000;
    constexpr Price       ASK_HIGH = 1670000;
}

// ============================================================
// Stock Directory + symbol resolution
// ============================================================

TEST(BookManagerTest, UnknownSymbolReturnsNullptr) {
    BookManager mgr;
    EXPECT_EQ(mgr.book_for_symbol("AAPL"), nullptr);
}

TEST(BookManagerTest, RegisteredSymbolWithoutOrdersReturnsNullptr) {
    BookManager mgr;
    mgr.on_stock_directory(LOC_AAPL, "AAPL");
    EXPECT_EQ(mgr.book_for_symbol("AAPL"), nullptr);
}

TEST(BookManagerTest, SymbolResolvesAfterFirstAdd) {
    BookManager mgr;
    mgr.on_stock_directory(LOC_AAPL, "AAPL");
    mgr.on_add(R1, Side::Buy, 100, BID_HIGH, LOC_AAPL);

    const OrderBook* book = mgr.book_for_symbol("AAPL");
    ASSERT_NE(book, nullptr);
    EXPECT_TRUE(book->has_bids());
}

TEST(BookManagerTest, SymbolAndLocateResolveToSameBook) {
    BookManager mgr;
    mgr.on_stock_directory(LOC_AAPL, "AAPL");
    mgr.on_add(R1, Side::Buy, 100, BID_HIGH, LOC_AAPL);

    EXPECT_EQ(mgr.book_for_symbol("AAPL"), mgr.book_for(LOC_AAPL));
}

TEST(BookManagerTest, MultipleSymbolsAreIndependent) {
    BookManager mgr;
    mgr.on_stock_directory(LOC_AAPL, "AAPL");
    mgr.on_stock_directory(LOC_MSFT, "MSFT");
    mgr.on_add(R1, Side::Buy, 100, BID_HIGH, LOC_AAPL);

    EXPECT_NE(mgr.book_for_symbol("AAPL"), nullptr);
    EXPECT_EQ(mgr.book_for_symbol("MSFT"), nullptr);
}

TEST(BookManagerTest, BookForUnknownLocateReturnsNullptr) {
    BookManager mgr;
    EXPECT_EQ(mgr.book_for(99), nullptr);
}

TEST(BookManagerTest, BookForUnregisteredSymbolButValidLocate) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy, 100, BID_HIGH, LOC_AAPL);
    EXPECT_NE(mgr.book_for(LOC_AAPL), nullptr);
    EXPECT_EQ(mgr.book_for_symbol("AAPL"), nullptr);
}

// ============================================================
// on_add
// ============================================================

TEST(BookManagerTest, AddCreatesBookAndBBO) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);

    const OrderBook* book = mgr.book_for(LOC_AAPL);
    ASSERT_NE(book, nullptr);
    auto bbo = book->get_bbo();
    ASSERT_TRUE(bbo.has_value());
    EXPECT_EQ(bbo->bid.price,  BID_HIGH);
    EXPECT_EQ(bbo->bid.shares, 100);
    EXPECT_EQ(bbo->ask.price,  ASK_LOW);
    EXPECT_EQ(bbo->ask.shares, 50);
}

TEST(BookManagerTest, AddsAtSameLevelAccumulate) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_HIGH, LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 150);
}

TEST(BookManagerTest, AddsAtDifferentLevelsKeepBestBid) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_LOW,  LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_HIGH, LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price, BID_HIGH);
    EXPECT_EQ(bbo->bid.shares, 50);
}

TEST(BookManagerTest, AddsAtDifferentLocatesAreIsolated) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);
    mgr.on_add(R3, Side::Buy,  200, BID_HIGH, LOC_MSFT);
    mgr.on_add(R4, Side::Sell, 80,  ASK_LOW,  LOC_MSFT);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 100);
    EXPECT_EQ(mgr.book_for(LOC_MSFT)->get_bbo()->bid.shares, 200);
}

// ============================================================
// on_executed
// ============================================================

TEST(BookManagerTest, ExecutePartialReducesLevel) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);

    mgr.on_executed(R1, 30);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 70);
}

TEST(BookManagerTest, ExecuteFullErasesLevelWhenAlone) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_LOW,  LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_executed(R1, 100);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_LOW);
    EXPECT_EQ(bbo->bid.shares, 50);
}

TEST(BookManagerTest, ExecuteFullKeepsLevelWhenOthersExist) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_HIGH, LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_executed(R1, 100);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_HIGH);
    EXPECT_EQ(bbo->bid.shares, 50);
}

TEST(BookManagerTest, ExecuteMoreThanRemainingDoesNotUnderflow) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_executed(R1, 200);

    EXPECT_FALSE(mgr.book_for(LOC_AAPL)->has_bids());
}

TEST(BookManagerTest, ExecuteUnknownRefIsNoOp) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);

    mgr.on_executed(99999, 50);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 100);
}

TEST(BookManagerTest, ExecuteFullThenAgainIsNoOp) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_LOW,  LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_executed(R1, 100);
    mgr.on_executed(R1, 50);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_LOW);
    EXPECT_EQ(bbo->bid.shares, 50);
}

TEST(BookManagerTest, MultiplePartialExecutesDrainTheOrder) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_executed(R1, 30);
    mgr.on_executed(R1, 30);
    mgr.on_executed(R1, 40);

    EXPECT_FALSE(mgr.book_for(LOC_AAPL)->has_bids());
}

// ============================================================
// on_executed_with_price
// ============================================================

TEST(BookManagerTest, ExecutedWithPriceBehavesLikeExecuted) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);

    mgr.on_executed_with_price(R1, 30);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 70);
}

TEST(BookManagerTest, ExecutedWithPriceFullErasesOrder) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_executed_with_price(R1, 100);
    mgr.on_executed_with_price(R1, 10);

    EXPECT_FALSE(mgr.book_for(LOC_AAPL)->has_bids());
}

// ============================================================
// on_cancel
// ============================================================

TEST(BookManagerTest, CancelPartialReducesLevel) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);

    mgr.on_cancel(R1, 40);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 60);
}

TEST(BookManagerTest, CancelFullErasesLevelWhenAlone) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_LOW,  LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_cancel(R1, 100);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price, BID_LOW);
}

TEST(BookManagerTest, CancelMoreThanRemainingDoesNotUnderflow) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_cancel(R1, 99999);

    EXPECT_FALSE(mgr.book_for(LOC_AAPL)->has_bids());
}

TEST(BookManagerTest, CancelUnknownRefIsNoOp) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);

    mgr.on_cancel(99999, 50);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 100);
}

// ============================================================
// on_delete
// ============================================================

TEST(BookManagerTest, DeleteSoleOrderErasesLevel) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_LOW,  LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_delete(R1);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_LOW);
    EXPECT_EQ(bbo->bid.shares, 50);
}

TEST(BookManagerTest, DeleteOneOfManyAtLevelKeepsOthers) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_HIGH, LOC_AAPL);
    mgr.on_add(R3, Side::Buy,  25,  BID_HIGH, LOC_AAPL);
    mgr.on_add(R4, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_delete(R2);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 125);
}

TEST(BookManagerTest, DeleteAfterPartialExecuteReducesByRemaining) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_HIGH, LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_executed(R1, 30);
    mgr.on_delete(R1);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 50);
}

TEST(BookManagerTest, DeleteUnknownRefIsNoOp) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);

    mgr.on_delete(99999);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 100);
}

TEST(BookManagerTest, DeleteTwiceIsIdempotent) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_LOW,  LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_delete(R1);
    mgr.on_delete(R1);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_LOW);
    EXPECT_EQ(bbo->bid.shares, 50);
}

// ============================================================
// on_replace
// ============================================================

TEST(BookManagerTest, ReplaceSamePriceChangesShares) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);

    mgr.on_replace(R1, 999, 200, BID_HIGH);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_HIGH);
    EXPECT_EQ(bbo->bid.shares, 200);
}

TEST(BookManagerTest, ReplaceDifferentPriceMovesOrder) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);

    mgr.on_replace(R1, 999, 100, BID_LOW);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_LOW);
    EXPECT_EQ(bbo->bid.shares, 100);
}

TEST(BookManagerTest, ReplaceErasesOldLevelWhenAlone) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_LOW,  LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_replace(R1, 999, 100, BID_MID);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_MID);
    EXPECT_EQ(bbo->bid.shares, 100);
}

TEST(BookManagerTest, ReplaceAfterPartialExecuteUsesRemaining) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  50,  BID_HIGH, LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_executed(R1, 30);
    mgr.on_replace(R1, 999, 80, BID_LOW);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_HIGH);
    EXPECT_EQ(bbo->bid.shares, 50);
}

TEST(BookManagerTest, ReplaceInheritsSideFromOld) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);

    mgr.on_replace(R1, 999, 100, BID_LOW);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_LOW);
    EXPECT_EQ(bbo->bid.shares, 100);
    EXPECT_EQ(bbo->ask.price,  ASK_LOW);
    EXPECT_EQ(bbo->ask.shares, 50);
}

TEST(BookManagerTest, ReplaceInheritsLocateFromOld) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);
    mgr.on_add(R3, Side::Buy,  200, BID_HIGH, LOC_MSFT);
    mgr.on_add(R4, Side::Sell, 80,  ASK_LOW,  LOC_MSFT);

    mgr.on_replace(R1, 999, 150, BID_LOW);

    auto aapl = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(aapl->bid.price,  BID_LOW);
    EXPECT_EQ(aapl->bid.shares, 150);

    auto msft = mgr.book_for(LOC_MSFT)->get_bbo();
    EXPECT_EQ(msft->bid.price,  BID_HIGH);
    EXPECT_EQ(msft->bid.shares, 200);
}

TEST(BookManagerTest, ReplaceUnknownOldRefIsNoOp) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);

    mgr.on_replace(99999, 888, 200, BID_LOW);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 100);
}

TEST(BookManagerTest, ReplaceOldRefBecomesUnknown) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_replace(R1, 999, 50, BID_LOW);
    mgr.on_executed(R1, 50);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_LOW);
    EXPECT_EQ(bbo->bid.shares, 50);
}

TEST(BookManagerTest, ReplaceNewRefIsExecutable) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_replace(R1, 999, 80, BID_LOW);
    mgr.on_executed(999, 30);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_LOW);
    EXPECT_EQ(bbo->bid.shares, 50);
}

TEST(BookManagerTest, ReplaceNewRefIsDeletable) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_replace(R1, 999, 80, BID_LOW);
    mgr.on_delete(999);

    EXPECT_FALSE(mgr.book_for(LOC_AAPL)->has_bids());
}

TEST(BookManagerTest, ReplaceTwiceChainsCorrectly) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_replace(R1, 999, 80, BID_LOW);
    mgr.on_replace(999, 1234, 60, BID_MID);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_MID);
    EXPECT_EQ(bbo->bid.shares, 60);

    mgr.on_executed(999, 10);
    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 60);
}

// ============================================================
// Sell-side coverage (mirror of buy-side tests for the symmetric paths)
// ============================================================

TEST(BookManagerTest, SellSidePartialExecuteReducesLevel) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  1,   BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 100, ASK_LOW,  LOC_AAPL);

    mgr.on_executed(R2, 30);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->ask.price,  ASK_LOW);
    EXPECT_EQ(bbo->ask.shares, 70);
}

TEST(BookManagerTest, SellSideDeleteBestAskPromotesNext) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  1,   BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 100, ASK_LOW,  LOC_AAPL);
    mgr.on_add(R3, Side::Sell, 50,  ASK_HIGH, LOC_AAPL);

    mgr.on_delete(R2);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->ask.price,  ASK_HIGH);
    EXPECT_EQ(bbo->ask.shares, 50);
}

TEST(BookManagerTest, SellSideReplaceMovesPrice) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  1,   BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 100, ASK_LOW,  LOC_AAPL);

    mgr.on_replace(R2, 999, 80, ASK_HIGH);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->ask.price,  ASK_HIGH);
    EXPECT_EQ(bbo->ask.shares, 80);
}

// ============================================================
// Cross-locate isolation
// ============================================================

TEST(BookManagerTest, OperationsOnOneLocateDoNotAffectAnother) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 50,  ASK_LOW,  LOC_AAPL);
    mgr.on_add(R3, Side::Buy,  200, BID_HIGH, LOC_MSFT);
    mgr.on_add(R4, Side::Sell, 80,  ASK_LOW,  LOC_MSFT);

    mgr.on_executed(R1, 100);
    mgr.on_delete(R2);
    mgr.on_replace(R1, 5555, 999, BID_LOW);

    EXPECT_FALSE(mgr.book_for(LOC_AAPL)->has_bids());
    EXPECT_FALSE(mgr.book_for(LOC_AAPL)->has_asks());

    auto msft = mgr.book_for(LOC_MSFT)->get_bbo();
    ASSERT_TRUE(msft.has_value());
    EXPECT_EQ(msft->bid.price,  BID_HIGH);
    EXPECT_EQ(msft->bid.shares, 200);
    EXPECT_EQ(msft->ask.price,  ASK_LOW);
    EXPECT_EQ(msft->ask.shares, 80);
}

// ============================================================
// Sequence stress
// ============================================================

TEST(BookManagerTest, AddPartialExecuteCancelDeleteSequence) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_executed(R1, 20);
    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 80);

    mgr.on_cancel(R1, 30);
    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 50);

    mgr.on_executed(R1, 25);
    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 25);

    mgr.on_delete(R1);
    EXPECT_FALSE(mgr.book_for(LOC_AAPL)->has_bids());
}

TEST(BookManagerTest, AddReplaceExecuteDeleteSequence) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_replace(R1, 555, 80, BID_LOW);
    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.price, BID_LOW);

    mgr.on_executed(555, 30);
    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 50);

    mgr.on_delete(555);
    EXPECT_FALSE(mgr.book_for(LOC_AAPL)->has_bids());
}

TEST(BookManagerTest, ManyOrdersSamePriceDeletedOneByOne) {
    BookManager mgr;
    mgr.on_add(1001, Side::Buy,  10, BID_HIGH, LOC_AAPL);
    mgr.on_add(1002, Side::Buy,  20, BID_HIGH, LOC_AAPL);
    mgr.on_add(1003, Side::Buy,  30, BID_HIGH, LOC_AAPL);
    mgr.on_add(1004, Side::Sell, 1,  ASK_LOW,  LOC_AAPL);

    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 60);

    mgr.on_delete(1002);
    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 40);

    mgr.on_delete(1001);
    EXPECT_EQ(mgr.book_for(LOC_AAPL)->get_bbo()->bid.shares, 30);

    mgr.on_delete(1003);
    EXPECT_FALSE(mgr.book_for(LOC_AAPL)->has_bids());
}

TEST(BookManagerTest, LevelDepthTrackedCorrectlyAcrossMixedOps) {
    BookManager mgr;
    mgr.on_add(R1, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R2, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R3, Side::Buy,  100, BID_HIGH, LOC_AAPL);
    mgr.on_add(R4, Side::Sell, 1,   ASK_LOW,  LOC_AAPL);

    mgr.on_executed(R1, 50);
    mgr.on_cancel(R2, 30);
    mgr.on_delete(R3);

    auto bbo = mgr.book_for(LOC_AAPL)->get_bbo();
    EXPECT_EQ(bbo->bid.price,  BID_HIGH);
    EXPECT_EQ(bbo->bid.shares, 120);
}
